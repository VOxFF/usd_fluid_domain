#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>
#include <ufd/EnvelopeBuilder.h>

#include <pxr/base/gf/range3d.h>
#include <pxr/base/gf/vec3f.h>

#include <gtest/gtest.h>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";
static const std::string BOX_X2_DISJOINT_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_disjoint.usda";
static const std::string BOX_X2_INTERSECTED_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_intersected.usda";

// Helper: compute AABB over a SurfaceData's points
static GfRange3d surface_bbox(const ufd::SurfaceData& s) {
    GfRange3d bbox;
    for (const auto& p : s.points)
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

    auto surface = ufd::EnvelopeBuilder(cfg).build(meshes);

    EXPECT_FALSE(surface.points.empty());
    EXPECT_FALSE(surface.face_vertex_counts.empty());
}

TEST(EnvelopeBuilderTest, SingleBoxEnvelopeBBoxContainsInput) {
    ufd::StageReader reader;
    reader.open(BOX_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 0.5;
    cfg.hole_threshold = 0.0;

    auto surface = ufd::EnvelopeBuilder(cfg).build(meshes);

    auto env_bb = surface_bbox(surface);
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

    auto surface = ufd::EnvelopeBuilder(cfg).build(meshes);

    EXPECT_FALSE(surface.points.empty());
    EXPECT_FALSE(surface.face_vertex_counts.empty());
}

TEST(EnvelopeBuilderTest, TwoDisjointBoxesEnvelopeBBoxContainsBothInputs) {
    ufd::StageReader reader;
    reader.open(BOX_X2_DISJOINT_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 0.5;
    cfg.hole_threshold = 0.0;

    auto surface = ufd::EnvelopeBuilder(cfg).build(meshes);

    auto env_bb = surface_bbox(surface);
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

    auto surface_open   = ufd::EnvelopeBuilder(cfg_open).build(meshes);
    auto surface_closed = ufd::EnvelopeBuilder(cfg_closed).build(meshes);

    EXPECT_NE(surface_open.points.size(), surface_closed.points.size());
}

// ---- Two intersected boxes ----

TEST(EnvelopeBuilderTest, IntersectedBoxesReturnsNonEmptySurface) {
    ufd::StageReader reader;
    reader.open(BOX_X2_INTERSECTED_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 0.5;
    cfg.hole_threshold = 0.0;

    auto surface = ufd::EnvelopeBuilder(cfg).build(meshes);

    EXPECT_FALSE(surface.points.empty());
    EXPECT_FALSE(surface.face_vertex_counts.empty());
}

TEST(EnvelopeBuilderTest, IntersectedBoxesEnvelopeBBoxContainsInput) {
    ufd::StageReader reader;
    reader.open(BOX_X2_INTERSECTED_USD);
    auto meshes = reader.collect_meshes();

    ufd::EnvelopeConfig cfg;
    cfg.voxel_size     = 0.5;
    cfg.hole_threshold = 0.0;

    auto surface = ufd::EnvelopeBuilder(cfg).build(meshes);

    auto env_bb = surface_bbox(surface);
    auto inp_bb = input_bbox(meshes);

    EXPECT_LE(env_bb.GetMin()[0], inp_bb.GetMin()[0] + 1e-3);
    EXPECT_LE(env_bb.GetMin()[1], inp_bb.GetMin()[1] + 1e-3);
    EXPECT_LE(env_bb.GetMin()[2], inp_bb.GetMin()[2] + 1e-3);
    EXPECT_GE(env_bb.GetMax()[0], inp_bb.GetMax()[0] - 1e-3);
    EXPECT_GE(env_bb.GetMax()[1], inp_bb.GetMax()[1] - 1e-3);
    EXPECT_GE(env_bb.GetMax()[2], inp_bb.GetMax()[2] - 1e-3);
}

// ---- Empty input ----

TEST(EnvelopeBuilderTest, EmptyMeshListReturnsEmptySurface) {
    auto surface = ufd::EnvelopeBuilder().build({});

    EXPECT_TRUE(surface.points.empty());
    EXPECT_TRUE(surface.face_vertex_counts.empty());
}
