#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>
#include <ufd/DomainGenerator.h>

#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/base/gf/range3d.h>

#include <gtest/gtest.h>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";

// Helper: load box.usda and return its world-space bounding box [0,10]^3
static GfRange3d box_bounds() {
    ufd::StageReader reader;
    reader.open(BOX_USD);
    ufd::SurfaceExtractor extractor;
    auto surface = extractor.extract(reader.collect_meshes());
    return extractor.compute_bounding_box(surface);
}

// Helper: retrieve the generated domain mesh from a stage
static UsdGeomMesh domain_mesh(UsdStageRefPtr stage, const std::string& path) {
    return UsdGeomMesh(stage->GetPrimAtPath(SdfPath(path)));
}

// Helper: compute AABB over a mesh's points attribute
static GfRange3d points_bbox(const UsdGeomMesh& mesh) {
    VtVec3fArray pts;
    mesh.GetPointsAttr().Get(&pts);
    GfRange3d bbox;
    for (const auto& p : pts)
        bbox.UnionWith(GfVec3d(p[0], p[1], p[2]));
    return bbox;
}

// ---- DomainConfig defaults ----

TEST(DomainGeneratorTest, DefaultConfigUsesBox) {
    ufd::DomainConfig config;

    EXPECT_EQ(config.shape, ufd::DomainShape::Box);
}

TEST(DomainGeneratorTest, DefaultExtentMultiplierIsTen) {
    ufd::DomainConfig config;

    EXPECT_DOUBLE_EQ(config.extent_multiplier, 10.0);
}

TEST(DomainGeneratorTest, DefaultFlowDirectionIsAlongX) {
    ufd::DomainConfig config;

    EXPECT_DOUBLE_EQ(config.flow_direction[0], 1.0);
    EXPECT_DOUBLE_EQ(config.flow_direction[1], 0.0);
    EXPECT_DOUBLE_EQ(config.flow_direction[2], 0.0);
}

TEST(DomainGeneratorTest, DefaultCylinderSegmentsIsThirtySix) {
    ufd::DomainConfig config;

    EXPECT_EQ(config.cylinder_segments, 36);
}

TEST(DomainGeneratorTest, FlowDirectionIsNormalized) {
    ufd::DomainConfig config;
    config.flow_direction = GfVec3d(2.0, 0.0, 0.0);
    config.shape = ufd::DomainShape::Cylinder;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();

    EXPECT_NO_FATAL_FAILURE(generator.generate(stage, box_bounds()));
}

// ---- generate() return value ----

TEST(DomainGeneratorTest, GenerateReturnsFluidDomainPath) {
    ufd::DomainConfig config;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());

    EXPECT_EQ(path, "/FluidDomain");
}

// ---- Box domain ----
// object bounds [0,10]^3, centroid (5,5,5), size (10,10,10)
// half_size = size * 10 * 0.5 = (50,50,50)
// domain: [-45,55]^3

TEST(DomainGeneratorTest, BoxDomainPrimIsDefinedOnStage) {
    ufd::DomainConfig config;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());

    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath(path)).IsValid());
}

TEST(DomainGeneratorTest, BoxDomainHasEightPoints) {
    ufd::DomainConfig config;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());
    VtVec3fArray pts;
    domain_mesh(stage, path).GetPointsAttr().Get(&pts);

    EXPECT_EQ(pts.size(), 8);
}

TEST(DomainGeneratorTest, BoxDomainHasSixFaces) {
    ufd::DomainConfig config;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());
    VtIntArray counts;
    domain_mesh(stage, path).GetFaceVertexCountsAttr().Get(&counts);

    EXPECT_EQ(counts.size(), 6);
}

