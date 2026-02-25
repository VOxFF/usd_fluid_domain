#include <ufd/SurfaceExtractor.h>

#include <pxr/usd/usdGeom/xformCache.h>

//#define UFD_DEBUG_TRANSFORMS
#ifdef UFD_DEBUG_TRANSFORMS
#include <iostream>
#endif

namespace ufd {

SurfaceData SurfaceExtractor::extract(
    const std::vector<UsdGeomMesh>& meshes) const {
    SurfaceData result;
    UsdGeomXformCache xform_cache;

    for (const auto& mesh : meshes) {
        VtVec3fArray points;
        mesh.GetPointsAttr().Get(&points);

        VtIntArray face_vertex_counts;
        mesh.GetFaceVertexCountsAttr().Get(&face_vertex_counts);

        VtIntArray face_vertex_indices;
        mesh.GetFaceVertexIndicesAttr().Get(&face_vertex_indices);

        GfMatrix4d world_xform =
            xform_cache.GetLocalToWorldTransform(mesh.GetPrim());

#ifdef UFD_DEBUG_TRANSFORMS
        std::cerr << "[SurfaceExtractor] " << mesh.GetPrim().GetPath()
                  << "\n  world_xform:\n" << world_xform << "\n";
#endif

        int point_offset = static_cast<int>(result.points.size());

        for (const auto& pt : points) {
            GfVec3d world_pt = world_xform.Transform(
                GfVec3d(pt[0], pt[1], pt[2]));
            result.points.push_back(
                GfVec3f(world_pt[0], world_pt[1], world_pt[2]));
        }

        for (const auto& count : face_vertex_counts) {
            result.face_vertex_counts.push_back(count);
        }

        for (const auto& idx : face_vertex_indices) {
            result.face_vertex_indices.push_back(idx + point_offset);
        }
    }

    return result;
}

GfRange3d SurfaceExtractor::compute_bounding_box(
    const SurfaceData& surface) const {
    GfRange3d bbox;
    for (const auto& pt : surface.points) {
        bbox.UnionWith(GfVec3d(pt[0], pt[1], pt[2]));
    }
    return bbox;
}

} // namespace ufd
