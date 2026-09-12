#include <gtest/gtest.h>
#include <cmath>
#include "compass_nav2/costmap_env_query.hpp"
#include "nav2_costmap_2d/cost_values.hpp"

namespace {
void mark(nav2_costmap_2d::Costmap2D& map, double x, double y, unsigned char cost) {
  unsigned int mx=0,my=0;
  ASSERT_TRUE(map.worldToMap(x,y,mx,my));
  map.setCost(mx,my,cost);
}
compass_nav2::CostmapEnvQuery query(
    nav2_costmap_2d::Costmap2D* map, const std::vector<compass::Person>& people={},
    const compass::SE2& pose={0,0,0},
    const std::vector<compass::Point2D>& path={{0,0},{4,0}}, double speed=.45) {
  compass_nav2::CostmapEnvQuery env;
  env.setContext(map,pose,{4,0},people,{speed,0},true,path,speed,{});
  return env;
}
}

TEST(CandidateEnvironment, SweptObstacleDistinguishesSides) {
  nav2_costmap_2d::Costmap2D map(100,100,.1,-5,-5,nav2_costmap_2d::FREE_SPACE);
  mark(map,.45,.50,nav2_costmap_2d::LETHAL_OBSTACLE);
  auto env=query(&map);
  compass::TopoClass left,right;left.set(7,compass::Side::L);right.set(7,compass::Side::R);
  EXPECT_LT(env.clearance(left),env.clearance(right));
  EXPECT_FALSE(env.executionSafe(left,.45,.20,2));
  EXPECT_TRUE(env.executionSafe(right,.45,.20,2));
}

TEST(CandidateEnvironment, UnknownAndMapBoundaryFailClosed) {
  nav2_costmap_2d::Costmap2D map(100,100,.1,-5,-5,nav2_costmap_2d::FREE_SPACE);
  compass::TopoClass empty;
  mark(map,.3,0,nav2_costmap_2d::NO_INFORMATION);
  auto env=query(&map);
  EXPECT_DOUBLE_EQ(env.clearance(empty),0);
  EXPECT_FALSE(env.feasible(empty));

  auto edge=query(&map,{}, {4.8,0,0}, {{4.8,0},{8,0}},.1);
  EXPECT_DOUBLE_EQ(edge.clearance(empty),0);
  EXPECT_FALSE(edge.executionSafe(empty,.1,0,0));
  auto no_map=query(nullptr);
  EXPECT_DOUBLE_EQ(no_map.clearance(empty),0);
  EXPECT_FALSE(no_map.feasible(empty));
}

TEST(CandidateEnvironment, ApproachingAndRecedingPeopleDiffer) {
  nav2_costmap_2d::Costmap2D map(100,100,.1,-5,-5,nav2_costmap_2d::FREE_SPACE);
  compass::Person p;p.pose={1,0,std::acos(-1.)};p.vel.vx=.5;
  compass::TopoClass empty;
  const auto approaching=query(&map,{p});
  EXPECT_LT(approaching.ttc(empty),2);
  p.pose.theta=0;p.vel.vx=1.;
  const auto receding=query(&map,{p});
  EXPECT_DOUBLE_EQ(receding.ttc(empty),10);
}

TEST(CandidateEnvironment, ModifiedSpeedIsRevalidated) {
  nav2_costmap_2d::Costmap2D map(100,100,.1,-5,-5,nav2_costmap_2d::FREE_SPACE);
  mark(map,.65,0,nav2_costmap_2d::LETHAL_OBSTACLE);
  auto env=query(&map);
  compass::TopoClass empty;
  EXPECT_FALSE(env.executionSafe(empty,.45,.1,2));
  EXPECT_TRUE(env.executionSafe(empty,.1,.1,2));
  const auto execution=env.trajectoryAtSpeed(empty,.1);
  ASSERT_FALSE(execution.empty());
  EXPECT_DOUBLE_EQ(execution.front().command.vx,.1);
  EXPECT_DOUBLE_EQ(execution.front().command.wz,0);
}

