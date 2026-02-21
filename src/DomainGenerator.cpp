#include <ufd/DomainGenerator.h>

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/sdf/path.h>

namespace ufd {

DomainGenerator::DomainGenerator(const DomainConfig& config)
    : _config(config) {}

std::string DomainGenerator::Generate(
    UsdStageRefPtr /*stage*/,
    const GfRange3d& /*objectBounds*/) const {
    // TODO: compute domain extents from objectBounds * extentMultiplier,
    //       apply originOffset, call the appropriate shape generator,
    //       and author the mesh prim on the stage.
    return "/FluidDomain";
}

void DomainGenerator::_GenerateBox(
    UsdStageRefPtr /*stage*/,
    const GfRange3d& /*domainBounds*/,
    const std::string& /*primPath*/) const {
    // TODO: author a box UsdGeomMesh (8 vertices, 6 quad faces).
}

} // namespace ufd
