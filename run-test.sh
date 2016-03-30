#!/bin/bash
make test
find . -name "*_test" -exec ./{} \;
