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

// pathTrackingAngularZ 단위 테스트 (node 불필요).
//
// 회귀로 고정하는 성질:
//   (1) 직선 경로에 정렬해 올라가 있으면 ω ≈ 0 (직진).
//   (2) 횡오차가 있으면 경로 쪽으로 조향한다 (부호 정확).
//   (3) 운동학 적분 시 경로로 *위빙 없이* 수렴한다 — 예전 베어링 비례
//       조향이 내던 ±0.8 m limit cycle 의 직접 회귀 방지.
//   (4) side_bias 는 고른 통과 측면으로 ω 를 밀어 사회적 커밋 부호를 보존한다.

#include <cmath>
#include <vector>

#include <gtest/gtest.h>

#include "compass_core/types.hpp"
#include "compass_nav2/path_tracking.hpp"

namespace
{
// y = 0 직선 경로 (x: 0 .. 6 m, 0.1 m 간격).
std::vector<compass::Point2D> straightPath()
{
  std::vector<compass::Point2D> p;
  for (int i = 0; i <= 60; ++i) {
    p.push_back({0.1 * i, 0.0});
  }
  return p;
}
}  // namespace

// (1) 직선에 정렬: ω ≈ 0.
TEST(PathTracking, AlignedOnStraightPathCommandsZero)
{
  const auto path = straightPath();
  compass::SE2 robot{0.5, 0.0, 0.0};
  const double w = compass_nav2::pathTrackingAngularZ(robot, path, 0.0, {});
  EXPECT_NEAR(w, 0.0, 1e-9);
}

// (2) 위쪽으로 횡오차 → 오른쪽으로 조향 (ω < 0), 아래쪽 → ω > 0.
TEST(PathTracking, CrossTrackSteersBackTowardPath)
{
  const auto path = straightPath();
  compass::SE2 above{0.5, 0.2, 0.0};
  compass::SE2 below{0.5, -0.2, 0.0};
  EXPECT_LT(compass_nav2::pathTrackingAngularZ(above, path, 0.0, {}), 0.0);
  EXPECT_GT(compass_nav2::pathTrackingAngularZ(below, path, 0.0, {}), 0.0);
}

// (3) 운동학 적분: 횡오차 0.2 m + 헤딩 0.2 rad 에서 위빙 없이 수렴.
TEST(PathTracking, ConvergesWithoutWeave)
{
  const auto path = straightPath();
  const double v = 0.45, dt = 0.05;
  compass::SE2 r{0.0, 0.2, 0.2};   // 경로 위 + 헤딩 오차
  double max_opposite = 0.0;       // 반대(아래)쪽 최대 침범 — 오버슈트 척도
  int sign_changes = 0;
  double prev_w = 0.0;
  for (int step = 0; step < 400 && r.x < 5.5; ++step) {
    const double w = compass_nav2::pathTrackingAngularZ(r, path, 0.0, {});
    if (step > 0 && std::abs(w) > 1e-3 && std::abs(prev_w) > 1e-3 &&
      ((w > 0) != (prev_w > 0)))
    {
      ++sign_changes;
    }
    prev_w = w;
    r.x += v * std::cos(r.theta) * dt;
    r.y += v * std::sin(r.theta) * dt;
    r.theta += w * dt;
    max_opposite = std::min(max_opposite, r.y);  // 가장 음수인 y
  }
  EXPECT_LT(std::abs(r.y), 0.05) << "경로로 수렴하지 못함 (final y).";
  EXPECT_LT(std::abs(r.theta), 0.05) << "헤딩이 접선에 정렬하지 못함.";
  EXPECT_GT(max_opposite, -0.05) << "반대쪽으로 오버슈트 (위빙).";
  EXPECT_LE(sign_changes, 1) << "ω 부호가 반복 전환 (limit cycle/위빙).";
}

// (4) side_bias 부호 보존: 좌(+1) → ω 증가, 우(−1) → ω 감소.
TEST(PathTracking, SideBiasShiftsTurnDirection)
{
  const auto path = straightPath();
  compass::SE2 robot{0.5, 0.0, 0.0};  // 정렬 (편향 없으면 ω=0)
  const double w0 = compass_nav2::pathTrackingAngularZ(robot, path, 0.0, {});
  const double w_left = compass_nav2::pathTrackingAngularZ(robot, path, 1.0, {});
  const double w_right = compass_nav2::pathTrackingAngularZ(robot, path, -1.0, {});
  EXPECT_GT(w_left, w0);
  EXPECT_LT(w_right, w0);
  EXPECT_NEAR(w_left, 0.4, 1e-9);    // k_side 기본 0.4 × (+1)
  EXPECT_NEAR(w_right, -0.4, 1e-9);
}
