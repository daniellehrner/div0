#!/bin/bash
# Wrapper script for lcov to use llvm-cov gcov instead of gcov
# Find versioned llvm-cov if unversioned not available
LLVM_COV=$(command -v llvm-cov 2>/dev/null || ls /usr/bin/llvm-cov-* 2>/dev/null | sort -V | tail -1)
exec "$LLVM_COV" gcov "$@"
