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

**The public beta is 2.6.0-b7**, following the owner's b5/b6 hardware checks
and b7 screen confirmation. The beta suffix
identifies this SMK adaptation; the musical upstream remains Sloop 2.6.

| Area | Current beta status |
| --- | --- |
| Project storage | Verified flash commits before reporting Saved; protected slots show LOCKED. Owner confirmed A-D persistence during b5 testing. |
| User presets | 32 slots; U01 save, restart, full readback and load checked on b6. |
| TRS MIDI OUT | Notes/chords, sustain, pitch, saved ON preference and held-note shutdown checked on b5/b6. Receiver expression check used FM-1 Sloop 2.6. |
| USB audio | b2 stereo captures at requested 44.1/48 kHz passed; Windows may resample. These are not fresh b7 timing measurements. |
| Pads and lights | Same-pad blue feedback and individual color diagnostic; rare green flash remains on the watch list. |
| Physical samples | Three 80 KiB slots, guarded preparation and uploads implemented; physical upload/restart testing deferred. Browser retains four virtual slots. |
| Oscilloscope | b7 removes pre-redraw black clears and refreshes the scope every UI tick. Builds and host regression pass; owner confirmed the screen improvement. |

B7 publication was explicitly authorized after the device checks. See
[candidate release notes](docs/SLOOP_SMK_CANDIDATE.md) and the detailed
[port status](docs/SLOOP_SMK_PORT.md) for version-specific evidence and limits.
Bluetooth remains outside this round; physical TRS is output only.

### Storage protection

Paired records hold projects, two banks of sixteen presets, settings, autosave
and FM6 data. Model/layout and ownership checks prevent silent replacement of
Felucca saves or unknown records. A failed project commit leaves the previous
RAM slot intact; Studio reports the device's save result. Older b2 replies do
not provide that result and are identified accordingly.

Host tests cover interrupted writes and protected areas. The owner's legacy
A-C project pairs were backed up and explicitly repaired during b5 testing;
that is a separate local operation, not automatic migration or a public upgrade
requirement. See [storage diagnostics](docs/SMK_STORAGE_DIAGNOSTICS.md).
Physical sample preparation is also explicit and never runs automatically at boot.

## Installation and availability

Open the [Sloop SMK installer](https://chancethemaker.github.io/sloop-smk/) in Chrome or Edge. The [Studio](https://chancethemaker.github.io/sloop-smk/editor.html) and [Simulator](https://chancethemaker.github.io/sloop-smk/simulator.html) also work without installing firmware.

1. Select **SMK-37 Pro** or **SMK-37 Elite**.
2. Choose **First installation** for official 016 firmware, or **Update** for Felucca SMK 0.9.22 / Sloop SMK. Older private builds need a separate migration.
3. The page downloads the checksum-verified official model-specific firmware and prepares b7 in the browser. An optional file selector is available if the automatic download fails.
4. Close Studio and other MIDI apps, connect the SMK directly by USB, confirm the beta notice, and click **Install SMK beta**. Keep USB and power connected until completion.

The public download contains the GPL application only. Official firmware is fetched from a pinned external mirror and processed locally; private device backups and calibration are not published. The portable updater retains model/layout validation and each instrument's calibration. It writes the application area; b7 runtime storage follows the separate ownership rules above.

The owner confirmed project/preset persistence on b5/b6 and the b7 display improvement before approving b7 publication. The public stock-install path is host-tested, not yet confirmed on hardware. Browser session exports are not device backups.

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

Native Sloop pages and hold gestures are retained where mapped. Hold PATCH for
settings; bank-1 K2 selects SCREEN / LIGHTS / AUDIO / SYSTEM. K5-K7 edit the
three rows; OCT- exits and saves. LIGHTS includes brightness and the individual
pad-color diagnostic; AUDIO includes TRS MIDI OUT. SYSTEM offers explicit user
sample preparation. Sample preparation is deferred for this test round.

## Build and test

The hardware build uses the existing Jieli toolchain with `JIELI_TOOLCHAIN` set to its root, Python dependencies and Pillow with libraqm:

```sh
# Local b7 candidate with guarded storage, samples and TRS output
python3 tools/build_sloop_smk.py --enable-storage --enable-samples --enable-trs

# Browser engine
python3 tools/build_sloop_smk_browser.py --zig /path/to/zig
node web/audio/test_smk_engine.mjs
node web/audio/test_smk_worklet.mjs
node web/test_midi_input.mjs
node web/test_simulator_keys.mjs
node web/test_smk37_update.mjs
node web/test_smk37_public.mjs
sh tools/test_smk37_public.sh

# Candidate checks (Linux/WSL with a C compiler)
sh tools/test_sloop_smk_candidate.sh
python3 tests/test_sloop_smk_storage_reset.py
node web/test_project_save.mjs
node web/test_sample_capability.mjs
```

Zig 0.13.0 builds the WebAssembly runtime. Current hardware outputs are isolated in `build/sloop-smk-2.6.0-b7/`, with a manifest containing the version, hash, memory usage and enabled features. Build outputs and private packages stay under ignored `build/`. The raw application binary is **not** directly installable. Local package preparation is separate, and no build command accesses hardware.

See [port status and validation](docs/SLOOP_SMK_PORT.md) for the pinned upstream revision, artifact hashes, remaining work and test evidence. See [workflow rules](docs/UPSTREAM_WORKFLOW.md) before importing updates or publishing firmware.

## Remaining checks

- Physical sample upload and restart persistence remain deferred.
- Watch for stray pad LEDs; confirm modulation with a known responsive receiver patch.
- Elite first-install and exhaustive hardware coverage remain separate from the owner's Pro results.

Prior audio testing is retained; another long capture is not a routine acceptance
requirement without a relevant change or regression. See [work queue](docs/WORK_QUEUE.md).
Website publication and firmware release are separate: pushing source does not
promote a candidate to the installer.

## Source and project layout

Current development is on `feat/sloop-smk-b3-hardware-ui`, following `integrate/sloop-2.6-smk`. Musical upstream is [isod89/sloop-fm1](https://github.com/isod89/sloop-fm1), pinned to Sloop 2.6 commit `4365a1d0075b9194de1c44826325abbd39751bc1`.

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
