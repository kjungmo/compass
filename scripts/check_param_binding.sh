#!/usr/bin/env bash
# Start a headless Nav2 controller_server for each parameter file, drive it through
# lifecycle configure, and check that every controller_server key in the file is
# declared and applied (scripts/check_param_binding.py). Needs a sourced ROS 2
# environment with nav2_controller and this workspace's install/ (compass_nav2).
#
#   bash scripts/check_param_binding.sh src/compass_nav2/config/compass_params.yaml sim/config/nav2_compass.yaml
set -u
here="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
logdir="${PARAM_BINDING_LOGDIR:-$(mktemp -d)}"
stop() {  # bash ignores SIGINT in background jobs, so stop the process group with SIGTERM
  kill -TERM -- "-$1" 2>/dev/null || true
  for _ in $(seq 15); do kill -0 "$1" 2>/dev/null || return 0; sleep 1; done
  kill -KILL -- "-$1" 2>/dev/null || true
}
failed=0
for params in "$@"; do
  log="$logdir/$(basename "$params" .yaml).controller_server.log"
  setsid ros2 run nav2_controller controller_server --ros-args --params-file "$params" > "$log" 2>&1 &
  pid=$!
  status=0
  timeout 90 python3 "$here/check_param_binding.py" "$params" --configure || status=$?
  grep -a "CompassController" "$log" | head -n 3
  stop "$pid"
  if [ "$status" -ne 0 ]; then
    echo "---- controller_server log for $params"; cat "$log"; failed=1
  fi
done
exit "$failed"
