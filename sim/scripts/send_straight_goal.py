#!/usr/bin/env python3
"""직선 FollowPath 목표 송신 노드 (compass_sim).

COMPASS 컨트롤러(``compass_nav2::CompassController``)는 ``FollowPath`` 경로가
설정될 때만 ``/cmd_vel`` 을 산출하므로, 빈 복도 직진 스모크를 한 명령으로
재현하기 위한 최소 목표 송신기다. 로봇의 현재 ``/odom`` 위치에서 출발해
세계 +x 축을 따라 곧게 뻗는 경로(기본 4 m, 0.1 m 간격)를 만들어
``/follow_path`` 액션으로 보낸다.

경로 좌표계는 컨트롤러 costmap 의 ``global_frame`` 과 같아야 한다(스모크
구성에서는 ``odom``). 각 자세의 헤딩은 +x(yaw=0)로 둔다.

용례 (빈 복도 직진 검증):
  ros2 launch compass_sim sim_bringup.launch.py headless:=true use_pedestrian:=false
  ros2 run   compass_sim send_straight_goal              # 다른 터미널

파라미터:
  length (m, 기본 4.0)   — 직선 경로 길이.
  step   (m, 기본 0.1)   — 자세 간격.
  frame_id (기본 odom)   — 경로 프레임 (컨트롤러 global_frame 와 일치).
  anchor_to_odom (기본 True) — True 면 현재 /odom 위치에서 시작, 실패 시 원점.
"""
import math

import rclpy
from rclpy.action import ActionClient
from rclpy.node import Node

from nav_msgs.msg import Odometry, Path
from geometry_msgs.msg import PoseStamped
from nav2_msgs.action import FollowPath


class StraightGoalSender(Node):
    def __init__(self):
        super().__init__("send_straight_goal")
        self.declare_parameter("length", 4.0)
        self.declare_parameter("step", 0.1)
        self.declare_parameter("frame_id", "odom")
        self.declare_parameter("anchor_to_odom", True)
        self.declare_parameter("y_offset", 0.0)   # 경로를 로봇 횡으로 띄워
        self.length = float(self.get_parameter("length").value)   # cross-track 보정을 시험.
        self.step = float(self.get_parameter("step").value)
        self.frame_id = str(self.get_parameter("frame_id").value)
        self.anchor = bool(self.get_parameter("anchor_to_odom").value)
        self.y_offset = float(self.get_parameter("y_offset").value)

        # 시작점을 현재 /odom 으로 앵커링 (한 건만 받으면 충분).
        self.start = (0.0, 0.0)
        if self.anchor:
            self._got_odom = False
            self.create_subscription(Odometry, "odom", self._odom_cb, 1)
            # 최대 ~3 s 동안 odom 한 건을 기다린다.
            for _ in range(30):
                rclpy.spin_once(self, timeout_sec=0.1)
                if self._got_odom:
                    break
            if not self._got_odom:
                self.get_logger().warn("odom 미수신 — 원점(0,0)에서 경로를 시작합니다.")

        self.client = ActionClient(self, FollowPath, "follow_path")

    def _odom_cb(self, msg: Odometry):
        self.start = (msg.pose.pose.position.x, msg.pose.pose.position.y)
        self._got_odom = True

    def build_path(self) -> Path:
        path = Path()
        path.header.frame_id = self.frame_id
        path.header.stamp = self.get_clock().now().to_msg()
        x0, y0 = self.start
        y0 += self.y_offset
        n = max(2, int(round(self.length / self.step)) + 1)
        for i in range(n):
            ps = PoseStamped()
            ps.header.frame_id = self.frame_id
            ps.pose.position.x = x0 + i * self.step
            ps.pose.position.y = y0
            ps.pose.orientation.w = 1.0   # yaw = 0 (+x 방향)
            path.poses.append(ps)
        return path

    def send(self):
        if not self.client.wait_for_server(timeout_sec=10.0):
            self.get_logger().error("follow_path 액션 서버를 찾지 못했습니다.")
            return False
        goal = FollowPath.Goal()
        goal.path = self.build_path()
        x0, y0 = self.start
        self.get_logger().info(
            f"직선 경로 송신: ({x0:.2f},{y0:.2f}) -> ({x0 + self.length:.2f},{y0:.2f}), "
            f"{len(goal.path.poses)} 점, frame={self.frame_id}.")
        fut = self.client.send_goal_async(goal)
        rclpy.spin_until_future_complete(self, fut)
        handle = fut.result()
        if not handle or not handle.accepted:
            self.get_logger().error("목표가 거부되었습니다.")
            return False
        self.get_logger().info("목표 수락됨 — 컨트롤러가 /cmd_vel 산출을 시작합니다.")
        result_fut = handle.get_result_async()
        rclpy.spin_until_future_complete(self, result_fut)
        self.get_logger().info("FollowPath 종료.")
        return True


def main():
    rclpy.init()
    node = StraightGoalSender()
    try:
        node.send()
    finally:
        node.destroy_node()
        rclpy.shutdown()


if __name__ == "__main__":
    main()
