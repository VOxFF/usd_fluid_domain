#include <ufd/EmitterBuilder.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/base/gf/range3d.h>
#include <pxr/base/gf/vec3f.h>

#include <gtest/gtest.h>

// Domain bounds matching box.usda: [0,10]^3
static const GfRange3d k_bounds(GfVec3d(0,0,0), GfVec3d(10,10,10));

// Helpers
static UsdGeomMesh emitter_mesh(UsdStageRefPtr stage, const std::string& name = "Emitter") {
    return UsdGeomMesh(stage->GetPrimAtPath(SdfPath("/" + name)));
}

static GfRange3d points_bbox(UsdStageRefPtr stage, const std::string& name = "Emitter") {
    VtVec3fArray pts;
    emitter_mesh(stage, name).GetPointsAttr().Get(&pts);
    GfRange3d bbox;
    for (const auto& p : pts)
        bbox.UnionWith(GfVec3d(p[0], p[1], p[2]));
    return bbox;
}

// ---- EmitterConfig defaults ----

TEST(EmitterBuilderTest, DefaultShapeIsDomainFace) {
    EXPECT_EQ(ufd::EmitterConfig{}.shape, ufd::EmitterShape::DomainFace);
}

TEST(EmitterBuilderTest, DefaultFaceIndexIsZero) {
    EXPECT_EQ(ufd::EmitterConfig{}.face_index, 0);
}

TEST(EmitterBuilderTest, DefaultDirectionIsAlongX) {
    auto d = ufd::EmitterConfig{}.direction;
    EXPECT_FLOAT_EQ(d[0], 1.0f);
    EXPECT_FLOAT_EQ(d[1], 0.0f);
    EXPECT_FLOAT_EQ(d[2], 0.0f);
}

// ---- build() return value ----

TEST(EmitterBuilderTest, BuildReturnsPrimPath) {
    auto stage = pxr::UsdStage::CreateInMemory();
    auto path  = ufd::EmitterBuilder().build(stage, k_bounds);
    EXPECT_EQ(path, "/Emitter");
}

TEST(EmitterBuilderTest, CustomNameReflectedInPrimPath) {
    ufd::EmitterConfig cfg;
    cfg.name = "Inlet";
    auto stage = pxr::UsdStage::CreateInMemory();
    auto path  = ufd::EmitterBuilder(cfg).build(stage, k_bounds);
    EXPECT_EQ(path, "/Inlet");
}

TEST(EmitterBuilderTest, CustomShapeReturnsEmptyPath) {
    ufd::EmitterConfig cfg;
    cfg.shape = ufd::EmitterShape::Custom;
    auto stage = pxr::UsdStage::CreateInMemory();
    EXPECT_TRUE(ufd::EmitterBuilder(cfg).build(stage, k_bounds).empty());
}

// ---- DomainFace ----

TEST(EmitterBuilderTest, DomainFaceCreatesFourPointQuad) {
    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder().build(stage, k_bounds);

    VtVec3fArray pts;
    emitter_mesh(stage).GetPointsAttr().Get(&pts);
    EXPECT_EQ(pts.size(), 4u);
}

TEST(EmitterBuilderTest, DomainFace0LiesOnXminPlane) {
    ufd::EmitterConfig cfg;
    cfg.face_index = 0;  // xmin
    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder(cfg).build(stage, k_bounds);

    auto bbox = points_bbox(stage);
    EXPECT_NEAR(bbox.GetMin()[0], 0.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[0], 0.0, 1e-5);  // all x == 0
    EXPECT_NEAR(bbox.GetMin()[1], 0.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[1], 10.0, 1e-5);
    EXPECT_NEAR(bbox.GetMin()[2], 0.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[2], 10.0, 1e-5);
}

TEST(EmitterBuilderTest, DomainFace1LiesOnXmaxPlane) {
    ufd::EmitterConfig cfg;
    cfg.face_index = 1;  // xmax
    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder(cfg).build(stage, k_bounds);

    auto bbox = points_bbox(stage);
    EXPECT_NEAR(bbox.GetMin()[0], 10.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[0], 10.0, 1e-5);
}

TEST(EmitterBuilderTest, DomainFace2LiesOnYminPlane) {
    ufd::EmitterConfig cfg;
    cfg.face_index = 2;  // ymin
    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder(cfg).build(stage, k_bounds);

    auto bbox = points_bbox(stage);
    EXPECT_NEAR(bbox.GetMin()[1], 0.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[1], 0.0, 1e-5);
}

