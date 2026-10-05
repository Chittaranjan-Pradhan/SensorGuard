#!/usr/bin/env bash
# 20-second live demo (simulator): healthy -> spike burst -> recovery. Great for the final presentation.
set -e
B="${1:-build}"
"$B/sensorguardd" -s -p 100 -n 200 -i spike:60:110 -l demo.log -c demo.csv
echo "Log written to demo.log, per-sample data to demo.csv"
