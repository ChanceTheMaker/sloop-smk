#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/sloop-tests build/sloop-gen
# Keep CI independent of private target packages or an installed toolchain.
if [ ! -f build/sloop-gen/felucca_icons.h ]; then
  for pair in tables:tables samples:samples fm6_patches:fm6 drumkits:drumkits font:font icons:icons; do
    script=${pair%:*}; header=${pair#*:}
    python3 "sloop/tools/gen_${script}.py" "build/sloop-gen/felucca_${header}.h"
  done
fi
cc -std=gnu11 -O1 -w tests/audio_worker_test.c -o build/sloop-tests/worker
build/sloop-tests/worker
for mode in 0 1; do
  cc -std=gnu11 -DSLOOP_DUAL_CORE=$mode tests/sloop_smk_multicore_test.c -o build/sloop-tests/lifecycle
  build/sloop-tests/lifecycle
done
cc -std=gnu11 -O1 -w -fno-builtin -pthread -Ibuild/sloop-gen -Ifirmware/hal tests/sloop_smk_render_worker_test.c -o build/sloop-tests/render-worker
build/sloop-tests/render-worker 0 build/sloop-tests/serial.pcm
build/sloop-tests/render-worker 1 build/sloop-tests/parallel.pcm
cmp build/sloop-tests/serial.pcm build/sloop-tests/parallel.pcm
printf 'PASS: serial and concurrent Sloop PCM are identical\n'
