#pragma once

#include <pxr/base/gf/vec3d.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace ufd {

enum class DomainShape {
    Box,
    Cylinder,
    Sphere
};

struct DomainConfig {
    DomainShape shape = DomainShape::Box;

    // Multiplier applied to the bounding-box extents of the object
    // to determine the far-field boundary size.
    double extent_multiplier = 10.0;

    // Manual offset of the domain origin relative to the object centroid.
    GfVec3d origin_offset{0.0, 0.0, 0.0};

    // When true, generate only half the domain (symmetry about the XZ plane).
    bool symmetry_y = false;

    // Parse config from a simple key=value file. Returns true on success.
    bool load_from_file(const std::string& path);
};

} // namespace ufd
