#include <ufd/StageReader.h>

#include <gtest/gtest.h>

TEST(StageReaderTest, OpenInvalidPathReturnsFalse) {
    ufd::StageReader reader;
    EXPECT_FALSE(reader.open("/nonexistent/path.usd"));
}

TEST(StageReaderTest, CollectMeshesOnClosedStageReturnsEmpty) {
    ufd::StageReader reader;
    auto meshes = reader.collect_meshes();
    EXPECT_TRUE(meshes.empty());
}
