#!/usr/bin/env bash
# Build and load the vsensor kernel module (needs kernel headers + sudo).
set -e
cd "$(dirname "$0")/../driver"
make
sudo insmod vsensor.ko
sleep 0.3
ls -l /dev/vsensor
dmesg | tail -n 2
