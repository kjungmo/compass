// Copyright (c) 2026 Kang Jung Mo
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
//     http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.

// Configure-time parameter validation: out-of-range values are rejected with a
// message that names the parameter and its allowed range; defaults load unchanged.

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "compass_nav2/compass_controller.hpp"
#include "compass_nav2/param_checks.hpp"

namespace
{
class LoadProbe : public compass_nav2::CompassController
{
public:
  void load(const std::vector<rclcpp::Parameter> & overrides)
  {
    rclcpp::NodeOptions options;
    options.parameter_overrides(overrides);
    auto node = std::make_shared<rclcpp_lifecycle::LifecycleNode>("load_probe", options);
    loadKnobs(node, "FollowPath");
  }
  const compass::Knobs & knobs() const {return knobs_;}
  double maxLinear() const {return max_linear_speed_;}
  double cruise() const {return cruise_speed_;}
};

std::string loadError(const rclcpp::Parameter & p)
{
  LoadProbe probe;
  try {
    probe.load({p});
  } catch (const std::invalid_argument & e) {
    return e.what();
  }
  return "";
}
}  // namespace

TEST(ParamValidation, DefaultsLoadUnchanged)
{
  LoadProbe probe;
  ASSERT_NO_THROW(probe.load({}));
  const compass::Knobs d;
  EXPECT_DOUBLE_EQ(probe.knobs().lambda, d.lambda);
  EXPECT_DOUBLE_EQ(probe.knobs().d_safe, d.d_safe);
  EXPECT_EQ(probe.knobs().n_thrash, d.n_thrash);
  EXPECT_DOUBLE_EQ(probe.maxLinear(), 0.5);
}

TEST(ParamValidation, RejectsOutOfRangeValuesNamingParameterAndRange)
{
  struct Case {rclcpp::Parameter p; std::string expect;};
  const std::vector<Case> cases = {
    {{"FollowPath.max_linear_speed", -1.0}, "FollowPath.max_linear_speed = -1 is outside the allowed range (0, inf)"},
    {{"FollowPath.d_safe", -0.5}, "FollowPath.d_safe = -0.5 is outside the allowed range [0, inf)"},
    {{"FollowPath.n_thrash", 0}, "FollowPath.n_thrash = 0 is outside the allowed range [1, inf)"},
    {{"FollowPath.lambda", 1.5}, "FollowPath.lambda = 1.5 is outside the allowed range (0, 1]"},
    {{"FollowPath.cruise_speed", 0.0}, "FollowPath.cruise_speed = 0 is outside the allowed range (0, inf)"},
    {{"FollowPath.sigma_s", 0.0}, "FollowPath.sigma_s = 0 is outside the allowed range (0, inf)"},
    {{"FollowPath.K_cap", 40}, "FollowPath.K_cap = 40 is outside the allowed range [0, 16]"},
    {{"FollowPath.progress_max_gap", 0.0}, "FollowPath.progress_max_gap = 0 is outside the allowed range (0, inf)"},
    {{"FollowPath.W", -3.0}, "FollowPath.W = -3 is outside the allowed range (0, inf)"},
  };
  for (const auto & c : cases) {
    EXPECT_EQ(loadError(c.p), c.expect) << c.p.get_name();
  }
}

TEST(ParamValidation, ZeroAblationsAndCappedCruiseStayAccepted)
{
  // k_rho = 0 and zero weights are documented ablations; cruise above the hard
  // cap is clamped at run time (configure only warns).
  EXPECT_EQ(loadError({"FollowPath.k_rho", 0.0}), "");
  EXPECT_EQ(loadError({"FollowPath.w_e", 0.0}), "");
  LoadProbe probe;
  ASSERT_NO_THROW(probe.load({{"FollowPath.cruise_speed", 5.0}}));
  EXPECT_DOUBLE_EQ(probe.cruise(), 5.0);
}

TEST(ParamValidation, RequireRangeRejectsNonFinite)
{
  EXPECT_THROW(compass_nav2::requireRange("x", std::nan(""), 0, 1), std::invalid_argument);
  EXPECT_THROW(
    compass_nav2::requireRange("x", compass_nav2::kInf, 0, compass_nav2::kInf),
    std::invalid_argument);
  EXPECT_NO_THROW(compass_nav2::requireRange("x", 1.0, 0, 1));
  EXPECT_THROW(compass_nav2::requireRange("x", 1.0, 0, 1, false, true), std::invalid_argument);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);
  const int rc = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return rc;
}
