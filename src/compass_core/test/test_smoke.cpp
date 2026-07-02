#include <gtest/gtest.h>
#include "compass_core/types.hpp"
TEST(Smoke, VersionString) { EXPECT_STREQ(compass::version(), "0.1.0"); }
