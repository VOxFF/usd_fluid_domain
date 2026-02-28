#pragma once

#include <ufd/SurfaceExtractor.h>

#include <pxr/usd/usdGeom/mesh.h>

#include <vector>

PXR_NAMESPACE_USING_DIRECTIVE

namespace ufd {

struct EnvelopeConfig {
    double voxel_size     = 0.1;  // voxel edge length in world units
    double hole_threshold = 0.5;  // morphological closing radius in world units;
                                  // holes smaller than this are bridged
};

// Builds a watertight outer envelope surface from one or more meshes using
// OpenVDB signed distance fields.  Meshes are unioned, then morphological
// closing is applied to bridge small holes, and the resulting SDF is
// iso-surfaced back into a polygon mesh.
class EnvelopeBuilder {
public:
    explicit EnvelopeBuilder(const EnvelopeConfig& config = {});

    // Build the envelope.  Meshes are read with their USD world-space
    // transforms applied.  Returns an empty SurfaceData if meshes is empty.
    SurfaceData build(const std::vector<UsdGeomMesh>& meshes) const;

private:
    EnvelopeConfig config_;
};

} // namespace ufd
