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

## Not yet hardware validated

No Sloop candidate has been flashed. Hardware audio/control/LED timing and re-entry to the USB updater need a device pass. Host tests and browser DSP tests are not device validation.
Hardware projects and presets are RAM-only and reset on power-off. Physical sample storage, TRS MIDI routing and Bluetooth are disabled. Browser virtual storage does not imply hardware storage support. Existing device flash data outside the application is preserved, not imported into Sloop.

Next: first controlled hardware boot/audio/control check; confirm USB identity/editor and updater re-entry; then port persistent project/preset/sample storage with explicit layout validation, followed by TRS routing and longer USB audio tests.

## Publication

Private packages, baseline images and calibration remain in ignored `build/`. The source branch and browser assets may be pushed independently. No public Sloop firmware release or upstream contribution has been made by this round. Do not flash automatically.
