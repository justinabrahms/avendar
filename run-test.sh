#!/bin/bash -e
# Builds and runs the relevant test files. If it's not picking
# something up, ensure you've listed it in the TESTS var in the
# Makefile.
make clean test
find . -name "*_test" -exec ./{} \;
