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
