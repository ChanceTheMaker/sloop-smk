# Sloop SMK 2.6.0-b1 port status

Upstream: isod89/sloop-fm1 main `4365a1d0075b9194de1c44826325abbd39751bc1` (2.6).
SMK hardware baseline: Felucca SMK `fb03412`, with the subsequent 0.9.22 public updater validation changes retained.
Sloop source is vendored under `sloop/`, not represented as a merge of upstream history. See `sloop/UPSTREAM.txt` for local vendor changes.

## Implemented

- Native Sloop DSP, editor protocol, pages, menus and display; SMK key/pad/encoder/fader/wheel/pedal and LED adapters.
- Native browser engine shared by Studio and the physical-style SMK simulator; seven grouped control sections.
- Browser USB MIDI input in both views. Native sample uploads, projects, presets, drumsynth/FM6 state and MIDI settings persist in browser sessions.
- Sloop sail logo and palette; consistent SMK lettering; hero uses a capture of the native Sloop LCD.
- Simulator number shortcuts: 1-8 top pads, Shift+1-8 bottom pads. Shift plus piano keys raises pitch an octave. Release ownership is independent of Shift state.
- Bounded RAM USB updater, normal MIDI identity, host-tested local candidate and return-to-Felucca package.

## Candidate and validation

Application: `a645df9c0595caeed97127c499774e5cc2464978d4ae03a5aa5064c150d1a34d`, 556504 bytes.
BSS ends at `0x1c19e68`; pool ends at `0x1c6c174`. The preset cache uses the existing pool; no memory boundaries were widened.

Passed: native engine protocol descriptors, SET/GET, project/session round trips, four virtual sample slots and upload, LCD rendering, key sound, faders and 18 buttons; explicit start/stop; worklet audio at 44.1 and 48 kHz; USB MIDI routing/cleanup; simulator pad and octave shortcuts; web updater package/disconnect/model checks; actual C loader boundary/failure tests.
Both the final private Sloop package and reverse Felucca package passed the C loader against full baseline images: 136 changed sectors, saved tail preserved. The reconstructed return image is byte-identical to its Felucca 0.9.22 baseline.
Browser checks: standalone simulator starts on a number-key gesture, native LCD renders, pad shortcuts and USB MIDI selector are present, no console errors. Studio successfully loaded the Sloop protocol and grouped controls.

## Storage-enabled b2 candidate (separate from staged b1)

Built with `python tools/build_sloop_smk.py --enable-storage`. Product `SMK-37_931`.
Application: `f88fa4c828f1ca4f80c311ab2a3f2e2b35f9344cf5c1058ae78ac9aa7f1a2000`, 572424 bytes. BSS end `0x1c19f28`; pool end `0x1c6faf4`. No linker boundaries were expanded.

Native Sloop A/B commits now support four projects, two banks of sixteen presets, settings/drumsynth/MIDI learn, autosave and the FM6 patch bank. Runtime validates the flash ID, official boot-region fingerprint and calibration structure before storage access. Each object is writable only when its pair is blank or has a valid Sloop commit. No format-on-boot path exists. Felucca saves and unknown contents are refused; a refused save reports an error and does not replace those sectors.

| Object | Copy A | Copy B |
| --- | --- | --- |
| Settings | 0xb1000 | 0xb2000 |
| Presets 1-16 | 0xb3000 | 0xb4000 |
| Presets 17-32 | 0xb5000 | 0xb6000 |
| Autosave | 0xb7000 | 0xb8000 |
| Projects A-D | 0xeb000, 0xed000, 0xef000, 0xf1000 | 0xec000, 0xee000, 0xf0000, 0xf2000 |
| FM6 bank | 0xf3000 | 0xfe000 |

Native record sizes fit single sectors with 256-byte headers. The sample regions, sample ownership journal, calibration, bootloader, stock VM and application are excluded from the storage writer. Physical sample editor addresses are explicitly rejected while that feature remains disabled. FM6 uses the SMK plaintext XIP boundary at 0x9d000, never the overlapping FM-1 boundary.

Host tests passed all nine objects, distinct addresses, save/load, interruption at every commit stage, fallback to the prior committed copy, no-write rejection of foreign/Felucca data, zero/overflow/protected-range rejection and refusal after an interrupted first-ever save. The final b2 private package passed the actual C loader: 140 changed sectors; the saved tail was preserved. Browser engine/worklet, 44.1/48 kHz audio, MIDI and simulator shortcut regressions passed after the shared-source changes.

An interrupted first-ever save without any valid commit deliberately needs recovery; the firmware will not infer ownership from partial contents. Returning to Felucca after writing Sloop records does not convert those records: incompatible sectors may be refused by Felucca. Export backups before switching firmware. No existing Felucca project is silently migrated or overwritten.

B2 is built and privately packaged, **not staged in the installer or flashed**. Localhost:8774 still serves b1 for the initial Sloop hardware boot/control check. Storage power-cycle and fault behavior still require a device pass.

## Not yet hardware validated

The owner reports installing Sloop from the local webpage and playing it on the physical SMK. This confirms a basic boot/play check; the exact displayed version has not yet been read back. Exhaustive controls, LEDs, long-run audio and re-entry to the USB updater still need a device pass. Host tests and browser DSP tests are not device validation.
In staged b1, hardware projects and presets are RAM-only and reset on power-off. Physical sample storage, TRS MIDI routing and Bluetooth are disabled. Browser virtual storage does not imply hardware storage support. Existing device flash data outside the application is preserved, not imported into Sloop.

Next: first controlled hardware boot/audio/control check; confirm USB identity/editor and updater re-entry; then validate b2 persistence on device; physical sample integration, TRS routing and longer USB audio tests remain.

## Publication

Private packages, baseline images and calibration remain in ignored `build/`. The source branch and browser assets may be pushed independently. No public Sloop firmware release or upstream contribution has been made by this round. Do not flash automatically.

## SMK splash

The boot title is centered on the 240-by-240 display, with the build version in the small font, centered ten pixels above the bottom. The version comes from the firmware identity rather than a hard-coded b1 string. Sloop's native font renders the title as `SLOOP SMK`.
