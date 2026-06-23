#!/usr/bin/env python3
"""반응형 보행자 노드 (compass_sim).

Gazebo 세계의 ``pedestrian`` 모델(자체 DiffDrive)을 구동한다. 평상시에는
지정한 순찰 경유점(patrol waypoints)을 순회하지만, 로봇이 반경
``avoid_radius`` (기본 2.0 m) 안으로 들어오면 로봇 반대 방향으로 비켜선다.
순수 반응 규칙이며 계획기가 없다 — 테스트베드 스모크용 최소 구현.

입력:  /odom (로봇), ped/odom (보행자)   — nav_msgs/Odometry
출력:  ped/cmd_vel                       — geometry_msgs/Twist
"""
import math

import rclpy
from rclpy.node import Node
from geometry_msgs.msg import Twist
from nav_msgs.msg import Odometry


def yaw_from_quat(q) -> float:
    """쿼터니언에서 yaw 추출."""
    siny = 2.0 * (q.w * q.z + q.x * q.y)
    cosy = 1.0 - 2.0 * (q.y * q.y + q.z * q.z)
    return math.atan2(siny, cosy)


def ang_diff(a: float, b: float) -> float:
    """[-pi, pi) 로 감싼 각도 차."""
    return math.atan2(math.sin(a - b), math.cos(a - b))


class ReactivePedestrian(Node):
    def __init__(self) -> None:
        super().__init__("reactive_pedestrian")
        self.declare_parameter("avoid_radius", 2.0)
        self.declare_parameter("walk_speed", 0.4)
        self.declare_parameter("max_yaw_rate", 1.2)
        # 복도를 가로지르는 순찰 경유점 (로봇 진행 경로 위를 왕복).
        self.declare_parameter("waypoints_x", [1.0, 1.0])
        self.declare_parameter("waypoints_y", [2.5, -2.5])

        self.avoid_radius = float(self.get_parameter("avoid_radius").value)
        self.walk_speed = float(self.get_parameter("walk_speed").value)
        self.max_yaw_rate = float(self.get_parameter("max_yaw_rate").value)
        wx = list(self.get_parameter("waypoints_x").value)
        wy = list(self.get_parameter("waypoints_y").value)
        self.waypoints = list(zip(wx, wy))
        self.wp_idx = 0

        self.robot = None   # (x, y)
        self.ped = None     # (x, y, yaw)

        self.create_subscription(Odometry, "/odom", self._robot_cb, 10)
        self.create_subscription(Odometry, "ped/odom", self._ped_cb, 10)
        self.pub = self.create_publisher(Twist, "ped/cmd_vel", 10)
        self.create_timer(0.1, self._tick)
        self.get_logger().info(
            f"reactive_pedestrian 시작: avoid_radius={self.avoid_radius} m, "
            f"walk_speed={self.walk_speed} m/s")

    def _robot_cb(self, msg: Odometry) -> None:
        p = msg.pose.pose.position
        self.robot = (p.x, p.y)

    def _ped_cb(self, msg: Odometry) -> None:
        p = msg.pose.pose.position
        self.ped = (p.x, p.y, yaw_from_quat(msg.pose.pose.orientation))

    def _tick(self) -> None:
        if self.ped is None:
            return
        px, py, pyaw = self.ped

        # 목표 방향: 기본은 현재 경유점, 로봇이 가까우면 회피 방향과 혼합.
        tx, ty = self.waypoints[self.wp_idx]
        if math.hypot(tx - px, ty - py) < 0.4:
            self.wp_idx = (self.wp_idx + 1) % len(self.waypoints)
            tx, ty = self.waypoints[self.wp_idx]

        goal_heading = math.atan2(ty - py, tx - px)
        speed = self.walk_speed

        if self.robot is not None:
            rx, ry = self.robot
            d = math.hypot(rx - px, ry - py)
            if d < self.avoid_radius:
                # 로봇에서 멀어지는 방향 (밀어내기 벡터).
                away = math.atan2(py - ry, px - rx)
                # 가까울수록 회피 비중을 키운다 (0..1).
                w = max(0.0, min(1.0, (self.avoid_radius - d) / self.avoid_radius))
                gx = (1 - w) * math.cos(goal_heading) + w * math.cos(away)
                gy = (1 - w) * math.sin(goal_heading) + w * math.sin(away)
                goal_heading = math.atan2(gy, gx)
                speed = self.walk_speed * (1.0 - 0.5 * w)  # 가까우면 감속

        err = ang_diff(goal_heading, pyaw)
        cmd = Twist()
        cmd.angular.z = max(-self.max_yaw_rate, min(self.max_yaw_rate, 2.0 * err))
        # 정렬되었을 때만 전진 (제자리 회전 우선).
        cmd.linear.x = speed * max(0.0, math.cos(err))
        self.pub.publish(cmd)


def main(args=None) -> None:
    rclpy.init(args=args)
    node = ReactivePedestrian()
    try:
        rclpy.spin(node)
    except KeyboardInterrupt:
        pass
    finally:
        node.destroy_node()
        if rclpy.ok():
            rclpy.shutdown()


if __name__ == "__main__":
    main()
