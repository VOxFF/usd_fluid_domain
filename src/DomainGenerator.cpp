#include <ufd/DomainGenerator.h>

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/sdf/path.h>

namespace ufd {

DomainGenerator::DomainGenerator(const DomainConfig& config)
    : config_(config) {}

std::string DomainGenerator::generate(
    UsdStageRefPtr /*stage*/,
    const GfRange3d& /*object_bounds*/) const {
    // TODO: compute domain extents from object_bounds * extent_multiplier,
    //       apply origin_offset, call the appropriate shape generator,
    //       and author the mesh prim on the stage.
    return "/FluidDomain";
}

void DomainGenerator::generate_box(
    UsdStageRefPtr /*stage*/,
    const GfRange3d& /*domain_bounds*/,
    const std::string& /*prim_path*/) const {
    // TODO: author a box UsdGeomMesh (8 vertices, 6 quad faces).
}

} // namespace ufd
