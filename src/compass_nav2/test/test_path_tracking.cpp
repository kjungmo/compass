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
//
// localGoalAhead unit tests follow (the local goal handed to the decision core).

#include <cmath>
#include <cstddef>
#include <limits>
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

// ---------------------------------------------------------------------------
// localGoalAhead: the local goal handed to the decision core.
namespace
{
using compass::Point2D;
using compass::SE2;
using compass_nav2::localGoalAhead;
const double kInf = std::numeric_limits<double>::infinity();
const double kNaN = std::numeric_limits<double>::quiet_NaN();

// y = 0 from x = 0 to 6 m every 0.25 m (25 points, 24 segments; exact in binary).
std::vector<Point2D> quarterPath()
{
  std::vector<Point2D> p;
  for (int i = 0; i <= 24; ++i) {
    p.push_back({0.25 * i, 0.0});
  }
  return p;
}

// The rule before the fix: first point at least `lookahead` from the robot,
// counted from the START of the plan; the last point if there is none.
Point2D ruleFromPlanStart(const SE2 & robot, const std::vector<Point2D> & path, double lookahead)
{
  for (const auto & p : path) {
    if (std::hypot(p.x - robot.x, p.y - robot.y) >= lookahead) {
      return p;
    }
  }
  return path.back();
}

// Out along y = 0 to x = 1, across to y = 0.5, back to x = 0 (0.5 m steps, exact).
//   p0 (0,0)  p1 (.5,0)  p2 (1,0)  p3 (1,.5)  p4 (.5,.5)  p5 (0,.5)
std::vector<Point2D> outAndBack()
{
  return {{0, 0}, {0.5, 0}, {1, 0}, {1, 0.5}, {0.5, 0.5}, {0, 0.5}};
}

Point2D goal(const SE2 & robot, const std::vector<Point2D> & path, double lookahead = 1.0)
{
  std::size_t segment = 0;
  return localGoalAhead(robot, path, lookahead, segment);
}

#define EXPECT_POINT(p, ex, ey) \
  do {const Point2D q_ = (p); EXPECT_DOUBLE_EQ(q_.x, ex); EXPECT_DOUBLE_EQ(q_.y, ey);} while (0)
}  // namespace

// The defect: 2 m along a plan that is not re-issued, the rule that counts from
// the plan start returns the plan start (behind); the goal must be 1 m ahead.
TEST(LocalGoalAhead, IsAheadOfTheRobotOnAPlanThatIsNotReissued)
{
  const auto path = quarterPath();
  const SE2 robot{2.0, 0.0, 0.0};
  EXPECT_POINT(ruleFromPlanStart(robot, path, 1.0), 0.0, 0.0);
  std::size_t segment = 0;
  EXPECT_POINT(localGoalAhead(robot, path, 1.0, segment), 3.0, 0.0);
  EXPECT_EQ(segment, 7u);  // 1.75 -> 2.0, the earlier of the two segments touching x = 2.0
}

// Exactly at the threshold counts (>=), on both sides.
TEST(LocalGoalAhead, LookaheadThresholdIsInclusive)
{
  const auto path = quarterPath();
  const SE2 robot{1.0, 0.0, 0.0};
  EXPECT_POINT(goal(robot, path, 1.0), 2.0, 0.0);                       // exactly 1.0 away
  EXPECT_POINT(goal(robot, path, std::nextafter(1.0, 2.0)), 2.25, 0.0);  // just over
  EXPECT_POINT(goal(robot, path, std::nextafter(1.0, 0.0)), 2.0, 0.0);   // just under
  EXPECT_POINT(goal(robot, path, 0.75), 1.75, 0.0);
}

// Zero or negative lookahead: the next point ahead. NaN or infinite: the last point.
TEST(LocalGoalAhead, DegenerateLookahead)
{
  const auto path = quarterPath();
  const SE2 robot{1.1, 0.0, 0.0};
  EXPECT_POINT(goal(robot, path, 0.0), 1.25, 0.0);
  EXPECT_POINT(goal(robot, path, -3.0), 1.25, 0.0);
  EXPECT_POINT(goal(robot, path, kNaN), 6.0, 0.0);
  EXPECT_POINT(goal(robot, path, kInf), 6.0, 0.0);
  EXPECT_POINT(goal(robot, path, 1e9), 6.0, 0.0);
}

