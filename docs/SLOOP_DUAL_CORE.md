# Sloop SMK dual-core beta

Sloop SMK **2.6.0-b8-dc1**, based on musical upstream Sloop 2.6
`4365a1d0075b9194de1c44826325abbd39751bc1` and the SMK b7 release.
Release 2026-10-12, following the Pro checks below.

## What changes

Pairs of FM6 voices can render concurrently: one on CPU1 and one on CPU0.
Each core has private operator scratch buffers; completed voices are added in
original order. The existing integer FM6 engine, presets and voice limits are
preserved. Other engines, controls, sequencing, FX, USB, display and storage
remain on CPU0. This does not double all-engine performance or add polyphony.
The browser Studio and simulator remain single-threaded and retain their
existing musical behavior.

CPU1 launch, mailbox checking, private 4 KiB user + 4 KiB supervisor stacks,
SRAM idle loop and fault retirement are adapted from the tested Melodee SMK dc7
source (ChanceTheMaker/melodee-smk `ae898250699828ea728ab549b2ac1f99e69948c0`).
The original Melodee worker is retained with its GPL license; Sloop's different
FM6 engine is adapted separately. No upstream contribution has been submitted.

A failed handshake stays serial. Mailbox rejection, CPU1 fault or a bounded
10 ms join timeout retires CPU1. A partially advanced voice block is dropped,
never recomputed; later blocks run serially. Retirement is remembered across
warm resets until a cold power-on. CPU1 is held before RAM initialization,
fatal recovery and USB loader handoff. CPU0 is the only flash owner; every
render returns with all jobs joined. At rest CPU1 executes only copied SRAM.

No oscillator, PLL, voltage, flash layout, calibration, storage format,
USB endpoint format or updater write boundary changes are included.

## Artifact

- Identity: `SMK-37_937`
- Application: 580740 bytes (824 bytes below the existing 581564-byte limit)
- SHA-256: `d5cba861417afb570bb3eca5dc910a7fd003bcb5a5c403b78a1770565a726798`
- BSS end: `0x01c196e8`; pool end: `0x01c70af4`
- CPU1 stacks: 8192 bytes; 5376 bytes remain before the CPU0 guard region.
- Linked audit checks the 41-instruction SRAM poll loop and its single callback.

## Validation

- Concurrent pthread execution through the real SMK runtime: 8840 completed
  worker jobs; serial/concurrent PCM byte-identical. Exercises both FM6 tracks,
  factory sounds, algorithm changes, sustain, wheels, pads, faders and an
  engine switch. This is host evidence, not a CPU-utilization measurement.
- Worker tests: busy/offline, stalled/faulted worker, corrupt mailbox, timeout
  wrap and bounded retirement. Lifecycle tests cover cold/warm boot, retry,
  handshake failure, crash and deliberate updater stop.
- Upstream FM6 regression: all 32 algorithms, envelopes, all factory patches,
  macros, retriggering, patch formats and bank storage passed.
- Golden audio regression: 182 reference renders unchanged, zero health,
  voice/routing or crash failures. Shared-host CPU timings are not device
  performance evidence. Website publication regressions also pass.
- SMK candidate suite: native project/preset persistence, interrupted storage,
  foreign-record protection, samples, MIDI/TRS routing, pad feedback and lights.
- Pro and Elite official 016 fixture packaging: full checksum roundtrip,
  corrupt-image rejection and protected bytes unchanged. No hardware flashed
  by these offline tests.

Pro hardware evidence on 2026-10-12: the owner installed from Doom 008 through
the normal USB updater and booted Sloop b8-dc1. Read-only telemetry reported
CPU1 online and, after FM6 chord playing, 16,016 jobs / 16,014 pairs, zero
timeouts, rejected reads or faults, and no warm-reset bar. Maximum callback
was 1,523 timer ticks (63.5 microseconds); maximum join wait was 551 ticks
(23.0 microseconds). These maxima describe this short test, not all patches or
a worst-case performance guarantee. One earlier idle telemetry session timed
out after its first reply; the subsequent 30-second capture completed.
The owner confirmed clean sound and responsive controls, then cold-restarted
the device. USB again reported b8-dc1, CPU1 online, zero fault counters and no
warm-reset bar. This is a focused Pro acceptance check, not exhaustive Elite
or long-duration USB-audio validation. Previous b5/b6
storage/TRS and b7 display evidence remains attributed to those builds.
Physical sample upload and Elite first-install remain separate follow-ups.

## Reproduce

```sh
JIELI_TOOLCHAIN=/path/to/toolchain python3 tools/build_sloop_smk.py \
  --enable-storage --enable-samples --enable-trs --dual-core
mkdir -p build/sloop-gen
cp build/sloop-smk-2.6.0-b8-dc1/gen/*.h build/sloop-gen/
sh tools/test_sloop_dual_core.sh
sh tools/test_sloop_smk_candidate.sh
```

Omit `--dual-core` for a serial comparison. That build is separate and does not
replace the previously published b7 binary.

Read-only telemetry uses `7D 53 43 31 01 00` (SC1/version 1/op 0), separate
from upstream command IDs and storage diagnostics. Replies carry nine unsigned
32-bit counters in five 7-bit bytes each: online, jobs, paired voices, timeouts,
rejected mailbox reads, faults, maximum job ticks, maximum wait ticks, warm-reset
bar. Timer ticks use the existing 24 MHz timer. No commands write settings.

```sh
python tools/sloop_smk_core_diag.py --input "<MIDI input>" \
  --output "<MIDI output>" --seconds 30
```

Private vendor fixtures, device backups, packages and recordings stay under
ignored `build/`; only the GPL application is eligible for public staging.
