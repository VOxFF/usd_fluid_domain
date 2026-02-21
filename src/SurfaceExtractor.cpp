#include <ufd/SurfaceExtractor.h>

#include <pxr/usd/usdGeom/xformCache.h>

namespace ufd {

SurfaceData SurfaceExtractor::extract(
    const std::vector<UsdGeomMesh>& /*meshes*/) const {
    // TODO: iterate meshes, read points/topology, apply transforms,
    //       merge into a single SurfaceData.
    return {};
}

GfRange3d SurfaceExtractor::compute_bounding_box(
    const SurfaceData& /*surface*/) const {
    // TODO: compute AABB from surface points.
    return {};
}

} // namespace ufd
