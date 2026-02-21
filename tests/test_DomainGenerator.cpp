#include <gtest/gtest.h>
#include "DomainGenerator.h"

TEST(DomainGeneratorTest, DefaultConfigUsesBox) {
    ufd::DomainConfig config;
    EXPECT_EQ(config.shape, ufd::DomainShape::Box);
}

TEST(DomainGeneratorTest, DefaultExtentMultiplierIsTen) {
    ufd::DomainConfig config;
    EXPECT_DOUBLE_EQ(config.extentMultiplier, 10.0);
}
