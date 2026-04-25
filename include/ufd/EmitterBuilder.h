#pragma once

#include <pxr/usd/usd/stage.h>
#include <pxr/base/gf/range3d.h>
#include <pxr/base/gf/vec2f.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/vt/array.h>

#include <string>

PXR_NAMESPACE_USING_DIRECTIVE

namespace ufd {

enum class EmitterShape {
    DomainFace,  // one of the 6 axis-aligned faces of the domain box
    Quad,        // rectangular surface (width × height)
    Circle,      // circular surface (32-segment polygon)
    Custom,      // not yet implemented
};

struct EmitterConfig {
    std::string  name      = "Emitter";
    EmitterShape shape     = EmitterShape::DomainFace;

    // DomainFace: 0=xmin, 1=xmax, 2=ymin, 3=ymax, 4=zmin, 5=zmax
    int      face_index = 0;

    GfVec2f  size      = {1.0f, 1.0f};  // Quad (width, height)
    float    radius    = 0.5f;           // Circle

    GfVec3f  location  = {0.0f, 0.0f, 0.0f};  // Quad / Circle center

    GfVec3f  direction = {1.0f, 0.0f, 0.0f};  // normalized flow direction
    float    magnitude = 1.0f;                 // flow speed
    float    rate      = 100.0f;               // particles/sec (SPH)
};

// Builds an emitter surface in a USD stage:
//   - A mesh representing the emitter area (80% transparent red)
//   - A velocity arrow (shaft + cone tip) from the center
//   - Custom USD attributes encoding config for the solver
class EmitterBuilder {
public:
    explicit EmitterBuilder(const EmitterConfig& config = {});

    // Build the emitter and write prims to stage.
    // domain_bounds is required when shape == DomainFace.
    // Returns the root prim path on success, or empty string on failure.
    std::string build(UsdStageRefPtr stage,
                      const GfRange3d& domain_bounds = {}) const;

private:
    EmitterConfig config_;

    void build_domain_face(const GfRange3d& bounds,
                           VtVec3fArray& points,
                           VtIntArray&   counts,
                           VtIntArray&   indices,
                           GfVec3f&      center) const;

    void build_quad(VtVec3fArray& points,
                    VtIntArray&   counts,
                    VtIntArray&   indices,
                    GfVec3f&      center) const;

    void build_circle(VtVec3fArray& points,
                      VtIntArray&   counts,
                      VtIntArray&   indices,
                      GfVec3f&      center) const;

    void add_arrow(UsdStageRefPtr     stage,
                   const std::string& base_path,
                   const GfVec3f&     center,
                   const GfVec3f&     dir,
                   float              arrow_len) const;
};

} // namespace ufd
