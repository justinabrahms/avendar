#!/bin/bash -xe
cd code
make main
cp avendar ../content/area
cd ../content/area
./avendar
