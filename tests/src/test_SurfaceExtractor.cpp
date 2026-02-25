#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>

#include <gtest/gtest.h>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";
static const std::string BOX_X2_DISJOINT_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_disjoint.usda";
static const std::string BOX_X2_INTERSECTED_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_intersected.usda";

TEST(SurfaceExtractorTest, ExtractEmptyMeshListReturnsEmptySurface) {
    ufd::SurfaceExtractor extractor;
    auto surface = extractor.extract({});
    EXPECT_TRUE(surface.points.empty());
    EXPECT_TRUE(surface.face_vertex_counts.empty());
    EXPECT_TRUE(surface.face_vertex_indices.empty());
}

TEST(SurfaceExtractorTest, BoundingBoxOfEmptySurfaceIsEmpty) {
    ufd::SurfaceExtractor extractor;
    ufd::SurfaceData empty;
    auto bbox = extractor.compute_bounding_box(empty);
    EXPECT_TRUE(bbox.IsEmpty());
}

TEST(SurfaceExtractorTest, ExtractBoxMeshReturnsEightPoints) {
    ufd::StageReader reader;
    reader.open(BOX_USD);
    auto meshes = reader.collect_meshes();
    ASSERT_EQ(meshes.size(), 1);

    ufd::SurfaceExtractor extractor;
    auto surface = extractor.extract(meshes);
    EXPECT_EQ(surface.points.size(), 8);
    EXPECT_EQ(surface.face_vertex_counts.size(), 12);
}

TEST(SurfaceExtractorTest, BoundingBoxOfBoxMesh) {
    ufd::StageReader reader;
    reader.open(BOX_USD);
    auto meshes = reader.collect_meshes();

    ufd::SurfaceExtractor extractor;
    auto surface = extractor.extract(meshes);
    auto bbox = extractor.compute_bounding_box(surface);
    EXPECT_FALSE(bbox.IsEmpty());
}

// ---- box_x2_disjoint: two spatially separate boxes ----
// box1 at [0,10]^3, box2 translated by (11,11,11) -> [11,21]^3, no overlap.

TEST(SurfaceExtractorTest, ExtractDisjointMeshesReturnsSixteenPoints) {
    ufd::StageReader reader;
    reader.open(BOX_X2_DISJOINT_USD);
    auto meshes = reader.collect_meshes();
    ASSERT_EQ(meshes.size(), 2);

    ufd::SurfaceExtractor extractor;
    auto surface = extractor.extract(meshes);
    EXPECT_EQ(surface.points.size(), 16);
    EXPECT_EQ(surface.face_vertex_counts.size(), 24);
    EXPECT_EQ(surface.face_vertex_indices.size(), 72); // 24 tris * 3 verts
}

TEST(SurfaceExtractorTest, BoundingBoxOfDisjointMeshes) {
    ufd::StageReader reader;
    reader.open(BOX_X2_DISJOINT_USD);
    auto meshes = reader.collect_meshes();

    ufd::SurfaceExtractor extractor;
    auto surface = extractor.extract(meshes);
    auto bbox = extractor.compute_bounding_box(surface);

    // box1 at [0,10]^3, box2 translated by (11,11,11) -> [11,21]^3
    EXPECT_FALSE(bbox.IsEmpty());
    EXPECT_NEAR(bbox.GetMin()[0],  0.0, 1e-5);
    EXPECT_NEAR(bbox.GetMin()[1],  0.0, 1e-5);
    EXPECT_NEAR(bbox.GetMin()[2],  0.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[0], 21.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[1], 21.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[2], 21.0, 1e-5);
}

// ---- box_x2_intersected: two overlapping boxes ----
// box1 at [0,10]^3, box2 translated by (5,5,5) -> [5,15]^3, 5-unit overlap.

TEST(SurfaceExtractorTest, ExtractIntersectedMeshesReturnsSixteenPoints) {
    ufd::StageReader reader;
    reader.open(BOX_X2_INTERSECTED_USD);
    auto meshes = reader.collect_meshes();
    ASSERT_EQ(meshes.size(), 2);

    ufd::SurfaceExtractor extractor;
    auto surface = extractor.extract(meshes);
    EXPECT_EQ(surface.points.size(), 16);
    EXPECT_EQ(surface.face_vertex_counts.size(), 24);
    EXPECT_EQ(surface.face_vertex_indices.size(), 72);
}

TEST(SurfaceExtractorTest, BoundingBoxOfIntersectedMeshes) {
    ufd::StageReader reader;
    reader.open(BOX_X2_INTERSECTED_USD);
    auto meshes = reader.collect_meshes();

    ufd::SurfaceExtractor extractor;
    auto surface = extractor.extract(meshes);
    auto bbox = extractor.compute_bounding_box(surface);

    // box1 at [0,10]^3, box2 translated by (5,5,5) -> [5,15]^3, overlapping
    EXPECT_FALSE(bbox.IsEmpty());
    EXPECT_NEAR(bbox.GetMin()[0],  0.0, 1e-5);
    EXPECT_NEAR(bbox.GetMin()[1],  0.0, 1e-5);
    EXPECT_NEAR(bbox.GetMin()[2],  0.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[0], 15.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[1], 15.0, 1e-5);
    EXPECT_NEAR(bbox.GetMax()[2], 15.0, 1e-5);
}