// Near and past the end of the plan the goal is the last point.
TEST(LocalGoalAhead, EndOfPlanIsTheLastPoint)
{
  const auto path = quarterPath();
  EXPECT_POINT(goal({5.0, 0.0, 0.0}, path), 6.0, 0.0);   // exactly 1.0 from the last point
  EXPECT_POINT(goal({5.6, 0.0, 0.0}, path), 6.0, 0.0);   // nothing 1.0 ahead
  EXPECT_POINT(goal({6.0, 0.0, 0.0}, path), 6.0, 0.0);   // on the last point
  EXPECT_POINT(goal({7.5, 0.0, 0.0}, path), 6.0, 0.0);   // past it (now behind the robot)
  std::size_t segment = 0;
  localGoalAhead({7.5, 0.0, 0.0}, path, 1.0, segment);
  EXPECT_EQ(segment, 23u);  // the last segment
}

TEST(LocalGoalAhead, EmptyPlanIsLookaheadInFrontOfTheHeading)
{
  std::size_t segment = 9;
  const Point2D g = localGoalAhead({1.0, 2.0, M_PI_2}, {}, 1.5, segment);
  EXPECT_NEAR(g.x, 1.0, 1e-12);
  EXPECT_NEAR(g.y, 3.5, 1e-12);
  EXPECT_EQ(segment, 0u);
}

TEST(LocalGoalAhead, OnePointPlanIsThatPoint)
{
  const std::vector<Point2D> near{{0.3, 0.0}}, far{{5.0, 1.0}};
  std::size_t segment = 4;
  EXPECT_POINT(localGoalAhead({0.0, 0.0, 0.0}, near, 1.0, segment), 0.3, 0.0);
  EXPECT_EQ(segment, 0u);
  EXPECT_POINT(goal({0.0, 0.0, 0.0}, far), 5.0, 1.0);
}

// Plans with long segments: the goal is the far end of the segment the robot is
// on, never its near end (which is behind the robot).
TEST(LocalGoalAhead, SparsePlan)
{
  const std::vector<Point2D> two{{0, 0}, {4, 0}}, three{{0, 0}, {4, 0}, {8, 0}};
  EXPECT_POINT(ruleFromPlanStart({1.5, 0, 0}, two, 1.0), 0.0, 0.0);  // the defect
  EXPECT_POINT(goal({1.5, 0, 0}, two), 4.0, 0.0);
  EXPECT_POINT(goal({3.5, 0, 0}, two), 4.0, 0.0);   // 0.5 away: last point
  std::size_t segment = 0;
  EXPECT_POINT(localGoalAhead({6.0, 0, 0}, three, 1.0, segment), 8.0, 0.0);
  EXPECT_EQ(segment, 1u);  // the segment after the first is always searched
  EXPECT_POINT(goal({3.5, 0, 0}, three), 8.0, 0.0);  // (4,0) is only 0.5 away
}

// Which end of the closest segment the scan starts at. Robot on or before the
// near end (projection t = 0): the near end is not behind, it may be the goal.
// Robot past the near end (t > 0): the near end is behind and is skipped.
TEST(LocalGoalAhead, NearEndOfTheClosestSegmentCountsOnlyWhenNotBehind)
{
  const auto path = quarterPath();
  EXPECT_POINT(goal({-2.0, 0.0, 0.0}, path), 0.0, 0.0);   // plan starts 2 m ahead
  EXPECT_POINT(goal({0.0, 1.5, 0.0}, path), 0.0, 0.0);    // abeam of the start, t = 0
  EXPECT_POINT(goal({0.125, 1.5, 0.0}, path), 0.25, 0.0);  // t = 0.5: (0,0) is behind
}

// A robot 1.5 m to the side of the plan gets the plan point abeam/ahead of it.
TEST(LocalGoalAhead, RobotFarOffThePath)
{
  const auto path = quarterPath();
  EXPECT_POINT(goal({3.0, 1.5, 0.0}, path), 3.0, 0.0);      // t = 1 on 2.75 -> 3.0
  EXPECT_POINT(goal({3.0, 0.5, 0.0}, path), 4.0, 0.0);      // hypot(1, .5) >= 1 > hypot(.75, .5)
  EXPECT_POINT(goal({3.0, -40.0, 0.0}, path), 3.0, 0.0);
}

// The robot is found wherever it is on a fresh plan, in one call.
TEST(LocalGoalAhead, RobotNotAtThePlanStartIsFoundInOneCall)
{
  const auto path = quarterPath();
  std::size_t segment = 0;
  EXPECT_POINT(localGoalAhead({4.1, 0.05, 0.0}, path, 1.0, segment), 5.25, 0.0);
  EXPECT_EQ(segment, 16u);  // 4.0 -> 4.25
}

