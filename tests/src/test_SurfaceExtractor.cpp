#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>

#include <gtest/gtest.h>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";

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
