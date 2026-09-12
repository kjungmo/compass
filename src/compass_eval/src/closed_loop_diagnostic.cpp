// Mechanism diagnostic: actual decision core + unicycle plant, scripted costs.
// This is NOT the Nav2 controller, Gazebo physics or an independent physical oracle.
#include "compass_eval/harness.hpp"
#include "compass_core/response_profile.hpp"
#include "compass_nav2/measured_progress.hpp"
#include "compass_nav2/path_tracking.hpp"
#include <fstream>
#include <iomanip>
#include <iostream>
#include <stdexcept>
using namespace compass;
using namespace compass_eval;
int main(int argc,char**argv) {
  if(argc!=3){std::cerr<<"usage: closed_loop_diagnostic legacy_proxy|legacy_measured|responsive_measured output.jsonl\n";return 1;}
  std::string profile=argv[1];
  if(profile!="legacy_proxy"&&profile!="legacy_measured"&&profile!="responsive_measured")return 1;
  Knobs k=profile=="responsive_measured"?responsive_profile():Knobs{};
  DecisionCore core{k};DecisionState st;st.c_star.set(PID,Side::R);
  SE2 robot;compass_nav2::MeasuredProgress progress;
  std::vector<Point2D> axis={{0,0},{8,0}};
  std::ofstream f(argv[2]);if(!f)return 1;f<<std::setprecision(17)<<std::boolalpha;
  double last_v=0;constexpr double dt=.05;
  for(int i=0;i<1200;++i) {
    const double t=i*dt;std::optional<double> delta;
    if(profile!="legacy_proxy") {
      const auto p=progress.sample(robot,t,"world",st.c_star,axis);
      if(!p.valid){std::cerr<<p.reason<<'\n';return 2;}delta=p.delta_m;
    }
    // Safe routes to two endpoints; after t=4 the declared target is the upper endpoint.
    // Candidate costs are scripted, not outputs of CostEvaluator::J.
    DecisionTrace trace;
    core.step_evals({mk(Side::R,t<4?.2:.6,true),mk(Side::L,t<4?.6:.2,true)},
                    st,10,last_v,t,dt,&trace,delta);
    const bool left=st.c_star.side(PID)==Side::L;
    const Point2D endpoint{8,left?.6:-.6};
    const double local_dist=std::hypot(endpoint.x-robot.x,endpoint.y-robot.y);
    compass_nav2::PathTrackGains gains;gains.k_theta=2.;gains.max_w=1.;
    const double w=compass_nav2::pathTrackingAngularZ(robot,{endpoint},0,gains);
    const double err=std::atan2(endpoint.y-robot.y,endpoint.x-robot.x)-robot.theta;
    const double v=std::min(.45,.8*local_dist)*std::max(0.,std::cos(err));
    const double before_dist=std::hypot(8-robot.x,.6-robot.y);
    // Exact unicycle integration for constant v,w over each interval.
    const double old_y=robot.y;
    if(std::abs(w)<1e-12){robot.x+=v*std::cos(robot.theta)*dt;robot.y+=v*std::sin(robot.theta)*dt;}
    else {robot.x+=v/w*(std::sin(robot.theta+w*dt)-std::sin(robot.theta));robot.y-=v/w*(std::cos(robot.theta+w*dt)-std::cos(robot.theta));}
    robot.theta+=w*dt;last_v=v;
    const double dist=std::hypot(8-robot.x,.6-robot.y);
    // Conservative interval clearance lower bound for walls at y=+-2, radius=.25.
    const double clearance=2.-std::abs(old_y)-v*dt-.25;
    const bool collision=clearance<0,goal=t>=4&&dist<=.3;
    f<<"{\"cycle\":"<<i<<",\"t\":"<<t<<",\"dt\":"<<dt<<",\"speed_mps\":"<<v
     <<",\"goal_distance_m\":"<<std::min(before_dist,dist)<<",\"min_clearance_m\":"<<clearance
     <<",\"free_motion_warranted\":"<<(t>=4)<<",\"planned_stop\":false,\"collision\":"<<collision
     <<",\"goal_reached\":"<<goal<<",\"x\":"<<robot.x<<",\"y\":"<<robot.y
     <<",\"side\":"<<(left?0:1)<<",\"rho\":"<<st.rho<<",\"switched\":"<<trace.switched<<"}\n";
    if(collision||goal)break;
  }
  f.flush();return f?0:1;
}
