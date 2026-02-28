#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>
#include <ufd/EnvelopeBuilder.h>

#include <pxr/usd/usd/stage.h>
#include <pxr/usd/usdGeom/mesh.h>
#include <pxr/usd/sdf/path.h>
#include <pxr/base/gf/range3d.h>
#include <pxr/base/gf/vec3f.h>

#include <gtest/gtest.h>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";
static const std::string BOX_X2_DISJOINT_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_disjoint.usda";
static const std::string BOX_X2_INTERSECTED_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_intersected.usda";

// Helper: get /Envelope mesh from an in-memory stage
static UsdGeomMesh envelope_mesh(UsdStageRefPtr stage) {
    return UsdGeomMesh(stage->GetPrimAtPath(SdfPath("/Envelope")));
}

// Helper: compute AABB over /Envelope mesh points
static GfRange3d surface_bbox(UsdStageRefPtr stage) {
    VtVec3fArray pts;
    envelope_mesh(stage).GetPointsAttr().Get(&pts);
    GfRange3d bbox;
    for (const auto& p : pts)
        bbox.UnionWith(GfVec3d(p[0], p[1], p[2]));
    return bbox;
}

// Helper: compute AABB over the world-space input meshes
static GfRange3d input_bbox(const std::vector<UsdGeomMesh>& meshes) {
    ufd::SurfaceExtractor ext;
    return ext.compute_bounding_box(ext.extract(meshes));
}

// ---- Single box ----

