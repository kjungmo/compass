#!/usr/bin/env python3
"""반응형 보행자 노드 (compass_sim).

Gazebo 세계의 ``pedestrian`` 모델(자체 DiffDrive)을 구동한다. 로봇의 직진
경로(복도 중심선 y≈0)를 ``x≈cross_x`` 지점에서 횡단하도록 좌우 경유점을
왕복한다 — 즉 한쪽(예: 남쪽 y<0)에서 출발해 반대쪽(북쪽 y>0)으로 건너간 뒤
다시 돌아온다. 이렇게 하면 로봇이 +x 로 전진해 ``x≈cross_x`` 에 도달할 무렵
보행자가 그 앞을 가로지르게 되어, 컨트롤러가 좌/우 측면 결정을 내리도록
강제된다.

로봇이 반경 ``avoid_radius`` 안으로 들어오면 약하게 비켜서되, 횡단 목표를
포기하고 복도를 따라 달아나지 않도록 회피 비중을 ``avoid_gain`` 으로 제한한다
(횡단 commitment 유지). 순수 반응 규칙이며 계획기가 없다 — 테스트베드 스모크용
최소 구현.

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
        self.declare_parameter("avoid_radius", 1.2)
        self.declare_parameter("walk_speed", 0.5)
        self.declare_parameter("max_yaw_rate", 1.5)
        # 회피가 횡단을 이기지 못하도록 제한하는 비중 상한 (0..1).
        self.declare_parameter("avoid_gain", 0.4)
        # 로봇 직진 경로(복도 중심선)를 가로지르는 횡단 경유점.
        # 남(y<0) <-> 북(y>0) 왕복. 횡단선 x 는 로봇이 cruise 로 진입할 여유(runway)를
        # 두도록 x=4.0 에 둔다. (기둥은 (2.0,3.0)으로 옮겨 복도 중심선을 비웠다.)
        self.declare_parameter("waypoints_x", [4.0, 4.0])
        self.declare_parameter("waypoints_y", [2.0, -2.0])

        self.avoid_radius = float(self.get_parameter("avoid_radius").value)
        self.walk_speed = float(self.get_parameter("walk_speed").value)
        self.max_yaw_rate = float(self.get_parameter("max_yaw_rate").value)
        self.avoid_gain = float(self.get_parameter("avoid_gain").value)
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
            f"reactive_pedestrian 시작: 횡단 x={self.waypoints[0][0]} m, "
            f"avoid_radius={self.avoid_radius} m, walk_speed={self.walk_speed} m/s")

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
                # 가까울수록 회피 비중을 키우되 avoid_gain 으로 상한을 둔다.
                # 횡단 목표가 죽지 않도록 회피는 보조항으로만 섞는다.
                w = max(0.0, min(1.0, (self.avoid_radius - d) / self.avoid_radius))
                w *= self.avoid_gain
                gx = (1 - w) * math.cos(goal_heading) + w * math.cos(away)
                gy = (1 - w) * math.sin(goal_heading) + w * math.sin(away)
                goal_heading = math.atan2(gy, gx)
                speed = self.walk_speed * (1.0 - 0.3 * w)  # 가까우면 약간 감속

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
