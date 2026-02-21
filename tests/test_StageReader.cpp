#include <gtest/gtest.h>
#include "StageReader.h"

TEST(StageReaderTest, OpenInvalidPathReturnsFalse) {
    ufd::StageReader reader;
    EXPECT_FALSE(reader.Open("/nonexistent/path.usd"));
}

TEST(StageReaderTest, CollectMeshesOnClosedStageReturnsEmpty) {
    ufd::StageReader reader;
    auto meshes = reader.CollectMeshes();
    EXPECT_TRUE(meshes.empty());
}