// The remembered segment never decreases: a robot that moves back along the
// plan keeps a goal at or ahead of where it had got to.
TEST(LocalGoalAhead, SegmentNeverDecreases)
{
  const auto path = quarterPath();
  std::size_t segment = 0;
  localGoalAhead({3.1, 0.0, 0.0}, path, 1.0, segment);
  ASSERT_EQ(segment, 12u);  // 3.0 -> 3.25
  EXPECT_POINT(localGoalAhead({1.0, 0.0, 0.0}, path, 1.0, segment), 3.0, 0.0);
  EXPECT_EQ(segment, 12u);
  // A new task / new geometry starts again from 0 (the caller resets it).
  segment = 0;
  EXPECT_POINT(localGoalAhead({1.0, 0.0, 0.0}, path, 1.0, segment), 2.0, 0.0);
  EXPECT_EQ(segment, 3u);
}

// A remembered segment that is not a segment of this path is treated as 0
// (n - 1 is the first index that is not a segment).
TEST(LocalGoalAhead, OutOfRangeSegmentRestartsFromThePlanStart)
{
  const auto path = quarterPath();  // 25 points: segments 0..23
  for (const std::size_t stale : {std::size_t{24}, std::size_t{25}, std::size_t{1000}}) {
    std::size_t segment = stale;
    EXPECT_POINT(localGoalAhead({1.0, 0.0, 0.0}, path, 1.0, segment), 2.0, 0.0);
    EXPECT_EQ(segment, 3u) << stale;
  }
  std::size_t last = 23;  // a real segment: kept
  EXPECT_POINT(localGoalAhead({1.0, 0.0, 0.0}, path, 1.0, last), 5.75, 0.0);
  EXPECT_EQ(last, 23u);
}

// A later pass of the plan that is closer to the robot is not taken while it is
// further than the window along the plan. The back leg starts 1.5 m of path
// after the end of segment 0 (p1): reached through p3 (1.0 m after p1) only if
// the window is at least 1.0 m.
TEST(LocalGoalAhead, LaterPassOfThePlanIsNotTakenBeyondTheWindow)
{
  const auto path = outAndBack();
  const SE2 robot{0.25, 0.4, 0.0};  // 0.4 from the out leg, 0.1 from the back leg
  std::size_t segment = 0;
  EXPECT_POINT(localGoalAhead(robot, path, 0.5, segment, 0.75), 1.0, 0.0);  // on the out leg
  EXPECT_EQ(segment, 0u);
  segment = 0;
  localGoalAhead(robot, path, 0.5, segment, std::nextafter(1.0, 0.0));  // just under
  EXPECT_EQ(segment, 0u);
  segment = 0;
  EXPECT_POINT(localGoalAhead(robot, path, 0.5, segment, 1.0), 0.0, 0.5);  // exactly: in range
  EXPECT_EQ(segment, 4u);
  segment = 0;
  localGoalAhead(robot, path, 0.5, segment, kInf);
  EXPECT_EQ(segment, 4u);
  // The default window is 1.0 m: with the out leg 2 m long the back leg starts
  // 2.0 m of path after p1 and is not reached by default.
  const std::vector<Point2D> longer{{0, 0}, {0.5, 0}, {2, 0}, {2, 0.5}, {0, 0.5}};
  EXPECT_DOUBLE_EQ(compass_nav2::kPlanSearchWindow, 1.0);
  segment = 0;
  localGoalAhead(robot, path, 0.5, segment);
  EXPECT_EQ(segment, 4u);
  segment = 0;
  localGoalAhead(robot, longer, 0.5, segment);
  EXPECT_EQ(segment, 0u);
  segment = 0;
  localGoalAhead(robot, longer, 0.5, segment, 2.0);
  EXPECT_EQ(segment, 3u);
  // Zero, negative or NaN window: a segment and the one after it only. That is
  // still enough to follow the robot from one segment to the next.
  const std::vector<Point2D> three{{0, 0}, {4, 0}, {8, 0}};
  // Out 2 m along +x, then back to a point 0.1 m from the robot: that last
  // segment starts 1.0 m of path after the end of segment 0.
  const std::vector<Point2D> hook{{0, 0}, {1, 0}, {2, 0}, {0.5, 0.2}};
  for (const double window : {0.0, -1.0, kNaN}) {
    segment = 0;
    localGoalAhead(robot, path, 0.5, segment, window);
    EXPECT_EQ(segment, 0u) << window;
    segment = 0;
    EXPECT_POINT(localGoalAhead({6.0, 0.0, 0.0}, three, 1.0, segment, window), 8.0, 0.0);
    EXPECT_EQ(segment, 1u) << window;
    // ... but not to the one after the next, although it is closer.
    segment = 0;
    localGoalAhead({0.5, 0.3, 0.0}, hook, 0.5, segment, window);
    EXPECT_EQ(segment, 0u) << window;
  }
  segment = 0;
  localGoalAhead({0.5, 0.3, 0.0}, hook, 0.5, segment);  // default window 1.0: in range
  EXPECT_EQ(segment, 2u);
}

