#include <ufd/SurfaceExtractor.h>

#include <gtest/gtest.h>

TEST(SurfaceExtractorTest, ExtractEmptyMeshListReturnsEmptySurface) {
    ufd::SurfaceExtractor extractor;
    auto surface = extractor.Extract({});
    EXPECT_TRUE(surface.points.empty());
    EXPECT_TRUE(surface.faceVertexCounts.empty());
    EXPECT_TRUE(surface.faceVertexIndices.empty());
}

TEST(SurfaceExtractorTest, BoundingBoxOfEmptySurfaceIsEmpty) {
    ufd::SurfaceExtractor extractor;
    ufd::SurfaceData empty;
    auto bbox = extractor.ComputeBoundingBox(empty);
    EXPECT_TRUE(bbox.IsEmpty());
}
