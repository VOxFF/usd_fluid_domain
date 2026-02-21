#include <ufd/StageReader.h>
#include <ufd/SurfaceExtractor.h>
#include <ufd/DomainGenerator.h>

#include <gtest/gtest.h>

static const std::string BOX_USD =
    std::string(TEST_RESOURCES_DIR) + "/box.usda";

TEST(DomainGeneratorTest, DefaultConfigUsesBox) {
    ufd::DomainConfig config;
    EXPECT_EQ(config.shape, ufd::DomainShape::Box);
}

TEST(DomainGeneratorTest, DefaultExtentMultiplierIsTen) {
    ufd::DomainConfig config;
    EXPECT_DOUBLE_EQ(config.extent_multiplier, 10.0);
}

TEST(DomainGeneratorTest, GenerateReturnsFluidDomainPath) {
    ufd::StageReader reader;
    reader.open(BOX_USD);
    auto meshes = reader.collect_meshes();

    ufd::SurfaceExtractor extractor;
    auto surface = extractor.extract(meshes);
    auto bounds = extractor.compute_bounding_box(surface);

    ufd::DomainConfig config;
    ufd::DomainGenerator generator(config);

    auto stage = pxr::UsdStage::CreateInMemory();
    auto path = generator.generate(stage, bounds);
    EXPECT_EQ(path, "/FluidDomain");
}