TEST(EnvelopeBuilderTest, SingleBoxReturnsNonEmptySurface) {
    ufd::StageReader reader;
    reader.open(BOX_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 1.0;
    cfg.hole_threshold = 0.0;

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path  = ufd::EnvelopeBuilder(cfg).build(stage, meshes);

    VtVec3fArray pts;
    VtIntArray counts;
    envelope_mesh(stage).GetPointsAttr().Get(&pts);
    envelope_mesh(stage).GetFaceVertexCountsAttr().Get(&counts);

    EXPECT_FALSE(pts.empty());
    EXPECT_FALSE(counts.empty());
}

TEST(EnvelopeBuilderTest, SingleBoxEnvelopeBBoxContainsInput) {
    ufd::StageReader reader;
    reader.open(BOX_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 0.5;
    cfg.hole_threshold = 0.0;

    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EnvelopeBuilder(cfg).build(stage, meshes);

    auto env_bb = surface_bbox(stage);
    auto inp_bb = input_bbox(meshes);

    // Envelope must fully wrap the input (surface is at voxel boundary outside)
    EXPECT_LE(env_bb.GetMin()[0], inp_bb.GetMin()[0] + 1e-3);
    EXPECT_LE(env_bb.GetMin()[1], inp_bb.GetMin()[1] + 1e-3);
    EXPECT_LE(env_bb.GetMin()[2], inp_bb.GetMin()[2] + 1e-3);
    EXPECT_GE(env_bb.GetMax()[0], inp_bb.GetMax()[0] - 1e-3);
    EXPECT_GE(env_bb.GetMax()[1], inp_bb.GetMax()[1] - 1e-3);
    EXPECT_GE(env_bb.GetMax()[2], inp_bb.GetMax()[2] - 1e-3);
}

// ---- Two disjoint boxes ----

TEST(EnvelopeBuilderTest, TwoDisjointBoxesReturnsNonEmptySurface) {
    ufd::StageReader reader;
    reader.open(BOX_X2_DISJOINT_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 1.0;
    cfg.hole_threshold = 0.0;

    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EnvelopeBuilder(cfg).build(stage, meshes);

    VtVec3fArray pts;
    VtIntArray counts;
    envelope_mesh(stage).GetPointsAttr().Get(&pts);
    envelope_mesh(stage).GetFaceVertexCountsAttr().Get(&counts);

    EXPECT_FALSE(pts.empty());
    EXPECT_FALSE(counts.empty());
}

TEST(EnvelopeBuilderTest, TwoDisjointBoxesEnvelopeBBoxContainsBothInputs) {
    ufd::StageReader reader;
    reader.open(BOX_X2_DISJOINT_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 0.5;
    cfg.hole_threshold = 0.0;

    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EnvelopeBuilder(cfg).build(stage, meshes);

    auto env_bb = surface_bbox(stage);
    auto inp_bb = input_bbox(meshes);

    EXPECT_LE(env_bb.GetMin()[0], inp_bb.GetMin()[0] + 1e-3);
    EXPECT_LE(env_bb.GetMin()[1], inp_bb.GetMin()[1] + 1e-3);
    EXPECT_LE(env_bb.GetMin()[2], inp_bb.GetMin()[2] + 1e-3);
    EXPECT_GE(env_bb.GetMax()[0], inp_bb.GetMax()[0] - 1e-3);
    EXPECT_GE(env_bb.GetMax()[1], inp_bb.GetMax()[1] - 1e-3);
    EXPECT_GE(env_bb.GetMax()[2], inp_bb.GetMax()[2] - 1e-3);
}

TEST(EnvelopeBuilderTest, ClosingBridgesGapBetweenDisjointBoxes) {
    // The two boxes are 1 world unit apart (box[0,10] and box[11,21]).
    // A hole_threshold of 1.0 should bridge the gap, producing a different
    // (typically larger) mesh than no closing.
    ufd::StageReader reader;
    reader.open(BOX_X2_DISJOINT_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg_open;
    cfg_open.voxel_size     = 0.5;
    cfg_open.hole_threshold = 0.0;

    ufd::EnvelopeConfig cfg_closed;
    cfg_closed.voxel_size     = 0.5;
    cfg_closed.hole_threshold = 1.0;

    auto stage_open   = pxr::UsdStage::CreateInMemory();
    auto stage_closed = pxr::UsdStage::CreateInMemory();
    ufd::EnvelopeBuilder(cfg_open).build(stage_open, meshes);
    ufd::EnvelopeBuilder(cfg_closed).build(stage_closed, meshes);

    VtVec3fArray pts_open, pts_closed;
    envelope_mesh(stage_open).GetPointsAttr().Get(&pts_open);
    envelope_mesh(stage_closed).GetPointsAttr().Get(&pts_closed);

    EXPECT_NE(pts_open.size(), pts_closed.size());
}

// ---- Two intersected boxes ----

TEST(EnvelopeBuilderTest, IntersectedBoxesReturnsNonEmptySurface) {
    ufd::StageReader reader;
    reader.open(BOX_X2_INTERSECTED_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 0.5;
    cfg.hole_threshold = 0.0;

    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EnvelopeBuilder(cfg).build(stage, meshes);

    VtVec3fArray pts;
    VtIntArray counts;
    envelope_mesh(stage).GetPointsAttr().Get(&pts);
    envelope_mesh(stage).GetFaceVertexCountsAttr().Get(&counts);

    EXPECT_FALSE(pts.empty());
    EXPECT_FALSE(counts.empty());
}

TEST(EnvelopeBuilderTest, IntersectedBoxesEnvelopeBBoxContainsInput) {
    ufd::StageReader reader;
    reader.open(BOX_X2_INTERSECTED_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 0.5;
    cfg.hole_threshold = 0.0;

    auto stage = pxr::UsdStage::CreateInMemory();
    ufd::EnvelopeBuilder(cfg).build(stage, meshes);

    auto env_bb = surface_bbox(stage);
    auto inp_bb = input_bbox(meshes);

    EXPECT_LE(env_bb.GetMin()[0], inp_bb.GetMin()[0] + 1e-3);
    EXPECT_LE(env_bb.GetMin()[1], inp_bb.GetMin()[1] + 1e-3);
    EXPECT_LE(env_bb.GetMin()[2], inp_bb.GetMin()[2] + 1e-3);
    EXPECT_GE(env_bb.GetMax()[0], inp_bb.GetMax()[0] - 1e-3);
    EXPECT_GE(env_bb.GetMax()[1], inp_bb.GetMax()[1] - 1e-3);
    EXPECT_GE(env_bb.GetMax()[2], inp_bb.GetMax()[2] - 1e-3);
}

// ---- Empty input ----

TEST(EnvelopeBuilderTest, EmptyMeshListReturnsEmptyPath) {
    auto stage = pxr::UsdStage::CreateInMemory();
    auto path  = ufd::EnvelopeBuilder().build(stage, {});

    EXPECT_TRUE(path.empty());
}