// Equally close passes: the earlier one wins.
TEST(LocalGoalAhead, EarlierSegmentWinsATie)
{
  const auto path = outAndBack();
  std::size_t segment = 0;
  // (0.25, 0.25) is exactly 0.25 from segment 0 and from segment 4.
  EXPECT_POINT(localGoalAhead({0.25, 0.25, 0.0}, path, 0.5, segment, kInf), 1.0, 0.0);
  EXPECT_EQ(segment, 0u);
}

// Duplicate points change nothing; points that are not ordered along a line do
// not break the search (the result is a plan point at least lookahead away).
TEST(LocalGoalAhead, DuplicateAndUnorderedPoints)
{
  const auto path = quarterPath();
  std::vector<Point2D> doubled;
  for (const auto & p : path) {
    doubled.push_back(p);
    doubled.push_back(p);
    doubled.push_back(p);
  }
  for (const double x : {-1.0, 0.0, 0.3, 2.0, 4.1, 5.9, 7.0}) {
    const Point2D a = goal({x, 0.1, 0.0}, path), b = goal({x, 0.1, 0.0}, doubled);
    EXPECT_POINT(b, a.x, a.y);
  }
  // A segment shorter than 1e-6 m is skipped like a duplicate: it is never the
  // remembered segment, even when its far end is the closest point of the plan.
  auto jog = path;
  jog.insert(jog.begin() + 5, Point2D{1.0, 1e-7});  // (1, 0) -> (1, 1e-7) is segment 4
  std::size_t jog_segment = 0;
  EXPECT_POINT(localGoalAhead({1.0, 0.2, 0.0}, jog, 1.0, jog_segment), 2.0, 0.0);
  EXPECT_EQ(jog_segment, 5u);
  const std::vector<Point2D> same(5, Point2D{1.0, 1.0});  // no segment at all
  std::size_t segment = 2;
  EXPECT_POINT(localGoalAhead({0.0, 0.0, 0.0}, same, 1.0, segment), 1.0, 1.0);
  EXPECT_EQ(segment, 2u);
  // Zig-zag in x: 0, 1, 0.5, 1.5, 1, 2, ...
  std::vector<Point2D> zigzag;
  for (int i = 0; i < 8; ++i) {
    zigzag.push_back({0.5 * i, 0.0});
    zigzag.push_back({0.5 * i + 1.0, 0.0});
  }
  segment = 0;
  const SE2 robot{1.25, 0.0, 0.0};
  const Point2D g = localGoalAhead(robot, zigzag, 1.0, segment);
  EXPECT_LT(segment, zigzag.size() - 1);
  EXPECT_GE(std::hypot(g.x - robot.x, g.y - robot.y), 1.0);
  EXPECT_GT(g.x, robot.x);
}

