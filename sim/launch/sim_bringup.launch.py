"""COMPASS 시뮬레이션 브링업 (Gazebo Harmonic + Nav2 controller_server).

구성 요소:
  * gz sim          — compass_office.sdf 세계 (기본 headless, 서버 전용 `-s`)
  * robot_state_publisher — diff_bot.urdf.xacro
  * ros_gz_sim create     — 로봇 스폰
  * ros_gz_bridge         — /cmd_vel→sim, /odom·/scan·/tf·ped/*←sim
  * reactive_pedestrian   — 반응형 보행자 노드
  * nav2 controller_server + lifecycle_manager — compass_nav2::CompassController 적재

런치 인자:
  headless (기본 true)  — true 면 `gz sim -s` (렌더/GUI 없음, WSL2 GPU 회피).
                          false 면 GUI 포함 `gz sim` 실행.
  use_nav2 (기본 true)  — controller_server + lifecycle_manager 적재 여부.
  use_pedestrian (true) — 반응형 보행자 노드 실행 여부.
  world / params_file   — 세계 SDF / Nav2 파라미터 경로 재정의.
"""
import os

from ament_index_python.packages import get_package_share_directory
from launch import LaunchDescription
from launch.actions import DeclareLaunchArgument, IncludeLaunchDescription
from launch.conditions import IfCondition
from launch.launch_description_sources import PythonLaunchDescriptionSource
from launch.substitutions import LaunchConfiguration, PythonExpression
from launch_ros.actions import Node
import xacro


def generate_launch_description() -> LaunchDescription:
    pkg_share = get_package_share_directory("compass_sim")
    default_world = os.path.join(pkg_share, "worlds", "compass_office.sdf")
    xacro_file = os.path.join(pkg_share, "urdf", "diff_bot.urdf.xacro")
    bridge_yaml = os.path.join(pkg_share, "config", "ros_gz_bridge.yaml")
    default_params = os.path.join(pkg_share, "config", "nav2_compass.yaml")

    robot_desc = xacro.process_file(xacro_file).toxml()

    headless = LaunchConfiguration("headless")
    use_nav2 = LaunchConfiguration("use_nav2")
    use_pedestrian = LaunchConfiguration("use_pedestrian")
    world = LaunchConfiguration("world")
    params_file = LaunchConfiguration("params_file")

    declare_headless = DeclareLaunchArgument(
        "headless", default_value="true",
        description="true 면 gz sim 서버 전용(-s, 렌더 없음). WSL2 GPU 스톨 회피용 기본값.")
    declare_use_nav2 = DeclareLaunchArgument(
        "use_nav2", default_value="true",
        description="Nav2 controller_server + lifecycle_manager 적재 여부.")
    declare_use_ped = DeclareLaunchArgument(
        "use_pedestrian", default_value="true",
        description="반응형 보행자 노드 실행 여부.")
    declare_world = DeclareLaunchArgument(
        "world", default_value=default_world,
        description="Gazebo 세계 SDF 경로.")
    declare_params = DeclareLaunchArgument(
        "params_file", default_value=default_params,
        description="Nav2 controller_server 파라미터 YAML 경로.")

    # headless 면 `-r -s <world>` (서버 전용), 아니면 `-r <world>` (GUI 포함).
    gz_args = PythonExpression([
        "'-r -s ' + '", world, "' if '", headless, "' == 'true' else '-r ' + '", world, "'"])

    gz_sim_share = get_package_share_directory("ros_gz_sim")
    gz_sim = IncludeLaunchDescription(
        PythonLaunchDescriptionSource(
            os.path.join(gz_sim_share, "launch", "gz_sim.launch.py")),
        launch_arguments={"gz_args": gz_args}.items(),
    )

    rsp = Node(
        package="robot_state_publisher",
        executable="robot_state_publisher",
        parameters=[{"robot_description": robot_desc, "use_sim_time": True}],
        output="screen",
    )

    # 로봇을 월드 원점에 스폰한다. 로봇 odom 은 스폰 상대 프레임이라 odom 원점이
    # 월드 원점과 일치해야 /people(보행자 ped/odom = 월드 좌표)이 컨트롤러의
    # global_frame=odom 과 같은 좌표계에 놓인다. (예전 -3.5 스폰은 보행자를
    # 컨트롤러 시점에서 +3.5 m 어긋나게 했다.)
    spawn = Node(
        package="ros_gz_sim",
        executable="create",
        arguments=["-name", "diff_bot", "-string", robot_desc,
                   "-x", "0.0", "-y", "0.0", "-z", "0.05"],
        output="screen",
    )

    bridge = Node(
        package="ros_gz_bridge",
        executable="parameter_bridge",
        parameters=[{"config_file": bridge_yaml, "use_sim_time": True}],
        output="screen",
    )

    pedestrian = Node(
        package="compass_sim",
        executable="reactive_pedestrian",
        name="reactive_pedestrian",
        parameters=[{"use_sim_time": True}],
        output="screen",
        condition=IfCondition(use_pedestrian),
    )

    # 보행자 지상 진실을 /people 로 발행 (컨트롤러의 사람 입력 경로 가동).
    people_gt = Node(
        package="compass_sim",
        executable="people_gt_publisher",
        name="people_gt_publisher",
        parameters=[{"use_sim_time": True, "frame_id": "odom"}],
        output="screen",
        condition=IfCondition(use_pedestrian),
    )

    controller_server = Node(
        package="nav2_controller",
        executable="controller_server",
        name="controller_server",
        output="screen",
        parameters=[params_file],
        condition=IfCondition(use_nav2),
    )

    lifecycle_manager = Node(
        package="nav2_lifecycle_manager",
        executable="lifecycle_manager",
        name="lifecycle_manager_control",
        output="screen",
        parameters=[{"use_sim_time": True,
                     "autostart": True,
                     "node_names": ["controller_server"],
                     "bond_timeout": 0.0}],
        condition=IfCondition(use_nav2),
    )

    return LaunchDescription([
        declare_headless, declare_use_nav2, declare_use_ped,
        declare_world, declare_params,
        gz_sim, rsp, spawn, bridge, pedestrian, people_gt,
        controller_server, lifecycle_manager,
    ])
