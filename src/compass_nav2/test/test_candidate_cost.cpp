#include "compass_core/cost_evaluator.hpp"
#include "compass_nav2/candidate_rollout.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
#include <limits>
using namespace compass;
struct Env : IEnvQuery {
  std::optional<CandidateTrajectory> left,right;
  std::optional<CandidateTrajectory> candidate_trajectory(const TopoClass& c) const override {
    return c.side(7)==Side::L?left:right;
  }
  double corridor_width(const TopoClass&) const override {return 3;}
  double clearance(const TopoClass&) const override {return 2;}
  double ttc(const TopoClass&) const override {return 10;}
  bool feasible(const TopoClass&) const override {return true;}
};
int main() {
  TopoClass L,R;L.set(7,Side::L);R.set(7,Side::R);
  DecisionInput in;in.local_goal={2,1};
  const std::vector<Point2D> path={{0,0},{4,0}};
  compass_nav2::PathTrackGains gains;
  Env env;
  env.left=compass_nav2::candidateRollout(in.robot_pose,path,L,.45,gains);
  env.right=compass_nav2::candidateRollout(in.robot_pose,path,R,.45,gains);
  assert(env.left->size()==21 && env.right->size()==21);
  assert(env.left->back().pose.y>0 && env.right->back().pose.y<0);
  assert(std::abs(env.left->back().pose.y+env.right->back().pose.y)<1e-12);
  assert(env.left->front().command.wz>0 && env.right->front().command.wz<0);
  Knobs k;k.w_g=1;k.w_s=k.w_e=k.w_r=0;
  assert(CostEvaluator(k).J(L,in,env)<CostEvaluator(k).J(R,in,env));
  // A person on the upper rollout penalizes L, independently of the rule weight.
  Person person;person.pose={.45,.3,0};in.people={person};
  k.w_g=0;k.w_s=1;
  assert(CostEvaluator(k).J(L,in,env)>CostEvaluator(k).J(R,in,env));
  const double stationary=CostEvaluator(k).J(L,in,env);
  in.people[0].vel.vx=3.;
  assert(CostEvaluator(k).J(L,in,env)<stationary);
  in.people.clear();
  auto stronger=gains;stronger.k_side=.8;
  env.left=compass_nav2::candidateRollout(in.robot_pose,path,L,.45,stronger);
  k.w_s=0;k.w_e=1;
  assert(CostEvaluator(k).J(L,in,env)>CostEvaluator(k).J(R,in,env));
  const auto good=*env.left;
  env.left=CandidateTrajectory{};assert(std::isinf(CostEvaluator(k).J(L,in,env)));
  env.left=good;env.left->back().t=.5;assert(std::isinf(CostEvaluator(k).J(L,in,env)));
  env.left=good;(*env.left)[3].t=(*env.left)[2].t;
  assert(std::isinf(CostEvaluator(k).J(L,in,env)));
  env.left=good;env.left->front().pose.x=1;
  assert(std::isinf(CostEvaluator(k).J(L,in,env)));
  env.left=good;env.left->back().pose.x+=1;
  assert(std::isinf(CostEvaluator(k).J(L,in,env)));
  env.left=good;(*env.left)[5].pose.x=std::numeric_limits<double>::quiet_NaN();
  assert(std::isinf(CostEvaluator(k).J(L,in,env)));
  env.left=good;person.cov[0]=-1;in.people={person};
  assert(std::isinf(CostEvaluator(k).J(L,in,env)));
  in.people.clear();
  // Omitted seam preserves the archived evaluator even when classes differ.
  env.left=std::nullopt;env.right=std::nullopt;
  assert(CostEvaluator(k).J(L,in,env)==CostEvaluator(k).J(R,in,env));
  auto mixed=L;mixed.set(8,Side::R);
  assert(compass_nav2::candidateRollout({},path,mixed,.45,gains).empty());
  assert(compass_nav2::candidateRollout({},{{0,0},{0,0}},L,.45,gains).empty());
  assert(compass_nav2::candidateRollout({},path,L,-.1,gains).empty());
  std::cout<<"candidate costs: geometry, goal/social/effort ranking, prediction, invalid data, legacy fallback pass\n";
}
