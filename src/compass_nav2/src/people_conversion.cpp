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

#include "compass_nav2/people_conversion.hpp"

#include <cmath>

#include "geometry_msgs/msg/transform_stamped.hpp"
#include "tf2/LinearMath/Quaternion.hpp"
#include "tf2/utils.hpp"

namespace compass_nav2
{

std::vector<compass::Person> toPersons(
  const compass_msgs::msg::People & msg,
  const std::string & target_frame,
  const std::shared_ptr<tf2_ros::Buffer> & tf_buffer)
{
  std::vector<compass::Person> out;
  out.reserve(msg.people.size());

  // 메시지 프레임이 비어 있으면 이미 target_frame 이라고 간주한다(스모크 기본).
  const std::string & src_frame = msg.header.frame_id;
  const bool same_frame = src_frame.empty() || src_frame == target_frame;

  // 다른 프레임이면 src->target 변환을 1회 조회해 모든 사람에 재사용한다.
  double tx = 0.0, ty = 0.0, tyaw = 0.0;
  bool have_tf = same_frame;
  if (!same_frame && tf_buffer) {
    try {
      const geometry_msgs::msg::TransformStamped tf =
        tf_buffer->lookupTransform(
          target_frame, src_frame, tf2::TimePointZero);
      tx = tf.transform.translation.x;
      ty = tf.transform.translation.y;
      tyaw = tf2::getYaw(tf.transform.rotation);
      have_tf = true;
    } catch (const tf2::TransformException &) {
      // 변환 불가: tf 가 아직 없으면 이 메시지의 사람을 모두 건너뛴다.
      have_tf = false;
    }
  }

  if (!have_tf) {
    return out;  // 변환 불가 — 빈 목록(보존적), 크래시 없음.
  }

  const double cyaw = std::cos(tyaw);
  const double syaw = std::sin(tyaw);

  for (const auto & p : msg.people) {
    compass::Person person;
    person.id = p.id;

    // 위치: same_frame 이면 그대로, 아니면 2D 강체 변환 적용.
    double px = p.x;
    double py = p.y;
    double vwx = p.vx;
    double vwy = p.vy;
    if (!same_frame) {
      px = cyaw * p.x - syaw * p.y + tx;
      py = syaw * p.x + cyaw * p.y + ty;
      // 속도는 회전 성분만(병진은 속도에 무관).
      vwx = cyaw * p.vx - syaw * p.vy;
      vwy = syaw * p.vx + cyaw * p.vy;
    }

    person.pose.x = px;
    person.pose.y = py;
    // 속도 규약: 월드 (vwx, vwy) -> 헤딩 + 선속.
    person.pose.theta = std::atan2(vwy, vwx);
    person.vel.vx = std::hypot(vwx, vwy);
    person.vel.wz = 0.0;

    person.cov[0] = p.covariance[0];
    person.cov[1] = p.covariance[1];
    person.cov[2] = p.covariance[2];
    person.cov[3] = p.covariance[3];

    out.push_back(person);
  }

  return out;
}

}  // namespace compass_nav2
