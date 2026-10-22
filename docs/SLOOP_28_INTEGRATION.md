# Sloop SMK 2.8 integration candidate

Branch: `integrate/sloop-2.8-smk`. Base: `10eaeeb9286ec92ee644593ddf128f8c4493439b`.
Upstream v2.8: `b77bf55e3bf804665b8f5dffb5955bd8443311f0`.
Previous musical base: `4365a1d0075b9194de1c44826325abbd39751bc1` (2.6).
This is an attributed source import, not a merge of upstream history.
Original notices and licenses are retained. No upstream PR or message.

## Included changes

- Device NOTES and DRUMS editing sheets; note/step move, copy, delete, nudge,
  fill conditions and ratchets; unquantized recording and held-note overdub.
- New arpeggiators, track colors, drum tuning/decay and filter resonance;
  upstream MIDI clock, bank-select and remote-control updates.
- Matching 12-tab editor, LIVE controls, themes and song files. The native
  browser simulator uses the same SMK core; optional mock preview uses upstream DSP.
- Upstream rendering across parts and drums on CPU1, replacing our FM6-only
  worker. The checked SMK startup/stop/mailbox/timeout lifecycle is retained.

## Deliberate SMK differences

The original-firmware return flow, native Windows installer isolation and
updater compatibility changes remain. FM-1 voltage/HAL changes are not used by
SMK. No experimental Bluetooth code was imported.

Protected flash regions, sample ownership, three physical sample slots and
four browser RAM slots stay as before. Preset RAM caching and verified save
commit ordering stay intact. Existing TRS bit26 is preserved: CPU1 OFF uses
otherwise unused bit3 on SMK. Non-SMK upstream uses its original bit26.
A failed CPU1 job is dropped instead of advancing partially rendered voices twice.
Diagnostic word2 now counts submitted render blocks, not FM6 pairs.

Read-only saved preferences load before USB and CPU1 start, so the chosen USB
rate and CPU1 OFF take effect that boot. Remote buttons combine with physical
holds. LIVE master volume affects output until K1 is moved. UI functions use
minsize while DSP retains speed optimization. Upstream .lowram data occupies
zero-initialized ordinary RAM, with no flash map or reserved stack boundary change.

Browser session v2 stores new side data and MIDI settings. Real v1 snapshots
from the previous published WASM load with defaults for new settings. CRC checks
happen before state mutation. Session discovery includes the previous published
runtime key; explicit import no longer rejects an otherwise compatible runtime hash.

## Candidate artifacts (private build outputs)

`build/sloop-smk-2.8.0-b1/manifest.json`, USB `SMK-37_950`, version `2.8.0-b1`.
Application: 576,288 bytes, SHA-256
`b83df1987733730012574dfd1ccdb8494649e4b6450f9daf7794ab736890d64f`.
BSS end `0x1c1821c`; pool end `0x1c70af4`; pool headroom 5,376 bytes.
CPU1 uses private 8 KiB stacks; linked SRAM idle audit passes.
Pro and Elite packages were separately constructed against their original 016
packages, with protected bytes and package roundtrips verified. Those vendor
images and generated packages remain ignored/private. The raw integration image
is not a directly installable file.

Build uses JieLi Linux toolchain 20250324.1, Pillow 12.3 with libraqm, and Zig
0.13 for browser WASM. Reproduce using `tools/build_sloop_smk.py` with
`--enable-storage --enable-samples --enable-trs --dual-core`; browser builds use
`tools/build_sloop_smk_browser.py` and `tools/build_sloop_preview.py`.
`tools/package_sloop_candidate.mjs` constructs an offline candidate only.
`tools/sloop_candidate.py check DIRECTORY` verifies its pinned app/package hashes
without opening MIDI. Its explicitly invoked `install` command keeps the shared
native USB lock across disposable begin/write/verify processes; it is not exposed
by the public website. The public helper still refuses candidate 950 by default.

## Validation

Passed:

- SMK storage/power-loss/protected-region, sample ownership, routing, lights,
  failed project save, cold reload and legacy reset host tests.
- New preference-bit compatibility, LIVE/K1 gain, remote/physical button
  ownership, side-data roundtrip and CRC rejection tests.
- Concurrent and serial PCM identical for 4,096 jobs, including all engines on
  CPU1; worker fault, timeout and lifecycle tests.
- Upstream seq2 and project tests against imported source; UI editing tests
  including 20,000 randomized frames and note/drum sheet operations.
- Sloop 2.8 editor protocol suite, including old-firmware fallbacks, backup,
  theme, song-file, LIVE, drum, ratchet, fill and piano-roll operations; all
  12 tabs and English/Japanese string coverage.
- Native WASM and audio worklet tests at 44.1/48 kHz; previous published
  runtime snapshot migration and corrupt-session rejection.
- Native installer mock/HTTP checks, package construction, progress, updater,
  original Pro/Elite restore, MIDI input and simulator keyboard tests.
- Headless Chrome native browser startup, 12 tabs, piano-roll editing and LIVE
  play/stop, desktop/mobile rendering and mock-mode audio startup. No USB or
  Web MIDI opened by those browser checks.

## SMK Pro hardware check — 2026-10-21

The owner explicitly approved installing the candidate. The exact application
above was packaged with original Pro 016 and installed from `Doom-SMK-37_023`
through its native USB loader. Restart replied as `SMK-37_950`; editor INFO
confirmed `SLOOP SMK 2.8.0-b1`, protocol 12. Chrome was not used for USB.

LIVE transport started and the sequencer step advanced. Three temporary rolling
notes exercised the sound engine. CPU1 jobs increased from 60,456 to 76,864,
with zero timeouts, rejected reads, faults or barred-worker flags. The largest
observed job was 6,286 timer ticks. This is a short functional test, not a
worst-case performance benchmark.

An eight-second stereo capture from the exact Windows input `Line (Felucca)`
contained signal in every one-second window. Left/right peaks were 13,559/13,522
with zero clipped samples. Windows captured at 48 kHz and may resample; this
does not establish the physical USB endpoint rate. Previous transport and
master-volume state were restored; no project save/erase was requested.
Evidence remains private in `build/sloop-smk-2.8.0-b1/hardware/` (`smoke.json`,
`usb-48000.json`, `usb-48000.wav`).

Physical controls and power-cycle confirmation are requested from the owner.
Long-duration USB/audio, physical save/reload and this candidate's return to
original firmware remain untested. Elite package validation is not an Elite
hardware test. No public firmware selection has changed.
