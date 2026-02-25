#include <ufd/DomainGenerator.h>

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/sdf/path.h>

#include <algorithm>
#include <cmath>

namespace ufd {

DomainGenerator::DomainGenerator(const DomainConfig& config)
    : config_(config) {
    config_.flow_direction.Normalize();
}

std::string DomainGenerator::generate(
    UsdStageRefPtr stage,
    const GfRange3d& object_bounds) const {
    const GfVec3d domain_center = object_bounds.GetMidpoint()
                                + config_.origin_offset;
    const GfVec3d size          = object_bounds.GetSize();
    const std::string prim_path = "/FluidDomain";

    switch (config_.shape) {

    case DomainShape::Box: {
        const GfVec3d half_size = size * (config_.extent_multiplier * 0.5);
        const GfRange3d domain_bounds(domain_center - half_size,
                                      domain_center + half_size);
        generate_box(stage, domain_bounds, prim_path);
        break;
    }

    case DomainShape::Cylinder: {
        const GfVec3d& f = config_.flow_direction;

        // Along-flow half-extent of the object bbox
        double along = std::abs(f[0]) * size[0]
                     + std::abs(f[1]) * size[1]
                     + std::abs(f[2]) * size[2];
        double half_length = along * 0.5 * config_.extent_multiplier;

        // Cross-flow radius: max perpendicular distance from object centroid
        // to each of the 8 bbox corners
        const GfVec3d half = size * 0.5;
        double radius = 0.0;
        for (int i = 0; i < 8; ++i) {
            GfVec3d v((i & 1) ? half[0] : -half[0],
                      (i & 2) ? half[1] : -half[1],
                      (i & 4) ? half[2] : -half[2]);
            GfVec3d perp = v - GfDot(v, f) * f;
            radius = std::max(radius, perp.GetLength());
        }
        radius *= config_.extent_multiplier;

        generate_cylinder(stage, domain_center, f, radius, half_length,
                          prim_path);
        break;
    }
    }

    return prim_path;
}

void DomainGenerator::generate_box(
    UsdStageRefPtr stage,
    const GfRange3d& domain_bounds,
    const std::string& prim_path) const {
    const GfVec3d mn = domain_bounds.GetMin();
    const GfVec3d mx = domain_bounds.GetMax();

    //      7-----6
    //     /|    /|
    //    4-----5 |
    //    | 3---|-2
    //    |/    |/
    //    0-----1
    //
    // X: mn→mx  Y: mn→mx  Z: mn→mx
    VtVec3fArray points = {
        GfVec3f(mn[0], mn[1], mn[2]), // 0
        GfVec3f(mx[0], mn[1], mn[2]), // 1
        GfVec3f(mx[0], mx[1], mn[2]), // 2
        GfVec3f(mn[0], mx[1], mn[2]), // 3
        GfVec3f(mn[0], mn[1], mx[2]), // 4
        GfVec3f(mx[0], mn[1], mx[2]), // 5
        GfVec3f(mx[0], mx[1], mx[2]), // 6
        GfVec3f(mn[0], mx[1], mx[2]), // 7
    };

    // 6 quad faces, outward normals (CCW winding viewed from outside)
    VtIntArray face_vertex_counts = {4, 4, 4, 4, 4, 4};
    VtIntArray face_vertex_indices = {
        0, 3, 2, 1, // bottom  (-Z)
        4, 5, 6, 7, // top     (+Z)
        0, 1, 5, 4, // front   (-Y)
        3, 7, 6, 2, // back    (+Y)
        0, 4, 7, 3, // left    (-X)
        1, 2, 6, 5, // right   (+X)
    };

    auto mesh = UsdGeomMesh::Define(stage, SdfPath(prim_path));
    mesh.GetPointsAttr().Set(points);
    mesh.GetFaceVertexCountsAttr().Set(face_vertex_counts);
    mesh.GetFaceVertexIndicesAttr().Set(face_vertex_indices);
    mesh.GetSubdivisionSchemeAttr().Set(UsdGeomTokens->none);
}

void DomainGenerator::generate_cylinder(
    UsdStageRefPtr stage,
    const GfVec3d& center,
    const GfVec3d& axis,
    double radius,
    double half_length,
    const std::string& prim_path) const {
    const int N = config_.cylinder_segments;

    // Build orthonormal basis perpendicular to axis
    GfVec3d ref = (std::abs(axis[0]) < 0.9) ? GfVec3d(1, 0, 0)
                                             : GfVec3d(0, 1, 0);
    GfVec3d u = (ref - GfDot(ref, axis) * axis).GetNormalized();
    GfVec3d v = GfCross(axis, u);

    const GfVec3d bottom_center = center - half_length * axis;
    const GfVec3d top_center    = center + half_length * axis;
    const double  two_pi        = 2.0 * std::acos(-1.0);

    VtVec3fArray points;
    points.reserve(2 * N + 2);

    // 0..N-1: bottom ring
    for (int i = 0; i < N; ++i) {
        double theta = two_pi * i / N;
        GfVec3d p = bottom_center
                  + radius * (std::cos(theta) * u + std::sin(theta) * v);
        points.emplace_back(p[0], p[1], p[2]);
    }

    // N..2N-1: top ring
    for (int i = 0; i < N; ++i) {
        double theta = two_pi * i / N;
        GfVec3d p = top_center
                  + radius * (std::cos(theta) * u + std::sin(theta) * v);
        points.emplace_back(p[0], p[1], p[2]);
    }

    // 2N: bottom cap center,  2N+1: top cap center
    points.emplace_back(bottom_center[0], bottom_center[1], bottom_center[2]);
    points.emplace_back(top_center[0],    top_center[1],    top_center[2]);

    VtIntArray face_vertex_counts;
    VtIntArray face_vertex_indices;
    face_vertex_counts.reserve(3 * N);
    face_vertex_indices.reserve(4 * N + 3 * N + 3 * N);

    // Side: N quads, outward normals
    for (int i = 0; i < N; ++i) {
        int next = (i + 1) % N;
        face_vertex_counts.push_back(4);
        face_vertex_indices.push_back(i);
        face_vertex_indices.push_back(next);
        face_vertex_indices.push_back(N + next);
        face_vertex_indices.push_back(N + i);
    }

    // Bottom cap: N triangles, outward normal = -axis
    for (int i = 0; i < N; ++i) {
        int next = (i + 1) % N;
        face_vertex_counts.push_back(3);
        face_vertex_indices.push_back(2 * N);
        face_vertex_indices.push_back(next);
        face_vertex_indices.push_back(i);
    }

    // Top cap: N triangles, outward normal = +axis
    for (int i = 0; i < N; ++i) {
        int next = (i + 1) % N;
        face_vertex_counts.push_back(3);
        face_vertex_indices.push_back(2 * N + 1);
        face_vertex_indices.push_back(N + i);
        face_vertex_indices.push_back(N + next);
    }

    auto mesh = UsdGeomMesh::Define(stage, SdfPath(prim_path));
    mesh.GetPointsAttr().Set(points);
    mesh.GetFaceVertexCountsAttr().Set(face_vertex_counts);
    mesh.GetFaceVertexIndicesAttr().Set(face_vertex_indices);
    mesh.GetSubdivisionSchemeAttr().Set(UsdGeomTokens->none);
}

} // namespace ufd
