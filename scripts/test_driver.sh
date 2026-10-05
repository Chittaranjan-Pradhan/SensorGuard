#!/usr/bin/env bash
# Driver + daemon system test against the REAL kernel module (driver must be loaded).
# Usage: scripts/test_driver.sh [build_dir]
set -u
B="${1:-build}"
DEV=/dev/vsensor
fail=0
check() { if eval "$2"; then echo "  ok   - $1"; else echo "  FAIL - $1"; fail=1; fi; }

[ -c "$DEV" ] || { echo "$DEV not found - run scripts/load_driver.sh first"; exit 1; }

echo "[DRV-1] basic character-device behaviour"
"$B/sgctl" $DEV reset >/dev/null
check "read returns a sample"            "$B/sgctl $DEV read 1 | grep -q 'valid=1'"
check "initial fault mode is none"       "[ \"\$($B/sgctl $DEV get)\" = none ]"
"$B/sgctl" $DEV set drift >/dev/null
check "ioctl SET_FAULT round-trips"      "[ \"\$($B/sgctl $DEV get)\" = drift ]"
check "RESET clears the fault"           "$B/sgctl $DEV reset >/dev/null && [ \"\$($B/sgctl $DEV get)\" = none ]"
check "read with tiny buffer is EINVAL"  "! dd if=$DEV bs=4 count=1 of=/dev/null 2>/dev/null"

echo "[DRV-2] injected faults are visible in the data stream"
"$B/sgctl" $DEV reset >/dev/null; "$B/sgctl" $DEV set dropout >/dev/null
check "dropout => valid=0"               "$B/sgctl $DEV read 2 | grep -q 'valid=0'"

echo "[DRV-3] full daemon on real device: drift => FAULT (exit 2)"
"$B/sgctl" $DEV reset >/dev/null
"$B/sensorguardd" -q -d $DEV -p 5 -n 300 -i drift:60; rc=$?
check "daemon exit code 2"               "[ $rc -eq 2 ]"
"$B/sgctl" $DEV reset >/dev/null

[ $fail -eq 0 ] && echo "DRIVER TESTS: ALL PASSED" || echo "DRIVER TESTS: FAILURES"
exit $fail
