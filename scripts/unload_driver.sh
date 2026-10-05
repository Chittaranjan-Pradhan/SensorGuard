#!/usr/bin/env bash
set -e
sudo rmmod vsensor
dmesg | tail -n 1
