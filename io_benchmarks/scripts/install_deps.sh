#!/bin/bash
# this file contains dependencies needed to run the benchmarks for Ubuntu

echo "Installing required dependencies..."

sudo apt update

# io uring helper library
sudo apt install -y build-essential cmake liburing-dev

echo "Dependencies installed successfully."