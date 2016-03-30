#!/bin/bash -e
# Builds and runs the relevant test files. If it's not picking
# something up, ensure you've listed it in the TESTS var in the
# Makefile.

# setup output directory for circle-ci reporting
TEST_OUTPUT_DIR="${CIRCLE_TEST_REPORTS:-.}"
mkdir -p $TEST_OUTPUT_DIR

# Build relevant tests
make clean test

# exec all tests with coverage output
find . -name "*_test" -exec ./{} --gtest_output=xml:$TEST_OUTPUT_DIR/{}.xml \;

# Mark all non _test files for coverage checking.
find . -name "*.c" -o -name "*.cpp" | grep -v "_test" | xargs gcov
