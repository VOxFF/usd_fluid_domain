#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>
#include <ufd/DomainBuilder.h>
#include <ufd/EnvelopeBuilder.h>
#include <ufd/StageComposer.h>

#include <pxr/usd/sdf/layer.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/usdShade/materialBindingAPI.h>
#include <pxr/usd/usdShade/shader.h>
#include <pxr/base/gf/range3d.h>
#include <pxr/base/gf/vec3f.h>
#include <pxr/base/tf/token.h>

#include <gtest/gtest.h>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";
static const std::string BOX_X2_DISJOINT_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_disjoint.usda";
static const std::string BOX_X2_INTERSECTED_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_intersected.usda";

static const std::string DOMAIN_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_test_domain.usda";
static const std::string ENVELOPE_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_test_envelope.usda";
static const std::string ROOT_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_test_root.usda";
static const std::string DISJOINT_DOMAIN_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_disjoint_test_domain.usda";
static const std::string DISJOINT_ENVELOPE_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_disjoint_test_envelope.usda";
static const std::string DISJOINT_ROOT_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_disjoint_test_root.usda";
static const std::string INTERSECTED_DOMAIN_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_intersected_test_domain.usda";
static const std::string INTERSECTED_ENVELOPE_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_intersected_test_envelope.usda";
static const std::string INTERSECTED_ROOT_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_intersected_test_root.usda";

// Helper: compute AABB over a mesh prim's points on a stage
static GfRange3d points_bbox(UsdStageRefPtr stage, const std::string& path) {
    VtVec3fArray pts;
    UsdGeomMesh(stage->GetPrimAtPath(SdfPath(path))).GetPointsAttr().Get(&pts);
    GfRange3d bbox;
    for (const auto& p : pts)
        bbox.UnionWith(GfVec3d(p[0], p[1], p[2]));
    return bbox;
}

// Helper: build an envelope stage from already-loaded meshes (source stage kept
// alive by the caller).
static UsdStageRefPtr make_envelope_stage(const std::vector<UsdGeomMesh>& meshes,
                                          const std::string& output_path) {
    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 1.0;
    cfg.hole_threshold = 0.0;
    auto stage = pxr::UsdStage::CreateNew(output_path);
    ufd::EnvelopeBuilder(cfg).build(stage, meshes);
    return stage;
}

// Helper: open BOX_USD once, build all three component stages, compose, and
// return {input, domain, envelope}.
static std::tuple<UsdStageRefPtr, UsdStageRefPtr, UsdStageRefPtr> make_composed() {
    ufd::StageReader reader;
    reader.open(BOX_USD);
    auto meshes = reader.collect_meshes();

    ufd::SurfaceExtractor extractor;
    auto bounds = extractor.compute_bounding_box(extractor.extract(meshes));

    auto domain_stage   = pxr::UsdStage::CreateNew(DOMAIN_USD);
    ufd::DomainBuilder(ufd::DomainConfig{}).build(domain_stage, bounds);

    auto envelope_stage = make_envelope_stage(meshes, ENVELOPE_USD);

    ufd::StageComposer composer(ROOT_USD);
    composer.add_component(ufd::ComponentType::InputGeometry, reader.get_stage());
    composer.add_component(ufd::ComponentType::FluidDomain,   domain_stage);
    composer.add_component(ufd::ComponentType::Envelope,      envelope_stage);
    composer.write();

    return {reader.get_stage(), domain_stage, envelope_stage};
}

// ---- Root layer ----

TEST(StageComposerTest, WriteCreatesRootLayerFile) {
    make_composed();

    auto root_layer = SdfLayer::FindOrOpen(ROOT_USD);

    EXPECT_TRUE(root_layer);
}

TEST(StageComposerTest, RootLayerHasThreeSublayers) {
    make_composed();

    auto root_layer = SdfLayer::FindOrOpen(ROOT_USD);

    EXPECT_EQ(root_layer->GetSubLayerPaths().size(), 3);
}

