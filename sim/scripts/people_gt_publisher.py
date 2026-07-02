#!/usr/bin/env python3
"""보행자 지상 진실 /people 발행 노드 (compass_sim).

Gazebo 의 ``pedestrian`` 모델 상태(``ped/odom``)를 읽어, COMPASS 컨트롤러가
구독하는 ``/people`` (compass_msgs/People) 로 ~10 Hz 재발행한다. 외부 사람
트래커 대신 시뮬레이터의 지상 진실을 그대로 흘려 컨트롤러의 사람 입력 경로를
스모크로 검증하기 위한 최소 노드다.

좌표계: ``odom`` (costmap global_frame 와 동일). 속도는 월드 프레임 (vx, vy) 로
변환해 싣는다 — compass_nav2::toPersons 가 헤딩+선속으로 환산한다.

입력:  ped/odom    — nav_msgs/Odometry (보행자 자세 + body-frame twist)
출력:  /people     — compass_msgs/People (사람 1명: 보행자)
"""
import math

import rclpy
from rclpy.node import Node
from nav_msgs.msg import Odometry
from compass_msgs.msg import People, Person


def yaw_from_quat(q) -> float:
    """쿼터니언에서 yaw 추출."""
    siny = 2.0 * (q.w * q.z + q.x * q.y)
    cosy = 1.0 - 2.0 * (q.y * q.y + q.z * q.z)
    return math.atan2(siny, cosy)


def stable_id(name: str) -> int:
    """이름을 64비트 안정 해시(식별자)로."""
    h = 1469598103934665603  # FNV-1a 64-bit offset basis
    for ch in name.encode("utf-8"):
        h ^= ch
        h = (h * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


class PeopleGtPublisher(Node):
    def __init__(self) -> None:
        super().__init__("people_gt_publisher")
        self.declare_parameter("frame_id", "odom")
        self.declare_parameter("person_name", "pedestrian")
        self.declare_parameter("rate_hz", 10.0)

        self.frame_id = str(self.get_parameter("frame_id").value)
        self.person_name = str(self.get_parameter("person_name").value)
        rate = float(self.get_parameter("rate_hz").value)
        self.person_id = stable_id(self.person_name)

        self.ped = None  # (x, y, yaw, vx_world, vy_world)

        self.create_subscription(Odometry, "ped/odom", self._ped_cb, 10)
        self.pub = self.create_publisher(People, "/people", 10)
        self.create_timer(1.0 / max(rate, 1e-3), self._tick)
        self.get_logger().info(
            f"people_gt_publisher 시작: /people <- ped/odom, frame={self.frame_id}, "
            f"id={self.person_id}, {rate:.0f} Hz")

    def _ped_cb(self, msg: Odometry) -> None:
        p = msg.pose.pose.position
        yaw = yaw_from_quat(msg.pose.pose.orientation)
        # Odometry twist 는 body 프레임 — yaw 로 월드 프레임으로 회전.
        vbx = msg.twist.twist.linear.x
        vby = msg.twist.twist.linear.y
        vwx = math.cos(yaw) * vbx - math.sin(yaw) * vby
        vwy = math.sin(yaw) * vbx + math.cos(yaw) * vby
        self.ped = (p.x, p.y, yaw, vwx, vwy)

    def _tick(self) -> None:
        if self.ped is None:
            return
        px, py, _yaw, vwx, vwy = self.ped

        person = Person()
        person.id = self.person_id
        person.x = px
        person.y = py
        # theta 는 toPersons 가 속도로부터 다시 계산하므로 진행 방향으로 채워 둔다.
        person.theta = math.atan2(vwy, vwx)
        person.vx = vwx
        person.vy = vwy
        person.covariance = [0.04, 0.0, 0.0, 0.04]

        msg = People()
        msg.header.stamp = self.get_clock().now().to_msg()
        msg.header.frame_id = self.frame_id
        msg.people = [person]
        self.pub.publish(msg)


def main(args=None) -> None:
    rclpy.init(args=args)
    node = PeopleGtPublisher()
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
