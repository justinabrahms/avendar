#!/bin/bash -e
# Builds and runs the relevant test files. If it's not picking
# something up, ensure you've listed it in the TESTS var in the
# Makefile.
TEST_OUTPUT_DIR="${CIRCLE_TEST_REPORTS:-.}"
mkdir -p $TEST_OUTPUT_DIR
make test
find . -name "*_test" -exec ./{} --gtest_output=xml:$TEST_OUTPUT_DIR/{}.xml \;
