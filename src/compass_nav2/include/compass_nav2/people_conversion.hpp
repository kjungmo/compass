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

#ifndef COMPASS_NAV2__PEOPLE_CONVERSION_HPP_
#define COMPASS_NAV2__PEOPLE_CONVERSION_HPP_

#include <memory>
#include <string>
#include <vector>

#include "tf2_ros/buffer.h"

#include "compass_msgs/msg/people.hpp"
#include "compass_core/types.hpp"

namespace compass_nav2
{

// compass_msgs/People 한 건을 compass::Person 목록으로 변환한다.
//
// 좌표계 규약: 변환 결과는 target_frame (costmap global_frame) 기준이어야 한다.
// 메시지의 header.frame_id 가 target_frame 과 같으면(또는 비어 있으면) 빠른
// 경로로 그대로 복사한다. 다르면 tf_buffer 로 위치·속도를 target_frame 으로
// 변환한다. tf 가 없거나(nullptr) 변환이 불가능한 사람은 조용히 건너뛴다
// (스모크/단위 테스트에서 tf 미구성 시 크래시 방지).
//
// 속도 규약(compass_core::Twist2D = {vx, wz}): 월드 속도 (vx, vy) 는 헤딩+선속
// 으로 사상한다 — pose.theta = atan2(vy, vx), vel.vx = hypot(vx, vy). 각속 wz
// 는 메시지에 없으므로 0. 이는 기존 TTC 코드(vel.vx 를 pose.theta 로 분해)와
// 일치한다.
//
// node 가 필요 없는 자유 함수이므로 단위 테스트가 가능하다.
std::vector<compass::Person> toPersons(
  const compass_msgs::msg::People & msg,
  const std::string & target_frame,
  const std::shared_ptr<tf2_ros::Buffer> & tf_buffer = nullptr);

}  // namespace compass_nav2

#endif  // COMPASS_NAV2__PEOPLE_CONVERSION_HPP_
