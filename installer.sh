#!/usr/bin/env bash

set -e

echo "installing"
cmake -B build
cmake --build build -j"$(nproc)"

sudo cmake --install build

echo "Done."