TEST(CandidateEnvironment, SweptClearanceRejectsBetweenSampleContact) {
  // At 1 m/s the first poses are x=0 and x=.05. This lethal cell lies at
  // x=[.025,.026], y=[.2499,.2509], overlapping the radius-.25 footprint only
  // between those samples. The old point-only check returned ~.000889 m and
  // accepted execution at d_safe=0. A fine map is a valid software input.
  nav2_costmap_2d::Costmap2D map(1800,800,.001,-.4,-.4001,
    nav2_costmap_2d::FREE_SPACE);
  map.setCost(425,650,nav2_costmap_2d::LETHAL_OBSTACLE);
  auto env=query(&map,{}, {0,0,0}, {{0,0},{4,0}},1.);
  compass::TopoClass empty;
  EXPECT_DOUBLE_EQ(env.clearance(empty),0);
  EXPECT_FALSE(env.feasible(empty));
  EXPECT_FALSE(env.executionSafe(empty,1.,0.,0.));
}

TEST(LegacyEnvironment, ApproachingAndRecedingTtcUseCorrectRelativeSign) {
  nav2_costmap_2d::Costmap2D map(100,100,.1,-5,-5,nav2_costmap_2d::FREE_SPACE);
  compass_nav2::CostmapEnvQuery env;
  compass::Person person;
  person.pose={1,0,0};
  person.vel.vx=0;
  compass::TopoClass empty;
  env.setContext(&map,{0,0,0},{4,0},{person},{.5,0});
  EXPECT_DOUBLE_EQ(env.ttc(empty),2);
  person.vel.vx=1;
  env.setContext(&map,{0,0,0},{4,0},{person},{.5,0});
  EXPECT_DOUBLE_EQ(env.ttc(empty),10);
}

TEST(LegacyEnvironment, MissingUnknownAndOutOfMapAreUnavailable) {
  compass_nav2::CostmapEnvQuery env;
  compass::TopoClass empty;
  env.setContext(nullptr,{0,0,0},{4,0},{},{.5,0});
  EXPECT_DOUBLE_EQ(env.clearance(empty),0);
  EXPECT_DOUBLE_EQ(env.corridor_width(empty),0);
  EXPECT_FALSE(env.feasible(empty));
  nav2_costmap_2d::Costmap2D map(100,100,.1,-5,-5,nav2_costmap_2d::FREE_SPACE);
  mark(map,0,0,nav2_costmap_2d::NO_INFORMATION);
  env.setContext(&map,{0,0,0},{4,0},{},{.5,0});
  EXPECT_DOUBLE_EQ(env.clearance(empty),0);
  EXPECT_FALSE(env.feasible(empty));
  env.setContext(&map,{5.1,0,0},{8,0},{},{.5,0});
  EXPECT_DOUBLE_EQ(env.clearance(empty),0);
  EXPECT_FALSE(env.feasible(empty));
}

TEST(LegacyEnvironment, RaySidesMatchCandidateAndEmittedPathTracking) {
  nav2_costmap_2d::Costmap2D map(100,100,.1,-5,-5,nav2_costmap_2d::FREE_SPACE);
  mark(map,.55,.25,nav2_costmap_2d::LETHAL_OBSTACLE);
  compass_nav2::CostmapEnvQuery env;
  compass::TopoClass left,right;
  left.set(7,compass::Side::L);
  right.set(7,compass::Side::R);
  env.setContext(&map,{0,0,0},{4,0},{},{.5,0});
  EXPECT_LT(env.clearance(left),env.clearance(right));
  const auto l=compass_nav2::candidateRollout({0,0,0},{{0,0},{4,0}},left,.5,{});
  const auto r=compass_nav2::candidateRollout({0,0,0},{{0,0},{4,0}},right,.5,{});
  EXPECT_LT(0,l.front().command.wz);
  EXPECT_LT(r.front().command.wz,0);
}
