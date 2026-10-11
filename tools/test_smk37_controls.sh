#!/bin/sh
# SPDX-License-Identifier: GPL-3.0-only
# Offline host tests and optional freestanding processor compilation.
set -eu
cd "$(dirname "$0")/.."
mkdir -p build/smk37-tests
for component in contacts controls button_watch rawscan analog_scan analog_watch analog_controls pad_feedback adc response velocity pedal store trim keyscan events audio lcd panel lights scanner boot bootguard time memory; do
    cc -std=c99 -Wall -Wextra -Werror -fsanitize=undefined,address \
        "tests/smk37_${component}_test.c" -o "build/smk37-tests/$component"
    "build/smk37-tests/$component"
done
if [ -n "${JIELI_TOOLCHAIN:-}" ]; then
    "$JIELI_TOOLCHAIN/pi32v2/bin/clang" -target pi32v2 -std=c99 -Os \
        -ffreestanding -Wall -Wextra -Werror -c tests/smk37_target_compile.c \
        -o build/smk37-tests/controls-pi32v2.o
    "$JIELI_TOOLCHAIN/common/bin/objdump" -h -t build/smk37-tests/controls-pi32v2.o
fi
