#pragma once

#include <ufd/DomainConfig.h>
#include <ufd/SurfaceExtractor.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/base/gf/range3d.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace ufd {

class DomainGenerator {
public:
    explicit DomainGenerator(const DomainConfig& config);

    // Generate the far-field domain mesh on the given stage based on
    // the bounding box of the object surface.
    // Returns the prim path of the created domain mesh.
    std::string Generate(UsdStageRefPtr stage,
                         const GfRange3d& objectBounds) const;

private:
    DomainConfig _config;

    void _GenerateBox(UsdStageRefPtr stage,
                      const GfRange3d& domainBounds,
                      const std::string& primPath) const;
};

} // namespace ufd
