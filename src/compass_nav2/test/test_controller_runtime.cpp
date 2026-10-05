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

// Deployment behaviour of the configured plugin (real configure() on a lifecycle
// node with a configured costmap): people topic, people freshness, operator
// diagnostics and the Humble task boundary.

#include <memory>
#include <string>
#include <vector>

#include <gtest/gtest.h>

#include "compass_nav2/compass_controller.hpp"

namespace
{
class RuntimeProbe : public compass_nav2::CompassController
{
public:
  std::string peopleTopic() const {return people_sub_->get_topic_name();}
};

struct Fixture
{
  rclcpp_lifecycle::LifecycleNode::SharedPtr node;
  std::shared_ptr<nav2_costmap_2d::Costmap2DROS> costmap;
  std::shared_ptr<tf2_ros::Buffer> tf;

  explicit Fixture(
    const std::vector<rclcpp::Parameter> & overrides = {}, const std::string & ns = "/")
  {
    rclcpp::NodeOptions options;
    options.parameter_overrides(overrides);
    node = std::make_shared<rclcpp_lifecycle::LifecycleNode>("controller_server", ns, options);
    costmap = std::make_shared<nav2_costmap_2d::Costmap2DROS>("runtime_probe_costmap");
    costmap->configure();
    tf = std::make_shared<tf2_ros::Buffer>(node->get_clock());
  }
  ~Fixture() {costmap->cleanup();}

  void configure(compass_nav2::CompassController & c)
  {
    c.configure(node, "FollowPath", tf, costmap);
  }
};
}  // namespace

TEST(PeopleTopic, DefaultIsTheAbsolutePeopleTopic)
{
  Fixture f({}, "/robot1");
  RuntimeProbe controller;
  f.configure(controller);
  EXPECT_EQ(controller.peopleTopic(), "/people");
  controller.cleanup();
}

TEST(PeopleTopic, RelativeNameResolvesInTheNodeNamespace)
{
  Fixture f({{"FollowPath.people_topic", "tracked_people"}}, "/robot1");
  RuntimeProbe controller;
  f.configure(controller);
  EXPECT_EQ(controller.peopleTopic(), "/robot1/tracked_people");
  controller.cleanup();
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  rclcpp::init(argc, argv);
  const int rc = RUN_ALL_TESTS();
  rclcpp::shutdown();
  return rc;
}
