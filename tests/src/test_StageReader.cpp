#include <ufd/StageReader.h>

#include <gtest/gtest.h>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";

TEST(StageReaderTest, OpenInvalidPathReturnsFalse) {
    ufd::StageReader reader;
    EXPECT_FALSE(reader.open("/nonexistent/path.usd"));
}

TEST(StageReaderTest, CollectMeshesOnClosedStageReturnsEmpty) {
    ufd::StageReader reader;
    auto meshes = reader.collect_meshes();
    EXPECT_TRUE(meshes.empty());
}

TEST(StageReaderTest, OpenValidStageSucceeds) {
    ufd::StageReader reader;
    EXPECT_TRUE(reader.open(BOX_USD));
    EXPECT_TRUE(reader.get_stage());
}

TEST(StageReaderTest, CollectMeshesFindsBoxMesh) {
    ufd::StageReader reader;
    reader.open(BOX_USD);
    auto meshes = reader.collect_meshes();
    EXPECT_EQ(meshes.size(), 1);
}
