#include "compass_nav2/measured_progress.hpp"
#include "compass_nav2/path_tracking.hpp"
#include "compass_core/decision_core.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
int main() {
  using namespace compass;using namespace compass_nav2;
  MeasuredProgress m;TopoClass L,R;L.set(7,Side::L);R.set(7,Side::R);
  std::vector<Point2D> path={{0,0},{10,0}};
  assert(m.sample({0,0,0},0,"odom",L,path).valid);
  assert(m.sample({.05,0,0},.1,"odom",L,path).delta_m==0);
  assert(std::abs(m.sample({.05,.05,0},.2,"odom",L,path).delta_m-.05)<1e-12);
  assert(m.sample({.05,0,0},.3,"odom",L,path).delta_m<0);
  assert(m.sample({.05,0,0},.4,"odom",R,path).delta_m==0);
  assert(m.sample({.05,-.05,0},.5,"odom",R,path).delta_m>0);
  assert(!m.sample({.05,-.05,0},.5,"odom",R,path).valid);
  assert(m.sample({0,0,0},.6,"map",L,path).delta_m==0);
  assert(!m.sample({10,0,0},.7,"map",L,path).valid);
  assert(m.sample({0,0,0},.8,"map",L,path).valid);
  assert(!m.sample({0,0,0},2.,"map",L,path).valid);
  m.reset();assert(!m.sample({0,0,0},3.,"map",L,{}).valid);
  // Issue #8: planned lateral offset fixed at the epoch anchor.
  const double target=.4/3.;   // default k_side/k_e tracker equilibrium
  {MeasuredProgress p;auto a=p.sample({0,0,0},0,"odom",L,path);
   assert(a.valid&&a.anchor&&a.delta_m==0);
   assert(std::abs(p.planned_offset(.4,3.)-target)<1e-12);
   assert(!p.sample({.05,0,0},.1,"odom",L,path).anchor);}
  {MeasuredProgress p;p.sample({0,-target,0},0,"odom",L,path);   // anchored on the R side
   assert(std::abs(p.anchor_offset()+target)<1e-12);
   assert(std::abs(p.planned_offset(.4,3.)-2*target)<1e-12);}
  {MeasuredProgress p;p.sample({0,-.2,0},0,"odom",R,path);        // already beyond the R target
   assert(p.planned_offset(.4,3.)==0);}                            // zero planned offset: no maneuver
  {MeasuredProgress p;p.sample({0,0,0},0,"odom",L,path);
   assert(p.planned_offset(0.,3.)==0&&p.planned_offset(.4,0.)==0);}  // no tracker bias: no maneuver
  {MeasuredProgress p;TopoClass none;p.sample({0,0,0},0,"odom",none,path);
   assert(p.planned_offset(.4,3.)==0);}                            // empty class: no maneuver
  {MeasuredProgress p;assert(p.planned_offset(.4,3.)==0);}          // not anchored
  {MeasuredProgress p;p.sample({0,0,0},0,"odom",L,path);            // plan replaced -> re-anchor
   p.reset();auto a=p.sample({-1,.1,0},.1,"odom",L,{{0,0},{0,10}}); // new path along +y; L side is -x
   assert(a.anchor&&std::abs(p.anchor_offset()-1.)<1e-12&&p.planned_offset(.4,3.)==0);}
  // Issue #8 (closure review): L_plan uses the steering bias (mean pair sign),
  // not k_side/k_e, so a mixed-side class is not overstated.
  {TopoClass m;m.set(1,Side::L);m.set(2,Side::L);m.set(3,Side::R);          // 2 L + 1 R, b = 1/3
   MeasuredProgress p;assert(p.sample({0,0,0},0,"odom",m,path).anchor);
   const double planned=.4*(1./3.)/3.;
   assert(std::abs(p.planned_offset(.4,3.)-planned)<1e-12);
   assert(std::abs(p.anchored_plan_length(0.,.4,3.)-planned)<1e-12);       // anchor at maneuver start
   assert(std::abs(p.anchored_plan_length(.2,.4,3.)-(.2+planned))<1e-12);  // credited progress kept
   TopoClass mr;mr.set(1,Side::R);mr.set(2,Side::R);mr.set(3,Side::L);      // 2 R + 1 L, mirrored
   MeasuredProgress q;q.sample({0,0,0},0,"odom",mr,path);
   assert(std::abs(q.planned_offset(.4,3.)-planned)<1e-12);
   TopoClass b;b.set(1,Side::L);b.set(2,Side::R);                          // balanced: no side
   MeasuredProgress z;assert(!z.sample({0,0,0},0,"odom",b,path).valid);
   assert(z.planned_offset(.4,3.)==0&&z.anchored_plan_length(.3,.4,3.)==.3);}
  // Adapter-level anchoring with the real tracker and the real core: realized
  // progress equal to the anchored L_plan gives rho = 1, and the closed-loop
  // tracker equilibrium for a mixed class reaches it (rho -> 1, not 1/3).
  {TopoClass m;m.set(1,Side::L);m.set(2,Side::L);m.set(3,Side::R);
   PathTrackGains g;   // defaults k_e = 3, k_theta = 3, k_side = .4, max_w = 1
   DecisionCore core{Knobs{}};
   ClassEval only;only.cls=m;only.J=.5;only.available=only.safe=true;
   // (a) exact: one measured delta equal to the planned offset.
   {MeasuredProgress p;DecisionState s;s.c_star=m;
    assert(p.sample({0,0,0},0,"odom",m,path).anchor);
    s.L_plan=p.anchored_plan_length(s.L_real,g.k_side,g.k_e);
    assert(s.L_plan>0&&std::abs(s.L_plan-p.planned_offset(g.k_side,g.k_e))<1e-15);
    core.step_evals({only},s,5.,.5,0.,.05,nullptr,s.L_plan);
    assert(s.rho==1.);}
   // (b) closed loop: unicycle driven by pathTrackingAngularZ with the shared bias.
   {MeasuredProgress p;DecisionState s;s.c_star=m;SE2 x{0,0,0};double t=0;const double dt=.05,v=.5;
    assert(p.sample(x,t,"odom",m,path).anchor);
    s.L_plan=p.anchored_plan_length(s.L_real,g.k_side,g.k_e);
    for(int i=0;i<400;++i) {
      const double w=pathTrackingAngularZ(x,path,side_bias(m),g);
      x.x+=v*std::cos(x.theta)*dt;x.y+=v*std::sin(x.theta)*dt;x.theta+=w*dt;t+=dt;
      const auto d=p.sample(x,t,"odom",m,path);assert(d.valid&&!d.anchor);
      core.step_evals({only},s,5.,v,t,dt,nullptr,d.delta_m);
    }
    assert(std::abs(x.y-s.L_plan)<1e-3);            // tracker settles at the planned offset
    assert(s.rho>.99);                              // rho reaches ~1 for the mixed class
    assert(s.L_real/(g.k_side/g.k_e)<.34);}}        // the old k_side/k_e L_plan would cap rho at ~1/3
  std::cout<<"measured progress: forward/lateral/retreat/class/frame/time/jump/path/planned-offset/mixed-class/anchoring tests pass\n";
}
