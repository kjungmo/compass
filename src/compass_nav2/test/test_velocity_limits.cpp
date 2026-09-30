#include "compass_core/decision_core.hpp"
#include "compass_nav2/velocity_limits.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <limits>
#include <iostream>
int main() {
  using namespace compass; using compass_nav2::applyCoreVelocityLimit;
  using compass_nav2::applyCandidateVelocityLimit;
  Knobs k;DecisionCore core{k};DecisionState s;s.c_star.set(7,Side::R);
  // Issue #4: a_brake is acceleration, not a per-cycle velocity decrement.
  auto out=core.step_evals({},s,2.,.45,0,.05);
  assert(out.safety_velocity_limited && std::abs(out.v_target-.425)<1e-12);
  assert(std::abs(applyCoreVelocityLimit(.45,out)-.425)<1e-12);
  assert(std::abs(applyCandidateVelocityLimit(.45,out)-.425)<1e-12);
  s=DecisionState{};s.c_star.set(7,Side::R);
  out=core.step_evals({},s,2.,.45,0,.10);
  assert(std::abs(out.v_target-.4)<1e-12);
  s=DecisionState{};s.c_star.set(7,Side::R);
  // All unavailable: acceleration .5 over .05 s suffices to stop .02 m/s.
  out=core.step_evals({},s,2.,.02,0,.05);
  assert(out.mode==Mode::NORMAL && out.safety_velocity_limited && out.v_target==0);
  assert(applyCoreVelocityLimit(.45,out)==0);
  // A small but nonzero bound must not be mistaken for startup velocity.
  s=DecisionState{};s.c_star.set(7,Side::R);
  out=core.step_evals({},s,2.,.055,0,.05);
  assert(out.safety_velocity_limited && std::abs(applyCoreVelocityLimit(.45,out)-.03)<1e-12);
  // An ordinary safe decision at rest still permits cruise startup.
  s=DecisionState{};s.c_star.set(7,Side::R);
  ClassEval e;e.cls=s.c_star;e.J=.5;e.safe=true;e.available=true;
  out=core.step_evals({e},s,5.,0.,0,.05);
  assert(!out.safety_velocity_limited && applyCoreVelocityLimit(.45,out)==.45);
  out.v_target=.2;
  assert(!out.safety_velocity_limited && applyCandidateVelocityLimit(.45,out)==.45);
  out.safety_velocity_limited=true;
  assert(applyCandidateVelocityLimit(.45,out)==.2);
  out.safety_velocity_limited=false;
  out.mode=Mode::STOP;assert(applyCoreVelocityLimit(.45,out)==0);
  out.mode=Mode::HOLD;assert(applyCoreVelocityLimit(.45,out)==0);
  out.mode=Mode::NORMAL;out.safety_velocity_limited=true;
  out.v_target=std::numeric_limits<double>::quiet_NaN();assert(applyCoreVelocityLimit(.45,out)==0);
  std::cout<<"safety velocity contract: acceleration*dt, zero/small bounds, startup, STOP/HOLD, nonfinite pass\n";
}