TEST(StageComposerTest, SublayerOrderIsEnvelopeThenDomainThenInput) {
    auto [input_stage, domain_stage, envelope_stage] = make_composed();

    auto root_layer = SdfLayer::FindOrOpen(ROOT_USD);
    auto sublayers  = root_layer->GetSubLayerPaths();

    EXPECT_EQ(sublayers[0], envelope_stage->GetRootLayer()->GetIdentifier());
    EXPECT_EQ(sublayers[1], domain_stage->GetRootLayer()->GetIdentifier());
    EXPECT_EQ(sublayers[2], input_stage->GetRootLayer()->GetIdentifier());
}

// ---- FluidDomain material ----

TEST(StageComposerTest, FluidDomainMaterialPrimIsAuthored) {
    auto [input_stage, domain_stage, envelope_stage] = make_composed();

    auto prim = domain_stage->GetPrimAtPath(SdfPath("/FluidDomain_Material"));

    EXPECT_TRUE(prim.IsValid());
}

TEST(StageComposerTest, FluidDomainMaterialHasCorrectColor) {
    auto [input_stage, domain_stage, envelope_stage] = make_composed();

    auto shader = UsdShadeShader(domain_stage->GetPrimAtPath(
        SdfPath("/FluidDomain_Material/PreviewSurface")));
    GfVec3f color;
    shader.GetInput(TfToken("diffuseColor")).Get(&color);

    EXPECT_NEAR(color[0], 0.2f, 1e-5f);
    EXPECT_NEAR(color[1], 0.5f, 1e-5f);
    EXPECT_NEAR(color[2], 0.8f, 1e-5f);
}

TEST(StageComposerTest, FluidDomainMaterialHasCorrectOpacity) {
    auto [input_stage, domain_stage, envelope_stage] = make_composed();

    auto shader = UsdShadeShader(domain_stage->GetPrimAtPath(
        SdfPath("/FluidDomain_Material/PreviewSurface")));
    float opacity = 0.0f;
    shader.GetInput(TfToken("opacity")).Get(&opacity);

    EXPECT_NEAR(opacity, 0.3f, 1e-5f);
}

TEST(StageComposerTest, FluidDomainMeshHasMaterialBinding) {
    auto [input_stage, domain_stage, envelope_stage] = make_composed();

    auto mesh_prim = domain_stage->GetPrimAtPath(SdfPath("/FluidDomain"));
    auto binding   = UsdShadeMaterialBindingAPI(mesh_prim).GetDirectBinding();

    EXPECT_EQ(binding.GetMaterialPath(), SdfPath("/FluidDomain_Material"));
}

// ---- Envelope material ----

TEST(StageComposerTest, EnvelopeMaterialPrimIsAuthored) {
    auto [input_stage, domain_stage, envelope_stage] = make_composed();

    auto prim = envelope_stage->GetPrimAtPath(SdfPath("/Envelope_Material"));

    EXPECT_TRUE(prim.IsValid());
}

TEST(StageComposerTest, EnvelopeMaterialHasCorrectColor) {
    auto [input_stage, domain_stage, envelope_stage] = make_composed();

    auto shader = UsdShadeShader(envelope_stage->GetPrimAtPath(
        SdfPath("/Envelope_Material/PreviewSurface")));
    GfVec3f color;
    shader.GetInput(TfToken("diffuseColor")).Get(&color);

    EXPECT_NEAR(color[0], 0.2f, 1e-5f);
    EXPECT_NEAR(color[1], 0.8f, 1e-5f);
    EXPECT_NEAR(color[2], 0.2f, 1e-5f);
}

TEST(StageComposerTest, EnvelopeMaterialHasCorrectOpacity) {
    auto [input_stage, domain_stage, envelope_stage] = make_composed();

    auto shader = UsdShadeShader(envelope_stage->GetPrimAtPath(
        SdfPath("/Envelope_Material/PreviewSurface")));
    float opacity = 0.0f;
    shader.GetInput(TfToken("opacity")).Get(&opacity);

    EXPECT_NEAR(opacity, 0.75f, 1e-5f);
}

