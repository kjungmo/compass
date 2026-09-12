#pragma once
#include "compass_core/types.hpp"
#include <algorithm>
#include <cmath>
#include <limits>
#include <string>
namespace compass_nav2 {
// Frozen path-normal projection for one commitment epoch, independent of ROS.
struct ProgressSample { bool valid=false; double delta_m=0; const char* reason="uninitialized"; };
class MeasuredProgress {
 public:
  void reset() { initialized_=false; }
  ProgressSample sample(const compass::SE2& pose, double stamp, const std::string& frame,
      const compass::TopoClass& cls, const std::vector<compass::Point2D>& path,
      double max_gap=.25, double max_speed=2.) {
    if(!std::isfinite(pose.x)||!std::isfinite(pose.y)||!std::isfinite(stamp)||
       frame.empty()||!std::isfinite(max_gap)||max_gap<=0||
       !std::isfinite(max_speed)||max_speed<=0) {reset();return {false,0,"invalid_input"};}
    if(!initialized_ || !cls.equals(cls_) || frame!=frame_) {
      // An epoch change establishes a fresh anchor; never join different frames/classes.
      double sign=0;
      for(const auto& p:cls.pairs()) sign+=(p.second==compass::Side::L?1.:-1.);
      if(!cls.pairs().empty() && sign==0) {reset();return {false,0,"mixed_side_class"};}
      nx_=ny_=0;
      if(!cls.pairs().empty()) {
        double best=std::numeric_limits<double>::infinity();bool found=false;
        for(size_t i=1;i<path.size();++i) {
          const auto a=path[i-1],b=path[i];const double dx=b.x-a.x,dy=b.y-a.y,l2=dx*dx+dy*dy;
          if(!std::isfinite(l2)||l2<1e-12) continue;
          const double u=std::clamp(((pose.x-a.x)*dx+(pose.y-a.y)*dy)/l2,0.,1.);
          const double d=std::hypot(pose.x-a.x-u*dx,pose.y-a.y-u*dy);
          if(d<best) {best=d;found=true;const double side=sign>0?1.:-1.;nx_=-side*dy/std::sqrt(l2);ny_=side*dx/std::sqrt(l2);}
        }
        if(!found) {reset();return {false,0,"missing_path_tangent"};}
      }
      cls_=cls;frame_=frame;last_=pose;stamp_=stamp;initialized_=true;
      return {true,0,"anchor"};
    }
    const double dt=stamp-stamp_,dx=pose.x-last_.x,dy=pose.y-last_.y;
    if(dt<=0||dt>max_gap||std::hypot(dx,dy)>max_speed*dt) {
      reset();return {false,0,"stale_clock_or_pose_jump"};
    }
    last_=pose;stamp_=stamp;
    return {true,dx*nx_+dy*ny_,"measured"};
  }
 private:
  bool initialized_=false;
  compass::SE2 last_;compass::TopoClass cls_;std::string frame_;
  double stamp_=0,nx_=0,ny_=0;
};
}
