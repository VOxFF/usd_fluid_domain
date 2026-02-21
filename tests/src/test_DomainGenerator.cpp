#include <ufd/DomainGenerator.h>

#include <gtest/gtest.h>

TEST(DomainGeneratorTest, DefaultConfigUsesBox) {
    ufd::DomainConfig config;
    EXPECT_EQ(config.shape, ufd::DomainShape::Box);
}

TEST(DomainGeneratorTest, DefaultExtentMultiplierIsTen) {
    ufd::DomainConfig config;
    EXPECT_DOUBLE_EQ(config.extent_multiplier, 10.0);
}
