#!/bin/bash
# Performance profiling script for SO loading: memfd vs file
# Usage: ./profile_so_loading.sh [runs] [mode_name]
#   runs: number of test runs (default: 5)
#   mode_name: name for this test run, e.g., "memfd" or "file" (default: "test")
#
# NOTE: You must manually edit aicpu_executor.cpp to set force_file_only or force_memfd_only
# before running this script. The mode_name parameter is only for logging purposes.

set -e

# Configuration
RUNS=${1:-5}  # Default 5 runs
MODE_NAME=${2:-"test"}  # Default mode name
TIMESTAMP=$(date +%Y%m%d_%H%M%S)
LOG_DIR="profiling_logs_${MODE_NAME}_${TIMESTAMP}"
SUMMARY_FILE="${LOG_DIR}/summary.txt"

# Colors for output
RED='\033[0;31m'
GREEN='\033[0;32m'
YELLOW='\033[1;33m'
NC='\033[0m' # No Color

# Create log directory
echo -e "${GREEN}=== SO Loading Performance Profiling: ${MODE_NAME} mode ===${NC}"
echo "Creating log directory: ${LOG_DIR}"
mkdir -p "${LOG_DIR}"

# Function to run test and extract metrics
run_test() {
  local run_num=$1
  local log_file="${LOG_DIR}/${MODE_NAME}_run_${run_num}.log"

  echo ""
  echo -e "${YELLOW}--- Run ${run_num}/${RUNS} ---${NC}"
  echo "Log file: ${log_file}"

  # Run the test and save full output
  export ASCEND_HOME_PATH=/usr/local/Ascend/cann-8.5.0
  python3 examples/scripts/run_example.py \
    -k examples/a2a3/tensormap_and_ringbuffer/vector_example/kernels \
    -g examples/a2a3/tensormap_and_ringbuffer/vector_example/golden.py \
    -p a2a3 -d 1 --verbose > "${log_file}" 2>&1

  # Extract timing metrics
  echo -e "${GREEN}Extracting timing metrics...${NC}"

  # Get the latest device log from any device (device-0, device-1, etc.)
  local device_log=$(ls -t ~/ascend/log/debug/device-*/device-*.log 2>/dev/null | head -1)

  if [ -n "$device_log" ]; then
    # Copy device log
    cp "$device_log" "${LOG_DIR}/device_${MODE_NAME}_run_${run_num}.log"

    # Extract metrics
    echo "=== Device Log Timing ===" >> "${log_file}.metrics"

    # Extract timing based on mode
    if [ "$MODE_NAME" = "memfd" ]; then
      # memfd timing
      grep -E "\[MEMFD_TIMING\]|\[SO_LOADING\].*memfd" "$device_log" >> "${log_file}.metrics" 2>/dev/null || true
    elif [ "$MODE_NAME" = "file" ]; then
      # file timing
      grep -E "\[FILE_TIMING\]|\[SO_LOADING\].*file" "$device_log" >> "${log_file}.metrics" 2>/dev/null || true
    fi
  fi

  # Extract host timing
  echo "=== Host Timing ===" >> "${log_file}.metrics"
  grep -E "TIMING:" "$log_file" >> "${log_file}.metrics" 2>/dev/null || true

  # Display summary
  echo ""
  echo -e "${GREEN}Key metrics from this run:${NC}"
  grep -E "\[MEMFD_TIMING\]|\[FILE_TIMING\]|\[SO_LOADING\]" "${log_file}.metrics" 2>/dev/null || echo "No metrics found"

  # Check for errors
  if grep -q -E "ERROR|FAILED|error:" "$log_file"; then
    echo -e "${RED}Warning: Errors detected in this run${NC}"
    grep -E "ERROR|FAILED|error:" "$log_file" | tail -5
  fi
}

# Function to parse metrics from log file
parse_metric() {
  local file=$1
  local pattern=$2
  local field=$3

  grep -E "$pattern" "$file" | head -1 | sed -n "s/.*$field=\([0-9]*\).*/\1/p"
}

