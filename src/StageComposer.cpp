#include <ufd/StageComposer.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/sdf/valueTypeName.h>
#include <pxr/usd/usdShade/material.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>
#include <pxr/base/vt/value.h>

#include <algorithm>
#include <unordered_map>

namespace ufd {

namespace {

struct MaterialStyle {
    GfVec3f color;
    float   opacity;
};

// Maps each component type to the root prim path it authors on its stage.
const std::unordered_map<ComponentType, std::string> k_prim_paths = {
    {ComponentType::FluidDomain, "/FluidDomain"},
    {ComponentType::Envelope,    "/Envelope"},
};

// Maps each component type to its visual material style.
const std::unordered_map<ComponentType, MaterialStyle> k_styles = {
    {ComponentType::FluidDomain, {GfVec3f(0.2f, 0.5f, 0.8f), 0.3f}},
    {ComponentType::Envelope,    {GfVec3f(0.2f, 0.8f, 0.2f), 0.75f}},
};

std::string prim_path_for(ComponentType type) {
    auto it = k_prim_paths.find(type);
    return it != k_prim_paths.end() ? it->second : "";
}

std::optional<MaterialStyle> style_for(ComponentType type) {
    auto it = k_styles.find(type);
    return it != k_styles.end() ? std::optional{it->second} : std::nullopt;
}

} // namespace

StageComposer::StageComposer(const std::string& root_path)
    : root_path_(root_path) {}

StageComposer::~StageComposer() = default;

void StageComposer::add_component(ComponentType type, UsdStageRefPtr stage) {
    components_.emplace_back(type, stage);
}

void StageComposer::apply_material(ComponentType type,
                                   UsdStageRefPtr stage,
                                   const std::string& mesh_prim_path) const {
    auto style = style_for(type);
    if (!style) return;

    const std::string mat_path    = mesh_prim_path + "_Material";
    const std::string shader_path = mat_path + "/PreviewSurface";

    auto material = UsdShadeMaterial::Define(stage, SdfPath(mat_path));
    auto shader   = UsdShadeShader::Define(stage, SdfPath(shader_path));

    shader.CreateIdAttr(VtValue(TfToken("UsdPreviewSurface")));
    shader.CreateInput(TfToken("diffuseColor"), SdfValueTypeNames->Color3f)
          .Set(style->color);
    shader.CreateInput(TfToken("opacity"), SdfValueTypeNames->Float)
          .Set(style->opacity);

    auto surface_output = shader.CreateOutput(TfToken("surface"),
                                              SdfValueTypeNames->Token);
    material.CreateSurfaceOutput().ConnectToSource(surface_output);

    auto mesh_prim  = stage->GetPrimAtPath(SdfPath(mesh_prim_path));
    auto binding_api = UsdShadeMaterialBindingAPI::Apply(mesh_prim);
    binding_api.Bind(material);
}

bool StageComposer::write() const {
    // Apply materials and save all non-InputGeometry component layers
    for (const auto& [type, stage] : components_) {
        if (type != ComponentType::InputGeometry) {
            apply_material(type, stage, prim_path_for(type));
            stage->GetRootLayer()->Save();
        }
    }

    // Build sublayer stack sorted strongest first (highest enum value first)
    auto sorted = components_;
    std::sort(sorted.begin(), sorted.end(),
              [](const auto& a, const auto& b) {
                  return a.first > b.first;
              });

    auto root_layer = SdfLayer::CreateNew(root_path_);
    if (!root_layer) return false;

    for (const auto& [type, stage] : sorted) {
        root_layer->GetSubLayerPaths().push_back(
            stage->GetRootLayer()->GetIdentifier());
    }

    return root_layer->Save();
}

} // namespace ufd
