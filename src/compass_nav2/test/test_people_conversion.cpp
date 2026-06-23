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

// toPersons 단위 테스트: node 없이 compass_msgs/People -> compass::Person 변환의
// id 보존과 속도 규약(월드 (vx,vy) -> pose.theta=atan2(vy,vx), vel.vx=hypot)을
// 회귀로 고정한다. tf 미구성(nullptr) 동작도 확인한다.

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "compass_msgs/msg/people.hpp"
#include "compass_msgs/msg/person.hpp"
#include "compass_nav2/people_conversion.hpp"

namespace
{
compass_msgs::msg::Person makePerson(
  uint64_t id, double x, double y, double vx, double vy)
{
  compass_msgs::msg::Person p;
  p.id = id;
  p.x = x;
  p.y = y;
  p.vx = vx;
  p.vy = vy;
  p.covariance = {0.01, 0.0, 0.0, 0.01};
  return p;
}
}  // namespace

// 같은 프레임(빈 frame_id)에서 두 사람을 변환 — id·heading·speed 규약 확인.
TEST(ToPersons, TwoPeopleSameFrameConvention)
{
  compass_msgs::msg::People msg;
  msg.header.frame_id = "";  // 빈 프레임 => target_frame 으로 간주(빠른 경로).

  // 사람 0: 월드 속도 (+1, 0) => 헤딩 0, 선속 1.
  msg.people.push_back(makePerson(11u, 2.0, 3.0, 1.0, 0.0));
  // 사람 1: 월드 속도 (0, +2) => 헤딩 +pi/2, 선속 2.
  msg.people.push_back(makePerson(22u, -1.0, 0.5, 0.0, 2.0));

  const std::vector<compass::Person> out =
    compass_nav2::toPersons(msg, "odom", nullptr);

  ASSERT_EQ(out.size(), 2u);

  // 사람 0.
  EXPECT_EQ(out[0].id, 11u);
  EXPECT_NEAR(out[0].pose.x, 2.0, 1e-9);
  EXPECT_NEAR(out[0].pose.y, 3.0, 1e-9);
  EXPECT_NEAR(out[0].pose.theta, 0.0, 1e-9);
  EXPECT_NEAR(out[0].vel.vx, 1.0, 1e-9);
  EXPECT_NEAR(out[0].vel.wz, 0.0, 1e-9);
  EXPECT_NEAR(out[0].cov[0], 0.01, 1e-9);

  // 사람 1.
  EXPECT_EQ(out[1].id, 22u);
  EXPECT_NEAR(out[1].pose.x, -1.0, 1e-9);
  EXPECT_NEAR(out[1].pose.y, 0.5, 1e-9);
  EXPECT_NEAR(out[1].pose.theta, M_PI / 2.0, 1e-9);
  EXPECT_NEAR(out[1].vel.vx, 2.0, 1e-9);
}

// 헤딩이 대각선(45도)인 경우의 규약 확인.
TEST(ToPersons, DiagonalVelocityHeading)
{
  compass_msgs::msg::People msg;
  msg.header.frame_id = "odom";  // target 과 동일 => 빠른 경로.
  msg.people.push_back(makePerson(7u, 0.0, 0.0, 3.0, 4.0));

  const std::vector<compass::Person> out =
    compass_nav2::toPersons(msg, "odom", nullptr);

  ASSERT_EQ(out.size(), 1u);
  EXPECT_EQ(out[0].id, 7u);
  EXPECT_NEAR(out[0].pose.theta, std::atan2(4.0, 3.0), 1e-9);
  EXPECT_NEAR(out[0].vel.vx, 5.0, 1e-9);  // hypot(3,4)=5
}

// 다른 프레임인데 tf_buffer 가 없으면 빈 목록(크래시 없음).
TEST(ToPersons, DifferentFrameNoTfYieldsEmpty)
{
  compass_msgs::msg::People msg;
  msg.header.frame_id = "map";
  msg.people.push_back(makePerson(1u, 1.0, 1.0, 0.5, 0.0));

  const std::vector<compass::Person> out =
    compass_nav2::toPersons(msg, "odom", nullptr);

  EXPECT_TRUE(out.empty());
}

int main(int argc, char ** argv)
{
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
