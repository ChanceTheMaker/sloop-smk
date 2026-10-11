# Sloop SMK

Sloop 2.6 adapted for **M-VAVE SMK-37 Pro & Elite**, with a themed browser Studio and a playable SMK simulator.

This repository carries the Felucca SMK hardware foundation and the Sloop musical/UI core. It is separate from [Felucca SMK](https://github.com/ChanceTheMaker/felucca-smk).

## What you can do

- Native Sloop sound engines, three synth parts and a drum track, six-operator FM, synth drums, sequencing, song/arranger pages, parameter locks, microtiming, fill conditions, insert effects and texture.
- SMK hardware adapters for the 37-key velocity keyboard, sixteen drum pads, eight encoders, four faders, pitch/mod wheels, sustain, display and indicator lights.
- Grouped Studio controls with per-section knobs/sliders, drag ordering, themes and native editor protocol.
- SMK simulator using the same musical and display source as the hardware candidate; color finishes, touch/mouse keys, full screen and a replayable tutorial.
- USB MIDI input in **both Browser Studio and Simulator**: notes, velocity, sustain, bend and modulation. Browser MIDI input does not request SysEx permission or install firmware.
- Browser sessions preserve virtual projects, user presets, four sample slots, synth drum kits, current FM6 voices and MIDI settings. Export a session file to keep an independent copy.
- The hardware candidate includes the bounded SMK USB updater. Its image and a return-to-Felucca 0.9.22 package pass the host loader test.

## Firmware status

**b2 is the next development step after b1.** Both use Sloop 2.6; the beta suffix identifies this SMK adaptation, not a new upstream Sloop release.

| | 2.6.0-b1 | 2.6.0-b2 |
| --- | --- | --- |
| Sloop engine, native pages and SMK controls | Included | Included |
| Projects and user presets on hardware | RAM-only; lost at power-off | Persistent storage for four projects and 32 presets |
| Settings, autosave and FM6 bank | No persistent hardware saves | Guarded native Sloop storage |
| Boot splash | Initial SMK splash | Centered SLOOP SMK title; actual version in small gray text at the bottom |
| Test status | Owner reports installation from the local page and playing on the SMK | Owner confirms installation and playback; offline tests pass; device persistence tests pending |
| Installer status | Previous local candidate | Public beta installer; device persistence testing pending |

The owner has played Sloop on the physical SMK. A complete check of controls, lights, USB audio and updater re-entry remains. Browser and host tests are tracked separately from device tests in the [port status](docs/SLOOP_SMK_PORT.md).

**Physical sample writes, TRS MIDI routing and Bluetooth are currently disabled in both candidates.** Browser sample uploads and virtual storage work independently of these hardware limits. USB audio code supports 44.1 and 48 kHz; sustained capture on the Sloop hardware port still needs testing.

### How b2 protects stored data

B2 uses paired storage records for projects, two banks of sixteen presets, settings/drum-synth/MIDI-learn data, autosave and the FM6 patch bank. It checks the model/layout and accepts writes only to blank object pairs or valid Sloop-owned records. Existing Felucca saves and unknown data are refused, not converted or silently overwritten. Occupied incompatible slots therefore cannot be used for Sloop saves without a separate migration or storage-initialization workflow.

Host tests cover interrupted commits, recovery of the prior committed copy and rejection of protected or foreign areas. An interrupted first-ever save without a valid commit deliberately blocks further writes to that object after reboot. Hardware power-cycle testing remains the next validation step. See the [storage map and limitations](docs/SLOOP_SMK_PORT.md#storage-enabled-b2-candidate-separate-from-staged-b1).

## Installation and availability

Open the [Sloop SMK installer](https://chancethemaker.github.io/sloop-smk/) in Chrome or Edge. The [Studio](https://chancethemaker.github.io/sloop-smk/editor.html) and [Simulator](https://chancethemaker.github.io/sloop-smk/simulator.html) also work without installing firmware.

1. Select **SMK-37 Pro** or **SMK-37 Elite**.
2. Choose **First installation** for official 016 firmware, or **Update** for Felucca SMK 0.9.22 / Sloop SMK. Older private builds need a separate migration.
3. The page downloads the checksum-verified official model-specific firmware and prepares b2 in the browser. An optional file selector is available if the automatic download fails.
4. Close Studio and other MIDI apps, connect the SMK directly by USB, confirm the beta notice, and click **Install SMK beta**. Keep USB and power connected until completion.

The public download contains the GPL application only. Official firmware is fetched from a pinned external mirror and processed locally; private device backups and calibration are not published. The portable updater retains model/layout validation and each instrument's calibration. It writes the application area; b2 runtime storage follows the separate ownership rules above.

The owner confirmed b2 installation and playback and explicitly approved this beta release with save/power-cycle testing still pending. The public stock-install path is host-tested, not yet confirmed on hardware. Browser session exports are not device backups.

Cloning this repository includes the public app, Studio and Simulator. Running the preview server without a private candidate uses the public installer. A staged private candidate remains available only on localhost; add `?public` to preview the public path locally.

## Try locally

```powershell
python tools/serve_smk37.py --port 8774
```

- Studio: http://localhost:8774/editor.html
- Simulator: http://localhost:8774/simulator.html
- Homepage: http://localhost:8774/

Choose **Browser Studio** to enable audio, or open Simulator. If autoplay is blocked, press a key or Enable audio. To use a connected controller, choose **Enable USB MIDI input** and select its port. Device mode uses the hardware editor and requires a matching Sloop firmware installation.

Studio and Simulator use the same browser session, isolated from Felucca and the FM-1 Sloop website. Session files are tied to their browser runtime revision. The SMK panel is a firmware/UI simulation, not an emulation of the USB controller, flash silicon or Bluetooth radio.

Simulator shortcuts: **1-8** play the top pad row; **Shift+1-8** play the bottom row. Piano shortcuts are **A W S E D F T G Y H U J K O L P ;**; hold **Shift** to play an octave higher.

## SMK controls

| Control | Sloop mapping |
| --- | --- |
| Knobs 1-4, bank 1 | Master level, tempo, preset, track |
| Knobs 1-4, bank 2 | Track, engine, preset, page |
| Knobs 5-8 | Four native page parameters |
| Faders | Track levels; FADER BANK selects envelope ADSR |
| Pads | Sixteen drum lanes |
| PATCH / PARA / BT | Native HOME / SAVE / EDIT buttons |
| ARP and repeat / scale and chord | Native ARP / scale page buttons |
| GLO / FX / SEQ / PLAY / REC | Corresponding native Sloop controls |
| Pitch / mod / sustain | Expressive playing controls |

Native Sloop pages and hold gestures are retained where mapped. Hardware controls, lights and timing still need a complete device pass for this candidate.

## Build and test

The hardware build uses the existing Jieli toolchain with `JIELI_TOOLCHAIN` set to its root, Python dependencies and Pillow with libraqm:

```sh
# RAM-only b1 hardware candidate
python3 tools/build_sloop_smk.py

# b2 hardware candidate with guarded persistent storage
python3 tools/build_sloop_smk.py --enable-storage

# Browser engine
python3 tools/build_sloop_smk_browser.py --zig /path/to/zig
node web/audio/test_smk_engine.mjs
node web/audio/test_smk_worklet.mjs
node web/test_midi_input.mjs
node web/test_simulator_keys.mjs
node web/test_smk37_update.mjs
node web/test_smk37_public.mjs
sh tools/test_smk37_public.sh

# Native storage checks (Linux/WSL with a C compiler)
mkdir -p build
cc -std=c11 -Wall -Wextra -Wno-unused-function tests/sloop_smk_storage_test.c -o build/sloop-storage-test
./build/sloop-storage-test
```

Zig 0.13.0 builds the WebAssembly runtime. Hardware outputs are separated into `build/sloop-smk-2.6.0-b1/` and `build/sloop-smk-2.6.0-b2/`, with a manifest containing the version, hash, memory usage and enabled features. Build outputs and private packages stay under ignored `build/`. The raw application binary is **not** directly installable. Local package preparation is separate, and no build command accesses hardware.

See [port status and validation](docs/SLOOP_SMK_PORT.md) for the pinned upstream revision, artifact hashes, remaining work and test evidence. See [workflow rules](docs/UPSTREAM_WORKFLOW.md) before importing updates or publishing firmware.

## Next hardware milestones

- Complete the Sloop controls, LEDs, editor connection and USB updater re-entry checks.
- Validate b2 project/preset/settings saves and restore behavior across power cycles.
- Integrate physical sample storage and uploads, then TRS MIDI routing.
- Run longer USB audio captures and combined playback/control tests.

Bluetooth remains outside this porting round. Website publication and firmware release are separate: pushing source does not promote a candidate to the installer.

## Source and project layout

The current integration branch is `integrate/sloop-2.6-smk`. Musical upstream is [isod89/sloop-fm1](https://github.com/isod89/sloop-fm1), pinned to Sloop 2.6 commit `4365a1d0075b9194de1c44826325abbd39751bc1`.

| Directory | Contents |
| --- | --- |
| `sloop/` | Vendored upstream musical/UI core and tools; local changes listed in `sloop/UPSTREAM.txt` |
| `firmware/` | SMK hardware adapters, guarded storage and USB updater |
| `web/` | Homepage, Studio, Simulator and browser audio runtime |
| `tools/` | Build tools, local preview server and packaging utilities |
| `tests/` | Host checks for hardware interfaces, storage and update boundaries |
| `docs/` | Port status, hardware evidence, workflow rules and remaining work |

## Credits and license

Sloop firmware by **isod89**, based on Felucca by **Leo Kuroshita (@kurogedelic), Hugelton Instruments**. SMK adaptation, Studio and simulator by **Chance Roth (@ChanceTheMaker)**.

GPL-3.0; retain upstream copyright notices, [LICENSE](LICENSE), [LICENSING.md](LICENSING.md) and [LICENSES](LICENSES). Vendored Sloop source lives under `sloop/`. Vendor firmware, calibration records and private device backups are not distributed here. M-VAVE, FM-1 and SMK-37 are trademarks of their respective owners.
