#!/bin/bash
# Builds and runs the relevant test files. If it's not picking
# something up, ensure you've listed it in the TESTS var in the
# Makefile.
make test
find . -name "*_test" -exec ./{} \;
