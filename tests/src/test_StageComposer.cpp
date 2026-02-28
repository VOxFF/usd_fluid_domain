#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>
#include <ufd/DomainGenerator.h>
#include <ufd/StageComposer.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>

#include <gtest/gtest.h>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";

static const std::string DOMAIN_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_test_domain.usda";
static const std::string ROOT_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_test_root.usda";

// Helper: open box.usda as input stage
static UsdStageRefPtr make_input_stage() {
    return UsdStage::Open(BOX_USD);
}

// Helper: create a domain stage with /FluidDomain mesh from box.usda bounds
static UsdStageRefPtr make_domain_stage() {
    ufd::StageReader reader;
    reader.open(BOX_USD);
    ufd::SurfaceExtractor extractor;
    auto bounds = extractor.compute_bounding_box(
        extractor.extract(reader.collect_meshes()));

    ufd::DomainConfig config;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateNew(DOMAIN_USD);
    generator.generate(stage, bounds);
    return stage;
}

// Helper: build and write a composed stage, returns {input, domain} stages
static std::pair<UsdStageRefPtr, UsdStageRefPtr> make_composed() {
    auto input_stage  = make_input_stage();
    auto domain_stage = make_domain_stage();

    ufd::StageComposer composer(ROOT_USD);
    composer.add_component(ufd::ComponentType::InputGeometry, input_stage);
    composer.add_component(ufd::ComponentType::FluidDomain,   domain_stage);
    composer.write();

    return {input_stage, domain_stage};
}

// ---- Root layer ----

TEST(StageComposerTest, WriteCreatesRootLayerFile) {
    make_composed();

    auto root_layer = SdfLayer::FindOrOpen(ROOT_USD);

    EXPECT_TRUE(root_layer);
}

TEST(StageComposerTest, RootLayerHasTwoSublayers) {
    make_composed();

    auto root_layer = SdfLayer::FindOrOpen(ROOT_USD);

    EXPECT_EQ(root_layer->GetSubLayerPaths().size(), 2);
}

TEST(StageComposerTest, FluidDomainSublayeredBeforeInputGeometry) {
    auto [input_stage, domain_stage] = make_composed();

    auto root_layer = SdfLayer::FindOrOpen(ROOT_USD);
    auto sublayers  = root_layer->GetSubLayerPaths();

    EXPECT_EQ(sublayers[0], domain_stage->GetRootLayer()->GetIdentifier());
    EXPECT_EQ(sublayers[1], input_stage->GetRootLayer()->GetIdentifier());
}

// ---- FluidDomain material ----

TEST(StageComposerTest, FluidDomainMaterialPrimIsAuthored) {
    auto [input_stage, domain_stage] = make_composed();

    auto prim = domain_stage->GetPrimAtPath(SdfPath("/FluidDomain_Material"));

    EXPECT_TRUE(prim.IsValid());
}

TEST(StageComposerTest, FluidDomainMaterialHasCorrectColor) {
    auto [input_stage, domain_stage] = make_composed();

    auto shader = UsdShadeShader(domain_stage->GetPrimAtPath(
        SdfPath("/FluidDomain_Material/PreviewSurface")));
    GfVec3f color;
    shader.GetInput(TfToken("diffuseColor")).Get(&color);

    EXPECT_NEAR(color[0], 0.2f, 1e-5f);
    EXPECT_NEAR(color[1], 0.5f, 1e-5f);
    EXPECT_NEAR(color[2], 0.8f, 1e-5f);
}

TEST(StageComposerTest, FluidDomainMaterialHasCorrectOpacity) {
    auto [input_stage, domain_stage] = make_composed();

    auto shader = UsdShadeShader(domain_stage->GetPrimAtPath(
        SdfPath("/FluidDomain_Material/PreviewSurface")));
    float opacity = 0.0f;
    shader.GetInput(TfToken("opacity")).Get(&opacity);

    EXPECT_NEAR(opacity, 0.3f, 1e-5f);
}

TEST(StageComposerTest, FluidDomainMeshHasMaterialBinding) {
    auto [input_stage, domain_stage] = make_composed();

    auto mesh_prim = domain_stage->GetPrimAtPath(SdfPath("/FluidDomain"));
    auto binding   = UsdShadeMaterialBindingAPI(mesh_prim).GetDirectBinding();

    EXPECT_EQ(binding.GetMaterialPath(), SdfPath("/FluidDomain_Material"));
}

// ---- InputGeometry not modified ----

TEST(StageComposerTest, InputGeometryHasNoMaterialAuthored) {
    auto [input_stage, domain_stage] = make_composed();

    auto prim = input_stage->GetPrimAtPath(SdfPath("/FluidDomain_Material"));

    EXPECT_FALSE(prim.IsValid());
}
