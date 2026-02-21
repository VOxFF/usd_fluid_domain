#pragma once

#include <vector>

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/base/gf/range3d.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace ufd {

struct SurfaceData {
    VtVec3fArray points;
    VtIntArray   faceVertexCounts;
    VtIntArray   faceVertexIndices;
};

class SurfaceExtractor {
public:
    // Extract and merge the surface from a set of UsdGeomMesh prims
    // into a single combined surface representation.
    SurfaceData Extract(const std::vector<UsdGeomMesh>& meshes) const;

    // Compute the axis-aligned bounding box of the extracted surface.
    GfRange3d ComputeBoundingBox(const SurfaceData& surface) const;
};

} // namespace ufd