# Function to parse MEMFD_TIMING format
parse_memfd_metric() {
  local file=$1
  local field=$2

  grep "\[MEMFD_TIMING\]" "$file" | head -1 | sed -n "s/.*$field=\([0-9]*\).*/\1/p"
}

# Function to parse FILE_TIMING format
parse_file_metric() {
  local file=$1
  local field=$2

  grep "\[FILE_TIMING\]" "$file" | head -1 | sed -n "s/.*$field=\([0-9]*\).*/\1/p"
}

# Main profiling loop
echo ""
echo "Starting profiling with ${RUNS} runs for ${MODE_NAME} mode..."
echo ""

for ((run=1; run<=RUNS; run++)); do
  run_test "$run"
  sleep 1  # Small delay between runs
done

# Generate summary report
echo ""
echo -e "${GREEN}=== Generating Summary Report ===${NC}"

{
  echo "=========================================="
  echo "SO Loading Performance Profiling Summary"
  echo "=========================================="
  echo "Date: $(date)"
  echo "Mode: ${MODE_NAME}"
  echo "Runs: ${RUNS}"
  echo ""

  # Parse all runs
  total_sum=0
  count=0

  for ((run=1; run<=RUNS; run++)); do
    metrics_file="${LOG_DIR}/${MODE_NAME}_run_${run}.log.metrics"

    if [ -f "$metrics_file" ]; then
      echo "--- Run ${run} ---"

      # Extract metrics based on mode
      if [ "$MODE_NAME" = "memfd" ]; then
        # memfd: extract from MEMFD_TIMING
        total=$(parse_memfd_metric "$metrics_file" "total")
        write=$(parse_memfd_metric "$metrics_file" "write")
        dlopen=$(parse_memfd_metric "$metrics_file" "dlopen")
      elif [ "$MODE_NAME" = "file" ]; then
        # file: extract from FILE_TIMING
        total=$(parse_file_metric "$metrics_file" "total")
        write=$(parse_file_metric "$metrics_file" "write")
        dlopen=$(parse_file_metric "$metrics_file" "dlopen")
      fi

      if [ -n "$total" ] && [ "$total" != "0" ]; then
        echo "  Total time: ${total} ticks (~$((${total} / 50000)) us)"
        [ -n "$write" ] && [ "$write" != "0" ] && echo "  Write time: ${write} ticks"
        [ -n "$dlopen" ] && [ "$dlopen" != "0" ] && echo "  dlopen time: ${dlopen} ticks"

        total_sum=$((total_sum + total))
        count=$((count + 1))
      else
        echo "  No valid metrics found"
      fi

      # Check for errors
      error_count=$(grep -c "ERROR" "${LOG_DIR}/${MODE_NAME}_run_${run}.log" || true)
      [ "$error_count" -gt 0 ] && echo "  Errors: ${error_count}"
    fi

    echo ""
  done

  # Calculate average
  if [ $count -gt 0 ]; then
    avg=$((total_sum / count))
    echo "--- Average over ${count} runs ---"
    echo "  Average total time: ${avg} ticks (~$((${avg} / 50000)) us)"
    echo ""
  fi

  echo "=========================================="
  echo "Full logs available at: ${LOG_DIR}"
  echo "=========================================="

} > "${SUMMARY_FILE}"

# Display summary
cat "${SUMMARY_FILE}"

echo ""
echo -e "${GREEN}=== Profiling Complete ===${NC}"
echo "Summary saved to: ${SUMMARY_FILE}"
echo "Full logs saved to: ${LOG_DIR}"
echo ""
echo "To view device logs:"
echo "  ls -lh ${LOG_DIR}/device_*.log"
echo ""
echo "To view individual run metrics:"
echo "  cat ${LOG_DIR}/*_run_*.log.metrics"
echo ""
echo "To compare modes:"
echo "  cat profiling_logs_*/summary.txt"
