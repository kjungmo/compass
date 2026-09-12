#include "compass_eval/harness.hpp"
#ifdef NDEBUG
#undef NDEBUG
#endif
#include <cassert>
#include <iostream>
using namespace compass;
using namespace compass_eval;
static void same(const DecisionState&a,const DecisionState&b) {
  assert(a.c_star.equals(b.c_star));assert(a.e_rev==b.e_rev && a.e_fwd==b.e_fwd);
  assert(a.rho==b.rho && a.L_real==b.L_real && a.L_plan==b.L_plan);
  assert(a.mode==b.mode && a.n_thrash==b.n_thrash && a.t_safe_dwell==b.t_safe_dwell);
  assert(a.safe_switch_times==b.safe_switch_times);
  assert(a.prev_c_prime.has_value()==b.prev_c_prime.has_value());
  if(a.prev_c_prime)assert(a.prev_c_prime->equals(*b.prev_c_prime));
}
int main(){
  size_t n=0;bool saw_safety=false,saw_commit=false,saw_no_challenger=false;
  for(auto scn:all_scns())for(unsigned seed=0;seed<50;++seed){
    auto input=make_scenario(scn,seed);DecisionCore a{Knobs{}},b{Knobs{}};
    DecisionState x,y;x.c_star.set(PID,Side::R);y=x;
    for(size_t i=0;i<input.size();++i){auto c=input[i];
      std::vector<ClassEval> e={mk(Side::R,c.J_R,true),mk(Side::L,c.J_L,true)};
      // Late injected safety and single-candidate conditions cover early returns.
      if(i>=180&&i<185){e[0].safe=false;e[1].safe=false;}
      if(i>=190){e={mk(x.c_star.side(PID).value(),.5,true)};}
      DecisionTrace t;
      auto ox=a.step_evals(e,x,5,c.v_in,i*DT,DT);
      auto oy=b.step_evals(e,y,5,c.v_in,i*DT,DT,&t);
      same(x,y);same(t.after,y);assert(ox.v_target==oy.v_target);
      assert(t.switched==!t.before.c_star.equals(t.after.c_star));
      if(t.commit_reset){saw_commit=true;assert(t.evidence_after_accumulate.value()>=t.threshold_used.value());assert(t.after.e_rev==0);}
      if(t.safety_branch){saw_safety=true;assert(!t.threshold_used);}
      if(!t.safety_branch&&!t.challenger)saw_no_challenger=true;
      ++n;
    }
  }
  assert(saw_safety&&saw_commit&&saw_no_challenger);
  std::cout<<n<<" traced/untraced cycles equal; commit, safety, no-challenger covered\n";
}
