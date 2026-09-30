#include "compass_eval/harness.hpp"
#include "compass_core/response_profile.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <limits>
#include <iostream>
using namespace compass;
using namespace compass_eval;
int main() {
  const auto k=responsive_profile();
  const auto bound=response_cycle_bound(.4,DT,k);
  assert(bound && *bound<=51);
  assert(!response_cycle_bound(.4,DT,Knobs{}));
  assert(!response_cycle_bound(.1,DT,k));
  assert(!response_cycle_bound(.4,0,k));
  auto cap_only=Knobs{};cap_only.e_max_rev=.7;
  assert(!response_cycle_bound(.4,DT,cap_only));
  // Worst progress, persistent safe challenger and sustained advantage.
  for(double rho:{0.,.5,1.}) {
    DecisionCore core{k};DecisionState s;s.c_star.set(PID,Side::R);s.rho=rho;s.L_real=rho*s.L_plan;
    unsigned first=0;
    for(unsigned i=0;i<*bound;++i) {
      DecisionTrace t;
      core.step_evals({mk(Side::R,.6,true),mk(Side::L,.2,true)},s,5,.5,i*DT,DT,&t,.025);
      if(t.commit_reset){first=i+1;assert(s.L_real==0 && s.rho==0);break;}
    }
    assert(first>0 && first<=*bound);
  }
  // Command speed must not advance measured lateral progress; retreat is signed.
  DecisionCore core{k};DecisionState s;s.c_star.set(PID,Side::R);
  const std::vector<ClassEval> tie={mk(Side::R,.5,true),mk(Side::L,.5,true)};
  core.step_evals(tie,s,5,.5,0,DT,nullptr,0.);
  assert(s.L_real==0 && s.rho==0);
  core.step_evals(tie,s,5,.5,DT,DT,nullptr,.2);
  core.step_evals(tie,s,5,.5,2*DT,DT,nullptr,-.1);
  assert(std::abs(s.L_real-.1)<1e-12);
  core.step_evals(tie,s,5,.5,3*DT,DT,nullptr,-1.);
  assert(s.L_real==0);
  bool rejected=false;
  try{core.step_evals(tie,s,5,.5,4*DT,DT,nullptr,std::numeric_limits<double>::quiet_NaN());}
  catch(const std::invalid_argument&){rejected=true;}
  assert(rejected && s.L_real==0);
  std::cout<<"response bound, cap-only counterexample, measured zero/retreat/reset/NaN passed; bound="<<*bound<<" cycles\n";
}