TEST(StageComposerTest, EnvelopeMeshHasMaterialBinding) {
    auto [input_stage, domain_stage, envelope_stage] = make_composed();

    auto mesh_prim = envelope_stage->GetPrimAtPath(SdfPath("/Envelope"));
    auto binding   = UsdShadeMaterialBindingAPI(mesh_prim).GetDirectBinding();

    EXPECT_EQ(binding.GetMaterialPath(), SdfPath("/Envelope_Material"));
}

// ---- InputGeometry not modified ----

TEST(StageComposerTest, InputGeometryHasNoMaterialAuthored) {
    auto [input_stage, domain_stage, envelope_stage] = make_composed();

    auto prim = input_stage->GetPrimAtPath(SdfPath("/FluidDomain_Material"));

    EXPECT_FALSE(prim.IsValid());
}

// ---- Two disjoint boxes with tight margin (extent_multiplier=2) ----
// bounds [0,21]^3, centroid (10.5,10.5,10.5), size (21,21,21)
// half_size = 21*2.0*0.5 = 21 -> domain [-10.5, 31.5]^3

TEST(StageComposerTest, TwoDisjointBoxesTightMarginCreatesRootLayer) {
    ufd::StageReader reader;
    reader.open(BOX_X2_DISJOINT_USD);
    auto meshes = reader.collect_meshes();

    ufd::SurfaceExtractor extractor;
    auto bounds = extractor.compute_bounding_box(extractor.extract(meshes));

    ufd::DomainConfig config;
    config.extent_multiplier = 2.0;
    auto domain_stage   = pxr::UsdStage::CreateNew(DISJOINT_DOMAIN_USD);
    auto envelope_stage = make_envelope_stage(meshes, DISJOINT_ENVELOPE_USD);
    ufd::DomainBuilder(config).build(domain_stage, bounds);

    ufd::StageComposer composer(DISJOINT_ROOT_USD);
    composer.add_component(ufd::ComponentType::InputGeometry, reader.get_stage());
    composer.add_component(ufd::ComponentType::FluidDomain,   domain_stage);
    composer.add_component(ufd::ComponentType::Envelope,      envelope_stage);
    composer.write();

    EXPECT_TRUE(SdfLayer::FindOrOpen(DISJOINT_ROOT_USD));
}

TEST(StageComposerTest, TwoDisjointBoxesTightMarginDomainExtentsCorrect) {
    ufd::StageReader reader;
    reader.open(BOX_X2_DISJOINT_USD);
    auto meshes = reader.collect_meshes();

    ufd::SurfaceExtractor extractor;
    auto bounds = extractor.compute_bounding_box(extractor.extract(meshes));

    ufd::DomainConfig config;
    config.extent_multiplier = 2.0;
    auto domain_stage   = pxr::UsdStage::CreateNew(DISJOINT_DOMAIN_USD);
    auto envelope_stage = make_envelope_stage(meshes, DISJOINT_ENVELOPE_USD);
    ufd::DomainBuilder(config).build(domain_stage, bounds);

    ufd::StageComposer composer(DISJOINT_ROOT_USD);
    composer.add_component(ufd::ComponentType::InputGeometry, reader.get_stage());
    composer.add_component(ufd::ComponentType::FluidDomain,   domain_stage);
    composer.add_component(ufd::ComponentType::Envelope,      envelope_stage);
    composer.write();

    auto bbox = points_bbox(domain_stage, "/FluidDomain");

    EXPECT_NEAR(bbox.GetMin()[0], -10.5, 1e-4);
    EXPECT_NEAR(bbox.GetMin()[1], -10.5, 1e-4);
    EXPECT_NEAR(bbox.GetMin()[2], -10.5, 1e-4);
    EXPECT_NEAR(bbox.GetMax()[0],  31.5, 1e-4);
    EXPECT_NEAR(bbox.GetMax()[1],  31.5, 1e-4);
    EXPECT_NEAR(bbox.GetMax()[2],  31.5, 1e-4);
}