TEST(DomainGeneratorTest, BoxDomainExtentsCorrect) {
    ufd::DomainConfig config;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());
    auto bbox = points_bbox(domain_mesh(stage, path));

    EXPECT_NEAR(bbox.GetMin()[0], -45.0, 1e-4);
    EXPECT_NEAR(bbox.GetMin()[1], -45.0, 1e-4);
    EXPECT_NEAR(bbox.GetMin()[2], -45.0, 1e-4);
    EXPECT_NEAR(bbox.GetMax()[0],  55.0, 1e-4);
    EXPECT_NEAR(bbox.GetMax()[1],  55.0, 1e-4);
    EXPECT_NEAR(bbox.GetMax()[2],  55.0, 1e-4);
}

TEST(DomainGeneratorTest, BoxDomainOriginOffsetShiftsExtents) {
    ufd::DomainConfig config;
    config.origin_offset = GfVec3d(10.0, 0.0, 0.0);
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());
    auto bbox = points_bbox(domain_mesh(stage, path));

    EXPECT_NEAR(bbox.GetMin()[0], -35.0, 1e-4); // -45 + 10
    EXPECT_NEAR(bbox.GetMax()[0],  65.0, 1e-4); //  55 + 10
    EXPECT_NEAR(bbox.GetMin()[1], -45.0, 1e-4);
    EXPECT_NEAR(bbox.GetMax()[1],  55.0, 1e-4);
}

// ---- Cylinder domain ----
// flow=(1,0,0), half_length = 10*0.5*10 = 50
// perp corners (0,±5,±5) -> radius = sqrt(50)*10 ≈ 70.711
// points: 2*36+2 = 74,  faces: 3*36 = 108

TEST(DomainGeneratorTest, CylinderDomainPrimIsDefinedOnStage) {
    ufd::DomainConfig config;
    config.shape = ufd::DomainShape::Cylinder;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());

    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath(path)).IsValid());
}

TEST(DomainGeneratorTest, CylinderDomainPointCount) {
    ufd::DomainConfig config;
    config.shape = ufd::DomainShape::Cylinder;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());
    VtVec3fArray pts;
    domain_mesh(stage, path).GetPointsAttr().Get(&pts);

    EXPECT_EQ(pts.size(), 2 * 36 + 2); // 74
}

TEST(DomainGeneratorTest, CylinderDomainFaceCount) {
    ufd::DomainConfig config;
    config.shape = ufd::DomainShape::Cylinder;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());
    VtIntArray counts;
    domain_mesh(stage, path).GetFaceVertexCountsAttr().Get(&counts);

    EXPECT_EQ(counts.size(), 3 * 36); // 108: N side quads + 2N cap triangles
}

TEST(DomainGeneratorTest, CylinderDomainAlongFlowExtentCorrect) {
    ufd::DomainConfig config;
    config.shape = ufd::DomainShape::Cylinder;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());
    auto bbox = points_bbox(domain_mesh(stage, path));

    // centroid X=5, half_length=50 -> [-45, 55]
    EXPECT_NEAR(bbox.GetMin()[0], 5.0 - 50.0, 1e-4);
    EXPECT_NEAR(bbox.GetMax()[0], 5.0 + 50.0, 1e-4);
}

TEST(DomainGeneratorTest, CylinderDomainCrossFlowRadiusCorrect) {
    ufd::DomainConfig config;
    config.shape = ufd::DomainShape::Cylinder;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());
    auto bbox = points_bbox(domain_mesh(stage, path));

    const double r = std::sqrt(50.0) * 10.0;
    EXPECT_NEAR(bbox.GetMin()[1], 5.0 - r, 1e-3);
    EXPECT_NEAR(bbox.GetMax()[1], 5.0 + r, 1e-3);
    EXPECT_NEAR(bbox.GetMin()[2], 5.0 - r, 1e-3);
    EXPECT_NEAR(bbox.GetMax()[2], 5.0 + r, 1e-3);
}

TEST(DomainGeneratorTest, CylinderSegmentsParameterRespected) {
    ufd::DomainConfig config;
    config.shape = ufd::DomainShape::Cylinder;
    config.cylinder_segments = 16;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, box_bounds());
    VtVec3fArray pts;
    domain_mesh(stage, path).GetPointsAttr().Get(&pts);

    EXPECT_EQ(pts.size(), 2 * 16 + 2); // 34
}
