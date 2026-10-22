#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/sloop-tests
python3 tests/test_sloop_scope_refresh.py
for name in sloop_smk_storage smk37_midi_tx smk37_midi_output smk37_pad_feedback smk37_lights sloop_smk_pad_colors; do
    cc -std=gnu99 -O1 -w -Ifirmware/hal -fsanitize=address,undefined "tests/${name}_test.c" -o "build/sloop-tests/$name"
    "build/sloop-tests/$name"
done
cc -std=gnu99 -O1 -w -fno-builtin -fsanitize=address -Ibuild/sloop-gen -Ifirmware/hal tests/sloop_smk_samples_test.c -o build/sloop-tests/samples
build/sloop-tests/samples
cc -std=gnu99 -O1 -w -fno-builtin -fsanitize=address -Ibuild/sloop-gen -Ifirmware/hal tests/sloop_smk_routing_test.c -o build/sloop-tests/routing
build/sloop-tests/routing

cc -std=gnu99 -O1 -w -fno-builtin -fsanitize=address -Ibuild/sloop-gen -Ifirmware/hal tests/sloop_smk_persistence_test.c -o build/sloop-tests/persistence
build/sloop-tests/persistence

cc -std=gnu99 -O1 -w -fno-builtin -fsanitize=address -Ibuild/sloop-gen -Ifirmware/hal tests/sloop_smk_storage_reset_test.c -o build/sloop-tests/reset
build/sloop-tests/reset

cc -std=gnu99 -O1 -w -fno-builtin -fsanitize=address -Ibuild/sloop-gen -Ifirmware/hal tests/sloop_smk_28_test.c -o build/sloop-tests/upgrade28
build/sloop-tests/upgrade28
