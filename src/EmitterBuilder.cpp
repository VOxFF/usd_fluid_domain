#include <ufd/EmitterBuilder.h>

#include <pxr/usd/usdGeom/basisCurves.h>
#include <pxr/usd/usdGeom/cone.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdGeom/tokens.h>
#include <pxr/usd/usdGeom/xformable.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/base/gf/quatf.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>

#include <algorithm>
#include <cmath>
#include <functional>
#include <numeric>
#include <unordered_map>

namespace ufd {

namespace {

// Build a local tangent frame for a given normal direction.
// Returns (right, up) — both perpendicular to dir and to each other.
std::pair<GfVec3f, GfVec3f> tangent_frame(const GfVec3f& dir) {
    GfVec3f ref = (std::abs(dir[1]) < 0.9f) ? GfVec3f(0, 1, 0) : GfVec3f(1, 0, 0);
    GfVec3f right = GfCross(dir, ref).GetNormalized();
    GfVec3f up    = GfCross(right, dir).GetNormalized();
    return {right, up};
}

// Quaternion rotating the Y-axis (0,1,0) onto dir.
GfQuatf rotation_to(const GfVec3f& dir) {
    const GfVec3f y(0, 1, 0);
    GfVec3f cross = GfCross(y, dir);
    float   dot   = GfDot(y, dir);
    if (cross.GetLength() < 1e-6f)
        return dot > 0.0f ? GfQuatf(1, 0, 0, 0)      // same direction
                          : GfQuatf(0, 1, 0, 0);      // 180° flip around X
    cross.Normalize();
    float half = std::acos(std::clamp(dot, -1.0f, 1.0f)) * 0.5f;
    float s    = std::sin(half);
    return GfQuatf(std::cos(half), cross[0]*s, cross[1]*s, cross[2]*s);
}

} // namespace

EmitterBuilder::EmitterBuilder(const EmitterConfig& config)
    : config_(config) {}

// ---------------------------------------------------------------------------

void EmitterBuilder::build_domain_face(const GfRange3d& bounds,
                                       VtVec3fArray&    points,
                                       VtIntArray&      counts,
                                       VtIntArray&      indices,
                                       GfVec3f&         center) const
{
    const float x0 = static_cast<float>(bounds.GetMin()[0]);
    const float y0 = static_cast<float>(bounds.GetMin()[1]);
    const float z0 = static_cast<float>(bounds.GetMin()[2]);
    const float x1 = static_cast<float>(bounds.GetMax()[0]);
    const float y1 = static_cast<float>(bounds.GetMax()[1]);
    const float z1 = static_cast<float>(bounds.GetMax()[2]);

    using Corners = std::array<GfVec3f, 4>;
    const std::array<Corners, 6> faces = {{
        {GfVec3f(x0,y0,z0), GfVec3f(x0,y0,z1), GfVec3f(x0,y1,z1), GfVec3f(x0,y1,z0)},  // 0 xmin
        {GfVec3f(x1,y0,z0), GfVec3f(x1,y1,z0), GfVec3f(x1,y1,z1), GfVec3f(x1,y0,z1)},  // 1 xmax
        {GfVec3f(x0,y0,z0), GfVec3f(x1,y0,z0), GfVec3f(x1,y0,z1), GfVec3f(x0,y0,z1)},  // 2 ymin
        {GfVec3f(x0,y1,z0), GfVec3f(x0,y1,z1), GfVec3f(x1,y1,z1), GfVec3f(x1,y1,z0)},  // 3 ymax
        {GfVec3f(x0,y0,z0), GfVec3f(x0,y1,z0), GfVec3f(x1,y1,z0), GfVec3f(x1,y0,z0)},  // 4 zmin
        {GfVec3f(x0,y0,z1), GfVec3f(x1,y0,z1), GfVec3f(x1,y1,z1), GfVec3f(x0,y1,z1)},  // 5 zmax
    }};

    const int idx = std::clamp(config_.face_index, 0, 5);
    const auto& corners = faces[idx];

    points  = VtVec3fArray(corners.begin(), corners.end());
    counts  = {4};
    indices = {0, 1, 2, 3};
    center  = (corners[0] + corners[1] + corners[2] + corners[3]) * 0.25f;
}

void EmitterBuilder::build_quad(VtVec3fArray& points,
                                VtIntArray&   counts,
                                VtIntArray&   indices,
                                GfVec3f&      center) const
{
    GfVec3f dir = config_.direction.GetNormalized();
    auto [right, up] = tangent_frame(dir);

    const float hw = config_.size[0] * 0.5f;
    const float hh = config_.size[1] * 0.5f;
    center = config_.location;
    points = {center - right*hw - up*hh,
              center + right*hw - up*hh,
              center + right*hw + up*hh,
              center - right*hw + up*hh};
    counts  = {4};
    indices = {0, 1, 2, 3};
}

void EmitterBuilder::build_circle(VtVec3fArray& points,
                                  VtIntArray&   counts,
                                  VtIntArray&   indices,
                                  GfVec3f&      center) const
{
    constexpr int N = 32;
    GfVec3f dir = config_.direction.GetNormalized();
    auto [right, up] = tangent_frame(dir);

    center = config_.location;
    points.resize(N);
    for (int i = 0; i < N; ++i) {
        float a = static_cast<float>(2.0 * M_PI * i / N);
        points[i] = center + right * (config_.radius * std::cos(a))
                           + up    * (config_.radius * std::sin(a));
    }
    counts = {N};
    indices.resize(N);
    std::iota(indices.begin(), indices.end(), 0);
}

void EmitterBuilder::add_arrow(UsdStageRefPtr     stage,
                               const std::string& base_path,
                               const GfVec3f&     center,
                               const GfVec3f&     dir,
                               float              arrow_len) const
{
    const GfVec3f tip = center + dir * arrow_len;
    const float   cone_h = arrow_len * 0.25f;
    const float   cone_r = cone_h   * 0.4f;
    const GfVec3f red(1.0f, 0.15f, 0.15f);

    // Shaft — linear BasisCurves
    auto shaft = UsdGeomBasisCurves::Define(stage, SdfPath(base_path + "Shaft"));
    shaft.CreateTypeAttr(VtValue(TfToken("linear")));
    shaft.GetPointsAttr()           .Set(VtVec3fArray({center, tip}));
    shaft.GetCurveVertexCountsAttr().Set(VtIntArray({2}));
    shaft.GetWidthsAttr()           .Set(VtFloatArray({arrow_len * 0.04f,
                                                       arrow_len * 0.04f}));
    shaft.GetDisplayColorAttr()     .Set(VtVec3fArray({red}));

    // Tip — Cone oriented along dir
    auto cone = UsdGeomCone::Define(stage, SdfPath(base_path + "Cone"));
    cone.GetHeightAttr().Set(static_cast<double>(cone_h));
    cone.GetRadiusAttr().Set(static_cast<double>(cone_r));
    cone.GetDisplayColorAttr().Set(VtVec3fArray({red}));

    // USD Cone is along Y by default; rotate to dir, position at tip + dir*cone_h/2
    GfVec3f cone_pos = tip + dir * (cone_h * 0.5f);
    auto xf = UsdGeomXformable(cone.GetPrim());
    xf.AddTranslateOp().Set(GfVec3d(cone_pos[0], cone_pos[1], cone_pos[2]));
    xf.AddOrientOp()   .Set(rotation_to(dir));
}

// ---------------------------------------------------------------------------

std::string EmitterBuilder::build(UsdStageRefPtr   stage,
                                  const GfRange3d& domain_bounds) const
{
    const std::string prim_path = "/" + config_.name;

    VtVec3fArray points;
    VtIntArray   counts;
    VtIntArray   indices;
    GfVec3f      center;

    using BuildFn = std::function<void()>;
    const std::unordered_map<EmitterShape, BuildFn> builders = {
        {EmitterShape::DomainFace, [&]{ build_domain_face(domain_bounds, points, counts, indices, center); }},
        {EmitterShape::Quad,       [&]{ build_quad(points, counts, indices, center); }},
        {EmitterShape::Circle,     [&]{ build_circle(points, counts, indices, center); }},
    };

    const auto it = builders.find(config_.shape);
    if (it == builders.end()) return {};
    it->second();

    if (points.empty()) return {};

    // Surface mesh
    auto mesh = UsdGeomMesh::Define(stage, SdfPath(prim_path));
    mesh.GetPointsAttr()           .Set(points);
    mesh.GetFaceVertexCountsAttr() .Set(counts);
    mesh.GetFaceVertexIndicesAttr().Set(indices);
    mesh.GetSubdivisionSchemeAttr().Set(UsdGeomTokens->none);

    // Solver attributes
    auto prim = stage->GetPrimAtPath(SdfPath(prim_path));
    prim.CreateAttribute(TfToken("emitter:direction"), SdfValueTypeNames->Float3)
        .Set(config_.direction.GetNormalized());
    prim.CreateAttribute(TfToken("emitter:magnitude"), SdfValueTypeNames->Float)
        .Set(config_.magnitude);
    prim.CreateAttribute(TfToken("emitter:rate"),      SdfValueTypeNames->Float)
        .Set(config_.rate);

    // Arrow length: fraction of domain diagonal or emitter size
    float arrow_len = 1.0f;
    if (config_.shape == EmitterShape::DomainFace) {
        GfVec3d d = domain_bounds.GetMax() - domain_bounds.GetMin();
        arrow_len = static_cast<float>(d.GetLength()) * 0.15f;
    } else if (config_.shape == EmitterShape::Quad) {
        arrow_len = GfVec2f(config_.size[0], config_.size[1]).GetLength() * 0.5f;
    } else {
        arrow_len = config_.radius * 2.0f;
    }

    add_arrow(stage, prim_path, center, config_.direction.GetNormalized(), arrow_len);

    return prim_path;
}

} // namespace ufd
