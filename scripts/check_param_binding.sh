#!/usr/bin/env bash
# Start a headless Nav2 controller_server for each parameter file, drive it through
# lifecycle configure, and check that every controller_server key in the file is
# declared and applied (scripts/check_param_binding.py). Needs a sourced ROS 2
# environment with nav2_controller and this workspace's install/ (compass_nav2).
#
#   bash scripts/check_param_binding.sh src/compass_nav2/config/compass_params.yaml sim/config/nav2_compass.yaml
#
# ROS traffic is kept on this host and on a random domain id, so parallel runs and
# other robots on the network cannot answer for (or disturb) the node under test.
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
logdir="${PARAM_BINDING_LOGDIR:-$(mktemp -d)}"
export ROS_DOMAIN_ID="${PARAM_BINDING_DOMAIN_ID:-$((RANDOM % 100 + 1))}"  # 1..100
if [ "${ROS_DISTRO:-}" = "humble" ]; then
  export ROS_LOCALHOST_ONLY=1
else
  export ROS_AUTOMATIC_DISCOVERY_RANGE=LOCALHOST
fi
pid=""
stop() {  # bash ignores SIGINT in background jobs, so stop the process group with SIGTERM
  [ -n "$pid" ] || return 0
  kill -TERM -- "-$pid" 2>/dev/null || true
  for _ in $(seq 15); do kill -0 "$pid" 2>/dev/null || { pid=""; return 0; }; sleep 1; done
  kill -KILL -- "-$pid" 2>/dev/null || true
  pid=""
}
trap 'stop' EXIT
trap 'stop; exit 130' INT TERM
failed=0
for params in "$@"; do
  log="$logdir/$(basename "$params" .yaml).controller_server.log"
  setsid ros2 run nav2_controller controller_server --ros-args --params-file "$params" > "$log" 2>&1 &
  pid=$!
  status=0
  timeout 90 python3 "$here/check_param_binding.py" "$params" --configure || status=$?
  grep -a "CompassController" "$log" | head -n 3
  stop
  if [ "$status" -ne 0 ]; then
    echo "---- controller_server log for $params"; cat "$log"; failed=1
  fi
done
exit "$failed"
