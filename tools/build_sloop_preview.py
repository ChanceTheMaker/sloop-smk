#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Build upstream mock-mode preview using the same Zig and generated assets as SMK."""
import argparse, subprocess
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('--zig', required=True)
a=p.parse_args()
exports='init buffer frames rate track param global select step locks fm6 sample kit hit note play position panic render'.split()
subprocess.run([a.zig,'cc','-target','wasm32-freestanding','-O2','-fno-builtin','-fwrapv','-fno-strict-aliasing','-nostdlib','-w',
 '-I'+str(ROOT/'sloop/tools/wasm_shim'),'-I'+str(ROOT/'build/sloop-gen'),'-I'+str(ROOT/'sloop/firmware/src'),
 str(ROOT/'sloop/firmware/host/preview.c'),str(ROOT/'sloop/tools/wasm_shim/mem.c'),
 '-Wl,--no-entry','-Wl,--export-memory','-Wl,--initial-memory=0x1000000','-Wl,-z,stack-size=0x20000',
 *['-Wl,--export=preview_'+e for e in exports],'-o',str(ROOT/'web/sloop-preview.wasm')],check=True)
