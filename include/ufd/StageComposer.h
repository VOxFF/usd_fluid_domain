#pragma once

#include <filesystem>
#include <string>
#include <utility>
#include <vector>

#include <pxr/usd/usd/stage.h>

PXR_NAMESPACE_USING_DIRECTIVE

namespace ufd {

// Defines the role of each component within the composed stage.
// Values determine sublayer strength — higher value = stronger (wins conflicts).
enum class ComponentType {
    InputGeometry = 0,  // original scene, already on disk — not re-saved
    FluidDomain   = 1,  // generated layer — saved on write()
};

class StageComposer {
public:
    explicit StageComposer(const std::string& root_path);
    ~StageComposer();

    // Register a component stage. InputGeometry is not saved; all others are.
    void add_component(ComponentType type, UsdStageRefPtr stage);

    // Save component layers (except InputGeometry) and write the root layer
    // with the sublayer stack ordered by component strength.
    bool write() const;

private:
    std::string root_path_;
    std::vector<std::pair<ComponentType, UsdStageRefPtr>> components_;

    // Author a material on the stage and bind it to the given mesh prim.
    void apply_material(ComponentType type,
                        UsdStageRefPtr stage,
                        const std::string& mesh_prim_path) const;
};

} // namespace ufd