TEST(EmitterBuilderTest, DomainFaceOutOfRangeClampedToValid) {
    ufd::EmitterConfig cfg;
    cfg.face_index = 99;
    auto stage = pxr::UsdStage::CreateInMemory();
    auto path  = ufd::EmitterBuilder(cfg).build(stage, k_bounds);
    EXPECT_FALSE(path.empty());  // clamped to face 5, not crashed
}

// ---- Quad ----

TEST(EmitterBuilderTest, QuadCreatesFourPoints) {
    ufd::EmitterConfig cfg;
    cfg.shape    = ufd::EmitterShape::Quad;
    cfg.size     = {4.0f, 6.0f};
    cfg.location = {5.0f, 5.0f, 5.0f};
    cfg.direction= {1.0f, 0.0f, 0.0f};

    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder(cfg).build(stage, k_bounds);

    VtVec3fArray pts;
    emitter_mesh(stage).GetPointsAttr().Get(&pts);
    EXPECT_EQ(pts.size(), 4u);
}

TEST(EmitterBuilderTest, QuadBBoxMatchesSize) {
    ufd::EmitterConfig cfg;
    cfg.shape    = ufd::EmitterShape::Quad;
    cfg.size     = {4.0f, 6.0f};
    cfg.location = {5.0f, 5.0f, 5.0f};
    cfg.direction= {1.0f, 0.0f, 0.0f};  // normal along X; quad spans Y/Z

    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder(cfg).build(stage, k_bounds);

    auto bbox = points_bbox(stage);
    EXPECT_NEAR(bbox.GetMax()[1] - bbox.GetMin()[1], 4.0, 1e-4);  // width in Y
    EXPECT_NEAR(bbox.GetMax()[2] - bbox.GetMin()[2], 6.0, 1e-4);  // height in Z
}

// ---- Circle ----

TEST(EmitterBuilderTest, CircleCreatesThirtyTwoPoints) {
    ufd::EmitterConfig cfg;
    cfg.shape    = ufd::EmitterShape::Circle;
    cfg.radius   = 3.0f;
    cfg.location = {5.0f, 5.0f, 5.0f};
    cfg.direction= {1.0f, 0.0f, 0.0f};

    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder(cfg).build(stage, k_bounds);

    VtVec3fArray pts;
    emitter_mesh(stage).GetPointsAttr().Get(&pts);
    EXPECT_EQ(pts.size(), 32u);
}

TEST(EmitterBuilderTest, CirclePointsLieAtCorrectRadius) {
    ufd::EmitterConfig cfg;
    cfg.shape    = ufd::EmitterShape::Circle;
    cfg.radius   = 3.0f;
    cfg.location = {5.0f, 5.0f, 5.0f};
    cfg.direction= {1.0f, 0.0f, 0.0f};

    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder(cfg).build(stage, k_bounds);

    VtVec3fArray pts;
    emitter_mesh(stage).GetPointsAttr().Get(&pts);
    for (const auto& p : pts) {
        float dy = p[1] - 5.0f;
        float dz = p[2] - 5.0f;
        EXPECT_NEAR(std::sqrt(dy*dy + dz*dz), 3.0f, 1e-4f);
    }
}

// ---- Solver attributes ----

TEST(EmitterBuilderTest, SolverAttributesWrittenToPrim) {
    ufd::EmitterConfig cfg;
    cfg.direction = {0.0f, 1.0f, 0.0f};
    cfg.magnitude = 2.5f;
    cfg.rate      = 50.0f;

    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder(cfg).build(stage, k_bounds);

    auto prim = stage->GetPrimAtPath(SdfPath("/Emitter"));
    ASSERT_TRUE(prim.IsValid());

    float mag, rate;
    GfVec3f dir;
    prim.GetAttribute(TfToken("emitter:direction")).Get(&dir);
    prim.GetAttribute(TfToken("emitter:magnitude")).Get(&mag);
    prim.GetAttribute(TfToken("emitter:rate"))     .Get(&rate);

    EXPECT_NEAR(dir[1], 1.0f, 1e-5f);
    EXPECT_NEAR(mag,    2.5f, 1e-5f);
    EXPECT_NEAR(rate,  50.0f, 1e-5f);
}

// ---- Arrow prims ----

TEST(EmitterBuilderTest, ArrowShaftPrimExists) {
    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder().build(stage, k_bounds);
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/EmitterShaft")).IsValid());
}

TEST(EmitterBuilderTest, ArrowConePrimExists) {
    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EmitterBuilder().build(stage, k_bounds);
    EXPECT_TRUE(stage->GetPrimAtPath(SdfPath("/EmitterCone")).IsValid());
}
