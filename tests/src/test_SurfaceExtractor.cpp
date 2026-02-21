#include <ufd/SurfaceExtractor.h>

#include <gtest/gtest.h>

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
