#pragma once
#include "compass_core/env_query.hpp"
#include "compass_nav2/path_tracking.hpp"
#include <cmath>

namespace compass_nav2 {
// Bounded research rollout of the existing tracking command. It does not certify
// homotopy or collision freedom. Mixed-side maneuvers need a separate generator.
inline compass::CandidateTrajectory candidateRollout(
    const compass::SE2& robot, const std::vector<compass::Point2D>& path,
    const compass::TopoClass& cls, double speed, const PathTrackGains& gains) {
  if (path.size()<2 || !std::isfinite(speed) || speed<0 ||
      !std::isfinite(robot.x)||!std::isfinite(robot.y)||!std::isfinite(robot.theta) ||
      !std::isfinite(gains.k_e)||gains.k_e<0 ||
      !std::isfinite(gains.k_theta)||gains.k_theta<0 ||
      !std::isfinite(gains.k_side)||gains.k_side<0 ||
      !std::isfinite(gains.max_w)||gains.max_w<=0) return {};
  bool segment=false;
  for (size_t i=0;i<path.size();++i) {
    if (!std::isfinite(path[i].x)||!std::isfinite(path[i].y)) return {};
    if(i && std::hypot(path[i].x-path[i-1].x,path[i].y-path[i-1].y)>1e-6)
      segment=true;
  }
  if(!segment) return {};
  double side=0;
  for(const auto& pair:cls.pairs()) {
    const double next=pair.second==compass::Side::L?1.:-1.;
    if(side && side!=next) return {};
    side=next;
  }
  compass::CandidateTrajectory result;
  compass::SE2 p=robot;
  constexpr double dt=.05;
  for(int i=0;i<=20;++i) {
    const double w=pathTrackingAngularZ(p,path,side,gains);
    if(!std::isfinite(w)) return {};
    result.push_back({i*dt,p,{speed,w}});
    if(i==20) break;
    if(std::abs(w)<1e-12) {
      p.x+=speed*std::cos(p.theta)*dt;p.y+=speed*std::sin(p.theta)*dt;
    } else {
      p.x+=speed/w*(std::sin(p.theta+w*dt)-std::sin(p.theta));
      p.y-=speed/w*(std::cos(p.theta+w*dt)-std::cos(p.theta));
    }
    p.theta=std::remainder(p.theta+w*dt,2*std::acos(-1.));
  }
  return result;
}
}
