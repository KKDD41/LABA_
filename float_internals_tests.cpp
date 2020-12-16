#include <gtest/gtest.h>
#include "float_internals.h"
#include <cmath>
#include <limits>

TEST(Test_0, SampleTests) {
  EXPECT_EQ(FloatInternalsAsString(0), "+0");
  EXPECT_EQ(FloatInternalsAsString(42),
            "+1.01010000000000000000000_2 * 2^{5}");
  EXPECT_EQ(FloatInternalsAsString(1.0e-38),
               "+0.11011001110001111101110_2 * 2^{-126}");
  EXPECT_EQ(FloatInternalsAsString(-0.1),
            "-1.10011001100110011001101_2 * 2^{-4}");
}

TEST(Test_1, SimpleCases) {
  EXPECT_EQ(FloatInternalsAsString(1.0e-37),
            "+1.00010000001110011101010_2 * 2^{-123}");
  EXPECT_EQ(FloatInternalsAsString(1.5e-38),
            "+1.01000110101010111100110_2 * 2^{-126}");
  EXPECT_EQ(FloatInternalsAsString(-1.777e-38),
            "-1.10000010111111110001000_2 * 2^{-126}");
  EXPECT_EQ(FloatInternalsAsString(1.1),
            "+1.00011001100110011001101_2 * 2^{0}");
  EXPECT_EQ(FloatInternalsAsString(1.19209e-07),
            "+1.11111111111111111010111_2 * 2^{-24}");
  EXPECT_EQ(FloatInternalsAsString(std::numeric_limits<float>::epsilon()),
            "+1.00000000000000000000000_2 * 2^{-23}");
  EXPECT_EQ(FloatInternalsAsString(1.40129846432e-45),
            "+0.00000000000000000000001_2 * 2^{-126}");
  EXPECT_EQ(FloatInternalsAsString(7.17464813734e-43),
            "+0.00000000000001000000000_2 * 2^{-126}");
}

TEST(Test_3, HardCases) {
  EXPECT_EQ(FloatInternalsAsString(1.701e38),
            "+1.11111111111000000100011_2 * 2^{126}");
  EXPECT_EQ(FloatInternalsAsString(1.7014118e38),
            "+1.00000000000000000000000_2 * 2^{127}");
  EXPECT_EQ(FloatInternalsAsString(std::numeric_limits<float>::max()),
            "+1.11111111111111111111111_2 * 2^{127}");
  EXPECT_EQ(FloatInternalsAsString(std::numeric_limits<float>::min()),
            "+1.00000000000000000000000_2 * 2^{-126}");
}

TEST(Test_2, SpecialCases) {
  EXPECT_EQ(FloatInternalsAsString(0), "+0");
  EXPECT_EQ(FloatInternalsAsString(-1.0e-100), "-0");
  EXPECT_EQ(FloatInternalsAsString(INFINITY), "+INF");
  EXPECT_EQ(FloatInternalsAsString(-std::numeric_limits<float>::infinity()),
            "-INF");
  EXPECT_EQ(FloatInternalsAsString(-INFINITY), "-INF");
}