// ---- Intersected boxes, cylindrical domain, tight margin (extent_multiplier=2) ----
// bounds [0,15]^3, centroid (7.5,7.5,7.5), size (15,15,15)
// flow along X: half_length = 15*0.5*2.0 = 15 -> X: [-7.5, 22.5]
// perp corners (0,±7.5,±7.5) -> radius = sqrt(112.5)*2.0 ≈ 21.213

TEST(StageComposerTest, IntersectedBoxesCylinderDomainCreatesRootLayer) {
    ufd::StageReader reader;
    reader.open(BOX_X2_INTERSECTED_USD);
    auto meshes = reader.collect_meshes();

    ufd::SurfaceExtractor extractor;
    auto bounds = extractor.compute_bounding_box(extractor.extract(meshes));

    ufd::DomainConfig config;
    config.shape             = ufd::DomainShape::Cylinder;
    config.extent_multiplier = 2.0;
    auto domain_stage   = pxr::UsdStage::CreateNew(INTERSECTED_DOMAIN_USD);
    auto envelope_stage = make_envelope_stage(meshes, INTERSECTED_ENVELOPE_USD);
    ufd::DomainBuilder(config).build(domain_stage, bounds);

    ufd::StageComposer composer(INTERSECTED_ROOT_USD);
    composer.add_component(ufd::ComponentType::InputGeometry, reader.get_stage());
    composer.add_component(ufd::ComponentType::FluidDomain,   domain_stage);
    composer.add_component(ufd::ComponentType::Envelope,      envelope_stage);
    composer.write();

    EXPECT_TRUE(SdfLayer::FindOrOpen(INTERSECTED_ROOT_USD));
}

TEST(StageComposerTest, IntersectedBoxesCylinderDomainAlongFlowExtentCorrect) {
    ufd::StageReader reader;
    reader.open(BOX_X2_INTERSECTED_USD);
    auto meshes = reader.collect_meshes();

    ufd::SurfaceExtractor extractor;
    auto bounds = extractor.compute_bounding_box(extractor.extract(meshes));

    ufd::DomainConfig config;
    config.shape             = ufd::DomainShape::Cylinder;
    config.extent_multiplier = 2.0;
    auto domain_stage   = pxr::UsdStage::CreateNew(INTERSECTED_DOMAIN_USD);
    auto envelope_stage = make_envelope_stage(meshes, INTERSECTED_ENVELOPE_USD);
    ufd::DomainBuilder(config).build(domain_stage, bounds);

    ufd::StageComposer composer(INTERSECTED_ROOT_USD);
    composer.add_component(ufd::ComponentType::InputGeometry, reader.get_stage());
    composer.add_component(ufd::ComponentType::FluidDomain,   domain_stage);
    composer.add_component(ufd::ComponentType::Envelope,      envelope_stage);
    composer.write();

    auto bbox = points_bbox(domain_stage, "/FluidDomain");

    // Along flow (X): centroid 7.5 ± half_length 15
    EXPECT_NEAR(bbox.GetMin()[0],  -7.5, 1e-3);
    EXPECT_NEAR(bbox.GetMax()[0],  22.5, 1e-3);

    // Cross flow (Y/Z): centroid 7.5 ± radius sqrt(112.5)*2
    const double r = std::sqrt(112.5) * 2.0;
    EXPECT_NEAR(bbox.GetMin()[1], 7.5 - r, 1e-3);
    EXPECT_NEAR(bbox.GetMax()[1], 7.5 + r, 1e-3);
    EXPECT_NEAR(bbox.GetMin()[2], 7.5 - r, 1e-3);
    EXPECT_NEAR(bbox.GetMax()[2], 7.5 + r, 1e-3);
}
