#include "compass_nav2/measured_progress.hpp"
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
  std::cout<<"measured progress: forward/lateral/retreat/class/frame/time/jump/path/planned-offset tests pass\n";
}
