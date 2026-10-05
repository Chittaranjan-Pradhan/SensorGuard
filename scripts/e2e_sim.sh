#!/usr/bin/env bash
# End-to-end test of the daemon using the built-in simulator (no root / kernel module needed).
# Usage: scripts/e2e_sim.sh path/to/sensorguardd
set -u
BIN="${1:-build/sensorguardd}"
TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT
fail=0
check() { if eval "$2"; then echo "  ok   - $1"; else echo "  FAIL - $1"; fail=1; fi; }

echo "[E2E-1] healthy run exits 0 and never reports a fault"
"$BIN" -s -q -p 1 -n 200 -c "$TMP/a.csv" 2>/dev/null; rc=$?
check "exit code 0"                 "[ $rc -eq 0 ]"
check "CSV has 200 data rows"       "[ \$(tail -n +2 $TMP/a.csv | wc -l) -eq 200 ]"
check "no fault rows"               "! tail -n +2 $TMP/a.csv | cut -d, -f5 | grep -qv '^none$'"

echo "[E2E-2] drift injected and never cleared => exit code 2"
"$BIN" -s -q -p 1 -n 300 -i drift:60 -c "$TMP/b.csv" 2>/dev/null; rc=$?
check "exit code 2 (FAULT)"         "[ $rc -eq 2 ]"
check "CSV contains drift rows"     "grep -q ',drift,' $TMP/b.csv"
check "CSV reaches FAULT state"     "grep -q ',FAULT\$' $TMP/b.csv"

echo "[E2E-3] spike injected then cleared => recovers, exit 0"
"$BIN" -s -q -p 1 -n 300 -i spike:60:100 -c "$TMP/c.csv" 2>/dev/null; rc=$?
check "exit code 0"                 "[ $rc -eq 0 ]"
check "saw FAULT during the burst"  "grep -q ',FAULT\$' $TMP/c.csv"
check "saw RECOVERING"              "grep -q ',RECOVERING\$' $TMP/c.csv"
check "ended NORMAL"                "[ \"\$(tail -n 1 $TMP/c.csv | cut -d, -f6)\" = NORMAL ]"

echo "[E2E-4] SIGTERM stops the daemon gracefully"
"$BIN" -s -q -p 10 -c "$TMP/d.csv" 2>/dev/null &
pid=$!
sleep 0.5; kill -TERM $pid; wait $pid; rc=$?
check "exit code 0 after SIGTERM"   "[ $rc -eq 0 ]"
check "CSV flushed with data"       "[ \$(wc -l < $TMP/d.csv) -gt 5 ]"

echo "[E2E-5] bad arguments are rejected"
"$BIN" --inject bogus:1 >/dev/null 2>&1; rc=$?
check "exit code 1"                 "[ $rc -eq 1 ]"

[ $fail -eq 0 ] && echo "E2E: ALL PASSED" || echo "E2E: FAILURES"
exit $fail
