# Sloop SMK candidate: 2.6.0-b7

2026-10-11. The owner confirmed the b7 screen improvement ("Looks good") and authorized
public b7 publication. B7 replaces public b2; earlier b5/b6 checks remain
version-specific evidence.
Upstream musical/UI source remains Sloop 2.6 at
`4365a1d0075b9194de1c44826325abbd39751bc1`. These are fork changes, not an upstream PR.

## Changes since public b2

- Project saves report success only after a verified flash commit. Failed saves
  preserve the prior RAM slot; locked/unavailable storage is visible on-device
  and in Studio. Corrected the erase driver's missing elapsed-time pointer.
- Added bounded read-only project diagnostics and a separately authorized local
  A-C legacy repair tool. No boot-time conversion or general public migration.
- Added SMK settings labels, explicit sample preparation, TRS OUT preference,
  pad brightness and individual pad/color tests. Physical sample storage uses
  three guarded 80 KiB slots; the browser retains four virtual slots.
- Pad feedback follows pad identity, holds short taps visibly and excludes
  sequencer activity. Rare stray green feedback remains under observation.
- TRS output sends expression and retains its ON setting. Disabling it drains
  channel resets and explicit Note Off messages for transmitted notes before
  stopping the UART, including receivers that ignore all-notes-off.
- **b7:** both scope views redraw on every 40 ms UI tick rather than every
  second tick (nominal ceiling 25 vs 12.5 fps). Full-screen visualizer redraws
  replace completed bands directly, without first transmitting a black clear.
  Heavier styles retain half-rate scheduling and title duration is preserved.
  Audio clocks, interrupt scheduling and SPI clock are unchanged. The owner confirmed the screen improvement; actual frame rate was not measured.

## Validation

- Candidate host suite passed for storage, samples, MIDI queue/UART routing,
  pad feedback, persistence and explicit storage repair. Python diagnostic/reset
  tests and Studio save/sample capability regressions passed.
- Owner confirmed all four project slots across the b5 persistence checks.
  U01 preset save, restart readback and load passed on b6; 71 parameters checked
  with deliberate mix/pattern preservation. This is not exhaustive bank testing.
- Physical TRS notes/chords, ON persistence, held-note shutdown, sustain and pitch
  passed across b5/b6. Expression receiver was FM-1 Sloop 2.6.
- Prior b2 stereo audio captures: 120 seconds each at requested 44.1 and 48 kHz,
  signal in every one-second window, no clipped samples, maximum exact-zero run
  below 1 ms. Windows WinMM may resample; this does not prove USB clock accuracy.
  The duplicate b6 recording was stopped and is not counted as a pass.
- b7 hardware and browser builds pass. Regression executes the production
  visualizer scheduler with a recording LCD backend: full-rate scope, no black
  clear on entry/forced redraw, complete bands and half-rate other styles.
  Native browser engine smoke tests pass. Offline b6-to-b7 loader test changed
  141 application sectors and preserved the saved-data tail.

## Remaining / deferred

Physical sample upload/restart (owner deferred);
pad LED watch; audible TRS modulation with a suitable patch; Elite first-install.
Do not repeat long audio captures absent a concrete regression or relevant
audio-path change. Bluetooth and public legacy migration are outside this round.

## Local artifact identity

- Product: `SMK-37_936`
- Application: 579504 bytes;
  SHA-256 `aafc79a98733a923e77afc51afa2178671b636914f70292b857d19f011678c19`
- Private package: 627712 bytes;
  SHA-256 `d0ddfd08862001eec1affe75c7947df18561a0445b746e22525659b63b085e73`
- Browser runtime: 2145598 bytes;
  SHA-256 `b83dc6ab0be35039c5055aea4b3f766a87ce253ea9d104c847730a4a6875e68e`
- BSS end `0x1c194c8`; pool end `0x1c70af4` (unchanged from b6).

Private packages, backups, recordings and repair receipts stay under ignored
`build/`. Detailed evidence and chronology: [port log](SLOOP_SMK_PORT.md).
