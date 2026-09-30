#include "compass_eval/harness.hpp"
#include <fstream>
#include "compass_core/response_profile.hpp"
#include <iomanip>
#include <iostream>
#include <locale>
#include <stdexcept>
using namespace compass;
using namespace compass_eval;
static void cls(std::ostream& o, const TopoClass& c) {
  o << '['; bool first=true;
  for (const auto& p:c.pairs()) { if(!first)o<<','; first=false;
    o << '[' << p.first << ',' << static_cast<int>(p.second) << ']'; }
  o << ']';
}
static void num(std::ostream& o, std::optional<double> x) {
  if(x && std::isfinite(*x)) o<<*x; else o<<"null";
}
int main(int argc, char** argv) {
  try {
    if(argc!=5 && argc!=6 && argc!=7) throw std::runtime_error("usage: compass_trace scenario seed v_input output.jsonl [legacy|responsive] [measured_progress_mps]");
    Scn scn=Scn::NearTie; bool found=false;
    for(auto x:all_scns()) if(std::string(argv[1])==scn_name(x)){scn=x;found=true;}
    if(!found) throw std::runtime_error("unknown scenario");
    std::size_t end=0; auto seed=std::stoul(argv[2], &end);
    if(end!=std::string(argv[2]).size() || seed>UINT32_MAX || argv[2][0]=='-')
      throw std::runtime_error("invalid seed");
    double v=std::stod(argv[3], &end);
    if(end!=std::string(argv[3]).size() || !std::isfinite(v) || v<0)
      throw std::runtime_error("invalid v_input");
    std::ofstream o(argv[4]); if(!o)throw std::runtime_error("cannot open output");
    o.imbue(std::locale::classic()); o<<std::setprecision(17)<<std::boolalpha;
    auto cycles=make_scenario(scn,static_cast<uint32_t>(seed),v);
    Knobs knobs;
    if (argc>=6) {
      const std::string profile=argv[5];
      if (profile=="responsive") knobs=responsive_profile();
      else if (profile!="legacy") throw std::runtime_error("unknown profile");
    }
    std::optional<double> delta;
    if(argc==7) {
      const double rate=std::stod(argv[6], &end);
      if(end!=std::string(argv[6]).size() || !std::isfinite(rate))
        throw std::runtime_error("invalid measured progress rate");
      delta=rate*DT;
    }
    DecisionCore core{knobs}; DecisionState st; st.c_star.set(PID,Side::R);
    for(size_t i=0;i<cycles.size();++i) {
      auto c=cycles[i]; DecisionTrace t;
      core.step_evals({mk(Side::R,c.J_R,c.safe_R),mk(Side::L,c.J_L,c.safe_L)},
                     st,c.ttc,c.v_in,i*DT,DT,&t,delta);
      o<<"{\"schema\":1,\"cycle\":"<<i<<",\"t\":"<<t.now<<",\"dt\":"<<t.dt
       <<",\"before\":";cls(o,t.before.c_star);o<<",\"class\":";cls(o,t.after.c_star);
      o<<",\"challenger\":";if(t.challenger)cls(o,*t.challenger);else o<<"null";
      o<<",\"measured_progress_delta_m\":";num(o,t.measured_progress_delta_m);
      o<<",\"advantage\":";num(o,t.advantage);
      o<<",\"e_before\":"<<t.before.e_rev<<",\"e_after_challenger_reset\":";num(o,t.evidence_after_challenger_reset);
      o<<",\"e_accumulated\":";num(o,t.evidence_after_accumulate);
      o<<",\"threshold\":";num(o,t.threshold_used);o<<",\"rho_used\":";num(o,t.rho_used);
      o<<",\"e_after\":"<<t.after.e_rev<<",\"rho_after\":"<<t.after.rho
       <<",\"l_real_before\":"<<t.before.L_real<<",\"l_real_after\":"<<t.after.L_real
       <<",\"l_plan\":"<<t.after.L_plan<<",\"v_input\":"<<t.v_input<<",\"v_output\":"<<t.v_output
       <<",\"mode\":"<<static_cast<int>(t.after.mode)<<",\"safety\":"<<t.safety_branch
       <<",\"challenger_reset\":"<<t.challenger_reset<<",\"commit_reset\":"<<t.commit_reset
       <<",\"switched\":"<<t.switched<<",\"candidates\":[";
      bool first=true;for(const auto& e:t.candidates){if(!first)o<<',';first=false;
        o<<"{\"class\":";cls(o,e.cls);o<<",\"cost\":";num(o,e.J);
        o<<",\"available\":"<<e.available<<",\"safe\":"<<e.safe<<'}';}
      o<<"]}\n";
    }
    o.flush();if(!o)throw std::runtime_error("output write failed");
  } catch(const std::exception& e){std::cerr<<e.what()<<'\n';return 1;}
}
