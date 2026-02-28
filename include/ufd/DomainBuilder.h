#pragma once

#include <ufd/DomainConfig.h>
#include <ufd/SurfaceExtractor.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/base/gf/range3d.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace ufd {

class DomainBuilder {
public:
    explicit DomainBuilder(const DomainConfig& config);

    // Build the far-field domain mesh on the given stage based on
    // the bounding box of the object surface.
    // Returns the prim path of the created domain mesh.
    std::string build(UsdStageRefPtr stage,
                      const GfRange3d& object_bounds) const;

private:
    DomainConfig config_;

    void build_box(UsdStageRefPtr stage,
                   const GfRange3d& domain_bounds,
                   const std::string& prim_path) const;

    void build_cylinder(UsdStageRefPtr stage,
                        const GfVec3d& center,
                        const GfVec3d& axis,
                        double radius,
                        double half_length,
                        const std::string& prim_path) const;
};

} // namespace ufd
