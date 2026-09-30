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
  std::cout<<"measured progress: forward/lateral/retreat/class/frame/time/jump/path tests pass\n";
}
