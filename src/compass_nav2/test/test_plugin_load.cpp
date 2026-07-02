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

// 런타임 스모크: CompassController 가 Nav2 의 컨트롤러 서버와 동일하게
// pluginlib 로 발견·인스턴스화되는지 확인한다. 플러그인 XML·base-class·심볼
// export 회귀를 시뮬레이터 없이 잡는다.

#include <memory>

#include <gtest/gtest.h>

#include "pluginlib/class_loader.hpp"
#include "nav2_core/controller.hpp"

TEST(PluginLoad, CompassControllerIsLoadable)
{
  pluginlib::ClassLoader<nav2_core::Controller> loader(
    "nav2_core", "nav2_core::Controller");
  std::shared_ptr<nav2_core::Controller> controller;
  ASSERT_NO_THROW(
    controller = loader.createSharedInstance("compass_nav2::CompassController"));
  EXPECT_NE(controller, nullptr);
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
