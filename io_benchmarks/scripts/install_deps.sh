#!/bin/bash
# this file contains dependencies needed to run the benchmarks for Ubuntu

echo "Installing required dependencies..."

sudo apt update

# perf
sudo apt install -y linux-tools-common linux-tools-generic linux-tools-$(uname -r)

# io uring helper library
sudo apt install -y build-essential cmake liburing-dev

echo "Dependencies installed successfully."