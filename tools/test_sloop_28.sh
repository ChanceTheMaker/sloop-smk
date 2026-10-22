#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Requires generated headers from a candidate build (including sloop_logo.h).
cd "$(dirname "$0")/.."
set -eu
mkdir -p build/sloop-tests/upstream-ui
for name in seq2 project ui_pages; do
 cc -std=gnu11 -O2 -w -pthread -Ibuild/sloop-gen -Isloop/firmware/src -Isloop/firmware/hal sloop/tests/${name}_test.c -lm -o build/sloop-tests/${name}
 build/sloop-tests/${name} build/sloop-tests/upstream-ui > build/sloop-tests/${name}.log 2>&1 || { tail -25 build/sloop-tests/${name}.log; exit 1; }
 tail -3 build/sloop-tests/${name}.log
done
