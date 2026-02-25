#include <ufd/StageReader.h>

#include <gtest/gtest.h>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";
static const std::string BOX_X2_DISJOINT_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_disjoint.usda";
static const std::string BOX_X2_INTERSECTED_USD =
    std::string(TEST_RESOURCES_DIR) + "/box_x2_intersected.usda";

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

TEST(StageReaderTest, OpenDisjointStageSucceeds) {
    ufd::StageReader reader;
    EXPECT_TRUE(reader.open(BOX_X2_DISJOINT_USD));
    EXPECT_TRUE(reader.get_stage());
}

TEST(StageReaderTest, CollectMeshesFromDisjointFindsTwoMeshes) {
    ufd::StageReader reader;
    reader.open(BOX_X2_DISJOINT_USD);
    auto meshes = reader.collect_meshes();
    EXPECT_EQ(meshes.size(), 2);
}

TEST(StageReaderTest, OpenIntersectedStageSucceeds) {
    ufd::StageReader reader;
    EXPECT_TRUE(reader.open(BOX_X2_INTERSECTED_USD));
    EXPECT_TRUE(reader.get_stage());
}

TEST(StageReaderTest, CollectMeshesFromIntersectedFindsTwoMeshes) {
    ufd::StageReader reader;
    reader.open(BOX_X2_INTERSECTED_USD);
    auto meshes = reader.collect_meshes();
    EXPECT_EQ(meshes.size(), 2);
}
