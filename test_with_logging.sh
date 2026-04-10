#!/bin/bash
# Test script with CANN logging enabled

# Enable CANN logging
export ASCEND_GLOBAL_LOG_LEVEL=0  # 0=debug (most verbose)
export ASCEND_SLOG_PRINT_TO_STDOUT=1  # Print to stdout

echo "=== CANN Logging Enabled ==="
echo "ASCEND_GLOBAL_LOG_LEVEL=$ASCEND_GLOBAL_LOG_LEVEL"
echo "ASCEND_SLOG_PRINT_TO_STDOUT=$ASCEND_SLOG_PRINT_TO_STDOUT"
echo ""

# Run the test
python examples/scripts/run_example.py \
    -k examples/a2a3/tensormap_and_ringbuffer/paged_attention/kernels \
    -g examples/a2a3/tensormap_and_ringbuffer/paged_attention/golden.py \
    -p a2a3 \
    -d 0 \
    2>&1 | tee test_with_logging.log