// Non-finite input does not throw or loop, and is never preferred.
TEST(LocalGoalAhead, NonFiniteInput)
{
  auto path = quarterPath();
  // NaN robot: no segment is closest; the last point.
  std::size_t segment = 0;
  EXPECT_POINT(localGoalAhead({kNaN, 0.0, 0.0}, path, 1.0, segment), 6.0, 0.0);
  EXPECT_EQ(segment, 0u);
  // A NaN point at index 3 ends the forward search there: a robot beyond it is
  // not found, the scan from the last finite segment skips the NaN point.
  path[3].x = kNaN;
  segment = 0;
  EXPECT_POINT(localGoalAhead({0.3, 0.0, 0.0}, path, 1.0, segment), 1.5, 0.0);
  EXPECT_EQ(segment, 1u);
  segment = 0;
  const Point2D g = localGoalAhead({3.0, 0.0, 0.0}, path, 1.0, segment);
  EXPECT_EQ(segment, 1u);
  EXPECT_POINT(g, 0.5, 0.0);  // from the far end of segment 1; documented limit
  // An infinite point likewise (it is "at least lookahead away", but not a goal).
  path[3].x = kInf;
  segment = 0;
  EXPECT_POINT(localGoalAhead({0.3, 0.0, 0.0}, path, 1.0, segment), 1.5, 0.0);
  EXPECT_EQ(segment, 1u);
  path[3].x = 0.75;
  // A non-finite LAST point is returned only as the fallback.
  path.back().y = kNaN;
  EXPECT_POINT(goal({1.0, 0.0, 0.0}, path), 2.0, 0.0);
  EXPECT_TRUE(std::isnan(goal({5.9, 0.0, 0.0}, path).y));
  // Very large coordinates.
  std::vector<Point2D> big;
  for (int i = 0; i <= 24; ++i) {
    big.push_back({1e9 + 0.25 * i, -1e9});
  }
  EXPECT_POINT(goal({1e9 + 2.0, -1e9, 0.0}, big), 1e9 + 3.0, -1e9);
}

namespace
{
// Paths parametrised by path length a (m), 10 m long, starting at the origin
// along +x: a line, a left arc of radius 2 m and a right arc of radius 3 m.
using Shape = Point2D (*)(double);
Point2D line(double a) {return {a, 0.0};}
Point2D leftArc(double a) {return {2.0 * std::sin(a / 2.0), 2.0 * (1.0 - std::cos(a / 2.0))};}
Point2D rightArc(double a) {return {3.0 * std::sin(a / 3.0), -3.0 * (1.0 - std::cos(a / 3.0))};}

// A plan as a planner issues it: from the robot's position along the shape.
std::vector<Point2D> planFrom(Shape shape, double a0)
{
  std::vector<Point2D> p;
  for (double a = a0; a < 10.0; a += 0.05) {
    p.push_back(shape(a));
  }
  p.push_back(shape(10.0));
  return p;
}

// Drives 8 m at 0.45 m/s and 20 Hz with a new plan every `replan_cycles`, the
// robot `offset` m beside the path (in y), and counts the cycles in which the
// goal differs from the rule that counts from the plan start.
int cyclesDifferingFromPlanStartRule(Shape shape, int replan_cycles, double offset)
{
  int differing = 0;
  std::vector<Point2D> plan;
  std::size_t segment = 0;
  for (int i = 0; i * 0.45 * 0.05 < 8.0; ++i) {
    const double a = i * 0.45 * 0.05;
    const Point2D on_path = shape(a);
    const SE2 robot{on_path.x, on_path.y + offset, 0.0};
    if (i % replan_cycles == 0) {
      plan = planFrom(shape, a);
      segment = 0;  // what setPlan does for a new geometry
    }
    const Point2D now = localGoalAhead(robot, plan, 1.0, segment);
    const Point2D before = ruleFromPlanStart(robot, plan, 1.0);
    if (now.x != before.x || now.y != before.y) {
      ++differing;
    }
  }
  return differing;
}
}  // namespace

// For a user whose planner re-issues the plan from the robot pose at 1 Hz (the
// default Nav2 behaviour tree), the local goal is bit-for-bit the one of the
// rule that counts from the plan start, because every plan point behind the
// robot is still nearer than lookahead_dist: on a line and on two arcs,
// replanned every cycle and every 1 s with the robot on the path or 0.3 m
// beside it, and every 2 s and 2.2 s (0.9675 m at cruise speed) on the path.
TEST(LocalGoalAhead, SameAsThePlanStartRuleWhenReplannedWithinTheLookahead)
{
  int n = 0;
  for (const Shape shape : {&line, &leftArc, &rightArc}) {
    for (const int replan_cycles : {1, 20}) {
      for (const double offset : {0.0, 0.3, -0.3}) {
        EXPECT_EQ(cyclesDifferingFromPlanStartRule(shape, replan_cycles, offset), 0)
          << "shape " << n << " every " << replan_cycles << " cycles, offset " << offset;
      }
    }
    EXPECT_EQ(cyclesDifferingFromPlanStartRule(shape, 40, 0.0), 0) << "shape " << n;
    EXPECT_EQ(cyclesDifferingFromPlanStartRule(shape, 44, 0.0), 0) << "shape " << n;
    // Replanned every 4 s (1.8 m) the two rules differ: that is the defect.
    EXPECT_GT(cyclesDifferingFromPlanStartRule(shape, 80, 0.0), 0) << "shape " << n;
    ++n;
  }
}
