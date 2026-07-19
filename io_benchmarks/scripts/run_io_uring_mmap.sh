#!/bin/bash

# below is the script used to test io_uring VS mmap on processing a huge file sequentially
# this script should be run from the io_benchmarks directory
# you may wish to direct it to a .txt file under the results directory.
# For example, ./scripts/run_io_uring_mmap.sh > io_uring_mmap_results.txt

DATA_FILE="./data/benchmark_data.bin"
GB_SIZE=5
BUILD_DIR="./build"

echo ">>> Building benchmarks..."
mkdir -p "$BUILD_DIR"
cd "$BUILD_DIR" || exit 1
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j$(nproc)
cd .. || exit 1
echo ""

if [ ! -f "$DATA_FILE" ]; then
    echo ">>> Data file not found. Generating ${GB_SIZE}GB test file..."
    mkdir -p ./data

    ./scripts/generate_file.sh "$GB_SIZE" "$DATA_FILE"
    echo ""
fi

echo "================================================================"
echo "                 STARTING I/O BENCHMARK SUITE                   "
echo "================================================================"

# pass specific hardware telemetry counters to perf stat:
PERF_FLAGS="-e task-clock,context-switches,cpu-migrations,page-faults,cycles,instructions"

echo ">>> [1/2] Clearing OS page cache..."
echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null

echo ">>> Running bench_mmap..."
sudo perf stat $PERF_FLAGS "$BUILD_DIR/bench_mmap" "$DATA_FILE"
echo ""

echo ">>> [2/2] Clearing OS page cache..."
echo 3 | sudo tee /proc/sys/vm/drop_caches > /dev/null

echo ">>> Running bench_io_uring..."
sudo perf stat $PERF_FLAGS "$BUILD_DIR/bench_io_uring" "$DATA_FILE"

echo ">>> Cleaning up test data..."
rm -f "$DATA_FILE"

echo "================================================================"
echo "                        BENCHMARK COMPLETE                      "
echo "================================================================"