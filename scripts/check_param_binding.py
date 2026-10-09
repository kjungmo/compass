#!/usr/bin/env python3
"""Check that Nav2 controller_server actually declared and applied every YAML key.

ROS 2 silently ignores parameter-file keys that a node never declares, so a typo,
a retired knob (for example `FollowPath.progress_length`) or a key spelled for a
different Nav2 release leaves the default in place without any warning. This
script optionally drives the node through the lifecycle `configure` transition
(which is when controller_server and the CompassController plugin declare their
parameters), then compares the YAML section of that node with the node's
declared parameters and current values. It exits non-zero if any YAML key is not
declared or holds a different value.

Only the section of the checked node is compared (by default `controller_server`,
plugin keys included as dotted names such as `FollowPath.d_safe`); other node
sections in the same file (local_costmap, lifecycle managers) are not checked.

    ros2 run nav2_controller controller_server --ros-args --params-file cfg.yaml &
    python3 scripts/check_param_binding.py cfg.yaml --configure
"""

import argparse
import math
import os
import sys

import rclpy
import yaml
from lifecycle_msgs.msg import Transition
from lifecycle_msgs.srv import ChangeState, GetState
from rcl_interfaces.msg import ParameterType
from rcl_interfaces.srv import GetParameters, ListParameters

# Keys that a given Nav2 release legitimately does not declare, with the reason.
# Keep this minimal: an entry hides a key from the check on that distribution.
DISTRO_ALLOWLIST = {
    # Humble reads the singular `progress_checker_plugin`; Jazzy replaced it with the
    # list `progress_checker_plugins`, whose default ["progress_checker"] names the
    # same plugin section, so sim/config/nav2_compass.yaml (written for both) still
    # configures the progress checker it lists on Jazzy.
    "jazzy": {"progress_checker_plugin"},
}


def flatten(prefix, value, out):
    if isinstance(value, dict):
        for key, sub in value.items():
            flatten(f"{prefix}.{key}" if prefix else str(key), sub, out)
    else:
        out[prefix] = value


def yaml_params(path, node):
    with open(path, encoding="utf-8") as f:
        doc = yaml.safe_load(f) or {}
    params = {}
    for section in ("/**", node, "/" + node):
        flatten("", (doc.get(section) or {}).get("ros__parameters", {}), params)
    if not params:
        sys.exit(f"FAIL: {path} has no ros__parameters section for node {node!r}")
    return params


def value_of(pv):
    return {
        ParameterType.PARAMETER_BOOL: lambda: pv.bool_value,
        ParameterType.PARAMETER_INTEGER: lambda: pv.integer_value,
        ParameterType.PARAMETER_DOUBLE: lambda: pv.double_value,
        ParameterType.PARAMETER_STRING: lambda: pv.string_value,
        ParameterType.PARAMETER_BOOL_ARRAY: lambda: list(pv.bool_array_value),
        ParameterType.PARAMETER_INTEGER_ARRAY: lambda: list(pv.integer_array_value),
        ParameterType.PARAMETER_DOUBLE_ARRAY: lambda: list(pv.double_array_value),
        ParameterType.PARAMETER_STRING_ARRAY: lambda: list(pv.string_array_value),
    }.get(pv.type, lambda: None)()


def same(expected, actual):
    if isinstance(expected, list) and isinstance(actual, list):
        return len(expected) == len(actual) and all(map(same, expected, actual))
    if isinstance(expected, bool) or isinstance(actual, bool):
        return expected is actual
    if isinstance(expected, (int, float)) and isinstance(actual, (int, float)):
        return math.isclose(float(expected), float(actual), rel_tol=0.0, abs_tol=1e-12)
    return expected == actual


def call(node, client, request, timeout):
    if not client.wait_for_service(timeout_sec=timeout):
        sys.exit(f"FAIL: service {client.srv_name} not available after {timeout:.0f} s")
    future = client.call_async(request)
    rclpy.spin_until_future_complete(node, future, timeout_sec=timeout)
    if future.result() is None:
        sys.exit(f"FAIL: no response from {client.srv_name}")
    return future.result()


def configure(node, target, timeout):
    state = call(node, node.create_client(GetState, f"{target}/get_state"),
                 GetState.Request(), timeout).current_state.label
    if state != "unconfigured":
        print(f"{target} is already {state}; not changing its lifecycle state")
        return
    req = ChangeState.Request()
    req.transition.id = Transition.TRANSITION_CONFIGURE
    res = call(node, node.create_client(ChangeState, f"{target}/change_state"), req, timeout)
    if not res.success:
        sys.exit(f"FAIL: {target} rejected the configure transition (see the node's log)")
    print(f"{target} configured")


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n\n")[0])
    ap.add_argument("yaml")
    ap.add_argument("--node", default="controller_server")
    ap.add_argument("--configure", action="store_true",
                    help="trigger the lifecycle configure transition first")
    ap.add_argument("--distro", default=os.environ.get("ROS_DISTRO", ""),
                    help="selects the per-distribution allowlist (default: $ROS_DISTRO)")
    ap.add_argument("--timeout", type=float, default=30.0)
    args = ap.parse_args()

    expected = yaml_params(args.yaml, args.node)
    allowed = DISTRO_ALLOWLIST.get(args.distro, set())
    rclpy.init()
    node = rclpy.create_node("compass_param_binding_check")
    target = "/" + args.node.lstrip("/")
    if args.configure:
        configure(node, target, args.timeout)
    listed = call(node, node.create_client(ListParameters, f"{target}/list_parameters"),
                  ListParameters.Request(), args.timeout)
    declared = set(listed.result.names)

    undeclared = sorted(k for k in expected if k not in declared and k not in allowed)
    skipped = sorted(k for k in expected if k not in declared and k in allowed)
    keys = sorted(k for k in expected if k in declared)
    req = GetParameters.Request()
    req.names = keys
    got = call(node, node.create_client(GetParameters, f"{target}/get_parameters"), req,
               args.timeout)
    actual = {k: value_of(v) for k, v in zip(keys, got.values)}
    mismatched = [(k, expected[k], actual[k]) for k in keys if not same(expected[k], actual[k])]

    print(f"{args.yaml}: {len(expected)} YAML keys for {target}, {len(keys)} applied as written")
    for k in skipped:
        print(f"  allowed on {args.distro}: {k} is not declared by this Nav2 release")
    for k in undeclared:
        print(f"  NOT DECLARED by {target}: {k} = {expected[k]!r} (the node ignores it)")
    for k, e, a in mismatched:
        print(f"  MISMATCH {k}: YAML {e!r}, node {a!r}")
    node.destroy_node()
    rclpy.shutdown()
    if undeclared or mismatched:
        sys.exit("FAIL: parameter binding")
    print("PASS: every checked YAML key is declared by the node and holds the YAML value")


if __name__ == "__main__":
    main()
