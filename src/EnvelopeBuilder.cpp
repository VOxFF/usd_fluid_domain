#include <ufd/EnvelopeBuilder.h>

#include <openvdb/openvdb.h>
#include <openvdb/tools/Composite.h>
#include <openvdb/tools/LevelSetFilter.h>
#include <openvdb/tools/LevelSetRebuild.h>
#include <openvdb/tools/MeshToVolume.h>
#include <openvdb/tools/VolumeToMesh.h>

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xformCache.h>
#include <pxr/usd/sdf/path.h>

namespace ufd {

EnvelopeBuilder::EnvelopeBuilder(const EnvelopeConfig& config)
    : config_(config) {}

std::string EnvelopeBuilder::build(
    UsdStageRefPtr stage,
    const std::vector<UsdGeomMesh>& meshes) const
{
    const std::string prim_path = "/Envelope";
    if (meshes.empty()) return {};

    openvdb::initialize();

    const float vox         = static_cast<float>(config_.voxel_size);
    const float close_world = static_cast<float>(config_.hole_threshold);
    const float close_vox   = close_world / vox;
    // Narrow band must be wide enough to survive the closing pass
    const float half_band   = close_vox + 3.0f;

    auto xform = openvdb::math::Transform::createLinearTransform(
        static_cast<double>(vox));

    UsdGeomXformCache xform_cache;
    openvdb::FloatGrid::Ptr sdf;

    for (const auto& mesh : meshes) {
        VtVec3fArray usd_pts;
        mesh.GetPointsAttr().Get(&usd_pts);

        VtIntArray face_counts;
        VtIntArray face_indices;
        mesh.GetFaceVertexCountsAttr().Get(&face_counts);
        mesh.GetFaceVertexIndicesAttr().Get(&face_indices);

        GfMatrix4d world_xform =
            xform_cache.GetLocalToWorldTransform(mesh.GetPrim());

        // Convert USD points to world-space OpenVDB Vec3s
        std::vector<openvdb::Vec3s> points;
        points.reserve(usd_pts.size());
        for (const auto& p : usd_pts) {
            GfVec3d wp = world_xform.Transform(GfVec3d(p[0], p[1], p[2]));
            points.emplace_back(
                static_cast<float>(wp[0]),
                static_cast<float>(wp[1]),
                static_cast<float>(wp[2]));
        }

        // Fan-triangulate faces; keep quads as quads
        std::vector<openvdb::Vec3I> triangles;
        std::vector<openvdb::Vec4I> quads;
        int cursor = 0;
        for (int count : face_counts) {
            if (count == 3) {
                triangles.emplace_back(
                    static_cast<uint32_t>(face_indices[cursor]),
                    static_cast<uint32_t>(face_indices[cursor + 1]),
                    static_cast<uint32_t>(face_indices[cursor + 2]));
            } else if (count == 4) {
                quads.emplace_back(
                    static_cast<uint32_t>(face_indices[cursor]),
                    static_cast<uint32_t>(face_indices[cursor + 1]),
                    static_cast<uint32_t>(face_indices[cursor + 2]),
                    static_cast<uint32_t>(face_indices[cursor + 3]));
            } else {
                // Fan-triangulate n-gons (n > 4)
                for (int i = 1; i < count - 1; ++i) {
                    triangles.emplace_back(
                        static_cast<uint32_t>(face_indices[cursor]),
                        static_cast<uint32_t>(face_indices[cursor + i]),
                        static_cast<uint32_t>(face_indices[cursor + i + 1]));
                }
            }
            cursor += count;
        }

        auto mesh_sdf =
            openvdb::tools::meshToSignedDistanceField<openvdb::FloatGrid>(
                *xform, points, triangles, quads, half_band, half_band);

        if (!sdf) {
            sdf = mesh_sdf;
        } else {
            openvdb::tools::csgUnion(*sdf, *mesh_sdf);
        }
    }

    if (!sdf || sdf->empty()) return {};

    // Morphological closing: dilate then erode by close_world (world units).
    // Bridges holes/gaps smaller than hole_threshold.
    if (close_world > 0.0f) {
        {
            openvdb::tools::LevelSetFilter<openvdb::FloatGrid> f(*sdf);
            f.offset(-close_world);  // dilate
        }
        sdf = openvdb::tools::levelSetRebuild(*sdf, 0.0f, half_band, half_band);
        {
            openvdb::tools::LevelSetFilter<openvdb::FloatGrid> f(*sdf);
            f.offset(close_world);   // erode
        }
        sdf = openvdb::tools::levelSetRebuild(*sdf, 0.0f, half_band, half_band);
    }

    // Iso-surface the SDF at the zero level set
    openvdb::tools::VolumeToMesh mesher(0.0);
    mesher(*sdf);

    // Collect points
    const size_t npts = mesher.pointListSize();
    VtVec3fArray points(static_cast<unsigned>(npts));
    const auto& vdb_pts = mesher.pointList();
    for (size_t i = 0; i < npts; ++i) {
        points[static_cast<unsigned>(i)] =
            GfVec3f(vdb_pts[i][0], vdb_pts[i][1], vdb_pts[i][2]);
    }

    // Collect face topology
    VtIntArray face_vertex_counts;
    VtIntArray face_vertex_indices;
    const auto& pools = mesher.polygonPoolList();
    for (size_t pi = 0; pi < mesher.polygonPoolListSize(); ++pi) {
        const auto& pool = pools[pi];

        for (size_t qi = 0; qi < pool.numQuads(); ++qi) {
            const openvdb::Vec4I& q = pool.quad(qi);
            face_vertex_counts.push_back(4);
            face_vertex_indices.push_back(static_cast<int>(q[0]));
            face_vertex_indices.push_back(static_cast<int>(q[1]));
            face_vertex_indices.push_back(static_cast<int>(q[2]));
            face_vertex_indices.push_back(static_cast<int>(q[3]));
        }

        for (size_t ti = 0; ti < pool.numTriangles(); ++ti) {
            const openvdb::Vec3I& t = pool.triangle(ti);
            face_vertex_counts.push_back(3);
            face_vertex_indices.push_back(static_cast<int>(t[0]));
            face_vertex_indices.push_back(static_cast<int>(t[1]));
            face_vertex_indices.push_back(static_cast<int>(t[2]));
        }
    }

    // Write to stage
    auto mesh = UsdGeomMesh::Define(stage, SdfPath(prim_path));
    mesh.GetPointsAttr().Set(points);
    mesh.GetFaceVertexCountsAttr().Set(face_vertex_counts);
    mesh.GetFaceVertexIndicesAttr().Set(face_vertex_indices);
    mesh.GetSubdivisionSchemeAttr().Set(UsdGeomTokens->none);

    return prim_path;
}

} // namespace ufd
