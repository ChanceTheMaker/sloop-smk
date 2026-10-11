#!/usr/bin/env python3
# SPDX-License-Identifier: GPL-3.0-only
"""Link an offline SMK integration image, never a flash/update package.

JIELI_TOOLCHAIN=... python tools/build_sloop_smk.py
The raw .bin is not directly installable. Private packaging is a separate step.
Uses the conservative existing AC79 linker allocation, not the FM-1 package
builder/storage geometry. Outputs are isolated under build/smk37-integration.
"""
import argparse
import hashlib
from pathlib import Path
from smk37_backup import inspect_backup
import json
import re
import subprocess
import sys
import zlib
import build


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--usb-test-number',type=int,default=30,choices=range(30,100))
    args = parser.parse_args()
    args.public_release=True
    args.enable_saves=args.enable_presets=args.enable_samples=args.enable_trs=False
    args.enable_usb_audio=args.enable_usb_update=True
    evidence=None
    out=build.SRC/'build/sloop-smk-2.6.0-b1'
    out.mkdir(parents=True,exist_ok=True)
    build.GEN=out/'gen';build.GEN.mkdir(parents=True,exist_ok=True)
    (build.GEN/'smk37_unit.h').write_text(
        '#define SMK37_PUBLIC_BUILD 1\n'
        '#define SMK37_UNIT_TRIM_CRC 0u\n'
        '#define SMK37_UNIT_HEAD_CRC 0x3a04ad23u\n'
        '#define SMK37_UNIT_SAMPLE_CRCS {0,0,0}\n'
        f'#define SMK37_UPDATE_PRODUCT "SMK-37_9{args.usb_test_number:02d}"\n'
        '#define SMK37_UPDATE_VERSION "SLOOP SMK 2.6.0-b1"\n')
    from build_smk37_loader import build_loader
    build_loader(out,build.GEN,f"SMK-37_9{args.usb_test_number:02d}")
    for script,header in [('gen_tables.py','felucca_tables.h'),('gen_samples.py','felucca_samples.h'),('gen_fm6_patches.py','felucca_fm6.h'),('gen_drumkits.py','felucca_drumkits.h'),('gen_font.py','felucca_font.h'),('gen_icons.py','felucca_icons.h')]:
        subprocess.run([sys.executable,str(build.SRC/'sloop/tools'/script),str(build.GEN/header)],check=True,cwd=build.SRC/'sloop')
    flags = [*build.CFLAGS, f"-DSMK37_ENABLE_SAMPLES={int(args.enable_samples)}", f"-DSMK37_ENABLE_USB_AUDIO={int(args.enable_usb_audio)}", "-Ifirmware/hal", "-Ifirmware/src", f"-I{build.GEN.relative_to(build.SRC)}", f"-DSMK37_ENABLE_PRESETS={int(args.enable_presets)}", f"-DSMK37_ENABLE_SAVES={int(args.enable_saves)}", f"-DSMK37_ENABLE_USB_UPDATE={int(args.enable_usb_update)}"]
    flags += [f'-DSMK37_ENABLE_TRS={int(args.enable_trs)}']
    build.tc_all(
        ("cc", *flags, "-c", build.FW / "src" / "sloop_smk_app.c", "-o", out / "app.o"),
        ("cc", f"-DSMK37_ENABLE_USB_AUDIO={int(args.enable_usb_audio)}", "-c", build.FW / "smk37_crt0.S", "-o", out / "crt0.o"),
        ("cc", "-c", build.FW / "hal" / "fm1_vec.S", "-o", out / "vec.o"),
    )
    elf = out / "smk37-integration.elf"
    # This allocation is a hypothesis based on shared CPU family + reserved
    # boot/vector addresses. The manifest explicitly does not certify RAM.
    build.tc("pi32v2/bin/ld", "-T", build.FW / "app.ld",
             out / "crt0.o", out / "vec.o", out / "app.o", "-o", elf)
    symbols = build.tc("common/bin/objdump", "-t", elf)
    sections = build.tc("common/bin/objdump", "-h", elf)
    disassembly = build.tc("common/bin/objdump", "-d", elf)
    (out / "smk37-integration.dis").write_text(disassembly)
    (out / "sections.txt").write_text(sections)
    (out / "symbols.txt").write_text(symbols)
    def sym(name):
        return int(re.search(r"^([0-9a-f]+) .*\s" + name + r"$", symbols, re.M)[1], 16)
    image = bytearray()
    for section, address in ((".text", build.APP_XIP), (".ram_text", sym("_rt_load")),
                             (".data", sym("_data_load"))):
        blob = out / (section[1:] + ".bin")
        blob.unlink(missing_ok=True)
        build.tc("common/bin/objcopy", "-O", "binary", "-j", section, elf, blob)
        raw = blob.read_bytes() if blob.exists() else b""
        if not raw:
            continue
        offset = address - build.APP_XIP
        if offset < len(image): raise SystemExit("overlapping image sections")
        image.extend(b"\xff" * (offset - len(image)))
        image.extend(raw)
    image.extend(b"\xff" * (-len(image) % 4))
    if sym("_start") != build.APP_XIP or image[:4] != bytes.fromhex("04818000"):
        raise SystemExit("invalid entry stub")
    # Keep inside the stricter FM-1 allocation as well as stock SMK app extent.
    if len(image) > 0x8DFBC: raise SystemExit("image exceeds conservative XIP allocation")
    for forbidden in ("fm1_flash", "ota_session", "uart_midi_init", "fm1_adc_init", "fm1_input_init"):
        if re.search(r"\b" + forbidden + r"\w*$", symbols, re.M):
            raise SystemExit("wrong-board/flash routine linked: " + forbidden)
    if re.search(r'\b(fl_erase4k_ram|fl_prog_ram|fm1_flash)\b',symbols):
        raise SystemExit('Unexpected flash writer in RAM-only Sloop candidate')
    name = "smk37-integration-NOT-FOR-INSTALL.bin"
    (out / name).write_bytes(image)
    manifest = {
        "board": "SMK-37 Pro & Elite", "firmware": "Sloop SMK 2.6.0-b1", "installable": False,
        "public_build": args.public_release,
        "purpose": "offline linked application integration",
        "storage_mode": "RAM-only projects/presets; browser exports persist separately", "image": name, "bytes": len(image), "sha256": hashlib.sha256(image).hexdigest(),
        "entry": hex(sym("_start")), "bss_end": hex(sym("_bss_end")),
        "pool_end": hex(sym("_pool_end")), "flash_write_support": args.enable_saves, "preset_storage_support": args.enable_presets, "preset_sectors": ["0xf3000", "0xfe000"] if args.enable_presets else [], "usb_update_support": args.enable_usb_update,
        "usb_update_hardware_validated": False,
        "usb_audio_support": args.enable_usb_audio,
        "usb_audio_hardware_validated": False,
        "sample_storage_support": args.enable_samples,
        "sample_storage_requires_explicit_initialization": args.enable_samples,
        "sample_storage_hardware_validated": False,
        "sample_regions": ["0x9d000..0xb1000", "0xc2000..0xd6000", "0xd6000..0xea000"] if args.enable_samples else [],
        "sample_ownership_journal": "0xbf000..0xc0000" if args.enable_samples else None,
        "sample_write_gate": "unit/stock CRCs; explicit resumable initialization; committed ownership; non-contiguous slot bounds; no boot erase" if args.enable_samples else "disabled",
        "trs_midi_output_support": args.enable_trs,
        "trs_midi_output_default": "OFF",
        "trs_midi_output_hardware_validated": False,
        "usb_audio_rates": [44100, 48000] if args.enable_usb_audio else [],
        "update_product": f"SMK-37_9{args.usb_test_number:02d}", "update_version": "SLOOP SMK 2.6.0-b1",
        "upstream_revision": "4365a1d0075b9194de1c44826325abbd39751bc1",
        "upstream_version": "2.6",
        "prior_unit_evidence": "Felucca SMK 0.9.22 on the owner Pro; Sloop artifact not flashed.",
        "flash_write_gate": "Only embedded bounded RAM updater; application has no storage writer",
        "pending_hardware_checks": ["Sloop startup and all controls", "USB update/restart", "long-run USB/audio timing"],

    }
    (out / "manifest.json").write_text(json.dumps(manifest, indent=2) + "\n")
    print(json.dumps(manifest, indent=2))
    return 0


if __name__ == "__main__":
    sys.exit(main())
