# Sloop SMK 2.6.0-b2 port status

## Current local candidate (2026-10-11)

B7 is the selected public beta after the owner's screen confirmation and
publication authorization. It includes the b5/b6 persistence and TRS work;
see [release notes](SLOOP_SMK_CANDIDATE.md) for exact artifacts and version-specific
evidence. Source and public application are published together. Sample hardware
testing remains deferred; prior b2 audio evidence is retained without repeat
recordings. No upstream contribution or public legacy-migration workflow.

The entries below retain their historical versions and validation status.

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

B2 is built, privately packaged and **staged at localhost:8774 at the owner's request**. The owner confirms b2 installation and playback; save and power-cycle testing are pending. Storage power-cycle and fault behavior still require a device pass.

## Not yet hardware validated

The owner reports installing Sloop from the local webpage and playing it on the physical SMK. This confirms a basic boot/play check; the exact displayed version has not yet been read back. Exhaustive controls, LEDs, long-run audio and re-entry to the USB updater still need a device pass. Host tests and browser DSP tests are not device validation.
In the previous b1 candidate, hardware projects and presets are RAM-only and reset on power-off. Physical sample storage, TRS MIDI routing and Bluetooth are disabled. Browser virtual storage does not imply hardware storage support. Existing device flash data outside the application is preserved, not imported into Sloop.

Next: complete hardware controls/LED checks; confirm USB identity/editor and updater re-entry; then validate b2 persistence on device; physical sample integration, TRS routing and longer USB audio tests remain.

## Publication

Private packages, baseline images and calibration remain in ignored `build/`. The source branch and browser assets may be pushed independently. The owner explicitly approved public b2 publication after confirming installation/playback, with save/power-cycle checks still pending. The public release publishes only the GPL app and prepares model-specific packages in the browser. No upstream contribution is included. Do not flash automatically.

## SMK splash

The boot title is centered on the 240-by-240 display, with the build version in the small font, centered ten pixels above the bottom. The version comes from the firmware identity rather than a hard-coded b1 string. Sloop's native font renders the title as `SLOOP SMK`.

## Public b2 release

The installer supports official Pro/Elite 016 first installation and portable updates from Felucca SMK 0.9.22 or Sloop SMK. Official fixtures are SHA-checked against the pinned public mirror. Both models passed container/CRC round trips, protected-area checks, corruption rejection and the actual C loader host test (152 changed sectors on synthetic stock baselines; saved tails preserved). The Pro parser uses logical coordinates across its interleaved marker bytes, carried over from the proven Felucca public packaging fix.

The web release allowlist contains only `public-release.json` and `sloop-smk-2.6.0-b2-app.bin`; no private update packages, device images or vendor firmware are staged. Application SHA-256 remains `f88fa4c828f1ca4f80c311ab2a3f2e2b35f9344cf5c1058ae78ac9aa7f1a2000`. Device b2 persistence and public stock-install testing remain pending.

All 24 light/dark theme thumbnails were recaptured from the native browser Studio with the Sloop SMK header and b2 runtime label; gallery and hover-preview URLs are cache-busted.


## b3 local integration candidate (2026-10-11)

Branch `feat/sloop-smk-b3-hardware-ui` implements the owner's six-item UI/features
round. No b3 device flash, public firmware replacement, push or PR has occurred.
The loopback preview at `http://localhost:8774/?private` serves the candidate;
the public release manifest/application remain b2. Browser runtime now identifies
as b3 and includes the shared menu/pad-feedback changes.

- Device menu: SMK knob labels K5-7; TRS MIDI OUT replaces the unavailable USB
  serial setting. USER SAMPLES replaces FM-1 hardware calibration (which must
  not run on the SMK). Browser menu marks physical actions DEVICE ONLY.
- Pad feedback: same physical pad ID, blue for at least 120 ms after an attack,
  remains lit while held; no sequencer-to-pad translation. Unchanged frames are
  not resent; a transport timeout stops further attempts. Brightness follows
  OFF/LOW/MID/HIGH. LIGHTS > PAD LIGHT TEST cycles PLAY, ALL OFF, PAD1-16 BLUE,
  PAD1-16 GREEN, PAD1-16 RED. K7 changes this setting; exiting the menu returns
  to PLAY. This is a diagnostic, not a claim that the reported green glitch is fixed.
- Physical samples: three noncontiguous 80 KiB slots (data starts at +512), using
  the existing SMK ownership journal bound to this unit's trim CRC. No format on
  boot. Hold PATCH, K2 to SYSTEM, K3 to USER SAMPLES, OCT+ to show the replacement
  confirmation, then OCT+ again within ten seconds. OCT- cancels. Preparation
  replaces factory sample contents; already-owned slots are not erased.
  Uploads are refused while the sequence is playing. Native sample command IDs
  and packet shapes are retained; header magic is published last. Browser keeps
  four RAM slots; the internal fourth hardware slot remains an empty placeholder.
  Studio explains the preparation step when b3 reports zero available slots.
- TRS output: OFF by default, bounded UART queue, notes and expression, USB stays
  independent. A disable requests sustain-off/all-sound-off/all-notes-off before
  closing UART. Setting persists in packed settings bit26. Physical note-offs
  retain their original channel after a MIDI-channel change. TRS input and
  Bluetooth are not part of this candidate. Actual timing/disable behavior still
  needs a Sloop device check, particularly the prior Felucca stuck-note report.
- Saves: b2 project/preset restart validation is in progress with the owner.
  PROJECT is the save/load page; SONG chains A-D sections. From Home, tap PARA
  through PRESETS, USER, PROJECT. K5 selects the slot, K7 loads, K8 saves; LOAD
  and SAVE each require a second detent within about 1.5 seconds. Keep used slots.

### Reported physical lighting defect

Owner reports the pad touched correctly lights dim blue, but another physical
pad flashes green. It also occurs on a single tap with the sequence stopped.
This rules out ordinary intentional sequencer feedback: b2's physical pad
serializer only requests blue. Input/LED numbering has NOT been permuted on the
basis of this report. The precise transfer/electrical cause remains unresolved.
Use b3's ALL OFF and individual solid-color tests to distinguish display/SPI
interference from physical chain mapping, then retest taps with playback stopped
and running. Do not mark this fixed based on host tests.

### Hardware audio evidence

Owner ran a repeating sequence. Only Windows `Line (Felucca)` was recorded:
120 seconds stereo at requested 44100 Hz and 120 seconds at 48000 Hz. Both had
signal in all 120 one-second windows/channel, zero clipped samples; longest
exact digital silence was 0.8 ms or less. RMS was about -35.4 dBFS (44.1k) and
-36.4 dBFS (48k). Files/JSON stay under ignored `build/sloop-b2-audio-20261011`.
Windows can resample: this demonstrates sustained capture at both requested
rates, not independently proven USB alternate-rate switching or a full latency/
dropout analysis. User-reported b2 physical playback remains the tested version.

### Local validation

- Jieli application/loader build within unchanged XIP/RAM/pool/stack bounds.
  The 4 KiB audio DMA buffer moved to the existing SRAM pool; no memory region
  was expanded. Browser DSP rebuilt from the same runtime adapter.
- Peripheral host suite (including input mapping, scan, GPIO, LCD, lights,
  timer, startup and new pad feedback) passed ASan/UBSan tests.
- `sh tools/test_sloop_smk_candidate.sh`: native nine-object storage commits,
  interrupted/foreign saves, native physical-sample adapter and wire uploads,
  interrupted initialization and header publication, CRC/duplicates/playback
  refusal/protected ranges, TRS queue/DMA/panic, MIDI OFF/expression routing,
  original note-off channels and packed TRS preference all passed.
- Native browser engine descriptors, 4 virtual samples/uploads, snapshots,
  project operations, buttons/faders, all16 pad feedback IDs; worklet at44.1/48k;
  browser MIDI input and simulator keyboard shortcuts passed. Zero-slot physical
  connection regression remains passing. Firmware updater browser tests passed.
- Actual candidate package accepted by browser parser and host C loader;
  141 changed application sectors, saved tail unchanged in the simulation.
  No physical erase/program was performed by these checks.

Device b3 acceptance remains pending: blue/green pad behavior, menu/LED controls,
TRS receiver/routing/disable/restart, sample prepare/upload/play/restart, and
project/preset reload. Host evidence must not be substituted for these checks.

Candidate app: 577208 bytes, SHA-256 `02c0e6251790e8318d842fa3bc65ef6caa7a6d9b37697909daa72d190097524f`; identity `SMK-37_932`. Private package SHA-256 `9009810c80b8c34a2dc840086b6345d5408e1076923c2b6dc5bfc4561cfe24cb`.

### b2 persistence failure reported 2026-10-11

Owner reports the project list empty after power cycling. Persistence acceptance
FAILED; do not treat b2 save/restart as validated. A read-only MIDI check of the
connected device confirmed SLOOP SMK 2.6.0-b2, backup-list status 0 (flash probe
passed), and all four project queries empty. No device write/reset/update was
sent. Evidence: ignored `build/sloop-b2-storage-readonly.json`. The packaged b2
base image has blank project pairs; it is not a current flash dump and does not
prove those sectors remain blank. Exact on-device save message is pending.

Found a separate, concrete reporting bug: native project_save populated RAM
before attempting the flash commit, and ED_PROJECT returned only RAM occupancy.
An unsuccessful save could therefore look used and Studio always said saved.
The local SMK flash path now stages in existing autosave scratch space, commits
and verifies flash before publishing the slot in RAM, retaining the previous
slot on failure. No erase bounds or ownership policy relaxed.

The existing optional ED_PROJECT status tail is present on SMK flash builds:
0 success, 1 transport busy (existing), 2 flash unavailable, 3 write/ownership
failure. Command 9 and the first three bytes are unchanged; older clients ignore
the tail. Updated Studio detects tail presence, rejects nonzero status, and
warns that old three-byte replies do not confirm persistence. Browser-only
sessions retain their RAM/session behavior. This is not a root-cause claim for
the owner's loss: physical save/flash/restart still needs diagnosis.

Native runtime plus fake NOR integration now tests all four actual project
captures/commits/cold reloads (tempo, level and note), failed overwrites, empty
slot failures, foreign preservation, unavailable flash and busy transport. The
real editor handler is exercised for success/failure status bytes. ASan passed.
`web/test_project_save.mjs` checks error reporting, old-reply warning and browser
behavior. Candidate host suite, browser engine and zero-slot connection test
passed; Jieli and Wasm builds passed with existing warnings. Nothing flashed or
published. Public release remains b2; local private candidate rebuilt only.

Follow-up: owner confirmed **SAVE ERROR**, followed by **used** in the slot list.
This reproduces the RAM-before-commit bug. Read-only backup captured the current
project, RAM slots A and B, settings, SYN kits and empty-object records into
`build/sloop-b2-ram-rescue-20261011T180450Z.json` (standard sloop-backup v1).
Every object length/CRC verified; no device write or reset sent. Slot C save
result requested to distinguish per-slot ownership refusal from a general I/O
failure. The actual flash bytes behind A/B have not been read.

Local b3 now reports status 4 / NOT SAVED: SLOT LOCKED for a non-owned project
pair before any write, preserving foreign or incomplete data. Other write errors
show the underlying return code on device. Status 4 is tested through the native
editor handler and browser, alongside status 0..3. Also fixed null elapsed-time
pointers passed to the RAM erase driver in the SMK storage/sample wrappers: its
wait routine unconditionally writes that result. This defect is confirmed in
source, but is not yet proven to be the cause of the owner's SAVE ERROR.
Candidate suite and Jieli build pass; no hardware update or public release.

Owner correction: the successful persistent save was slot 4 (D), not C. It
reported SAVED and remained after power-off/on. Slots 1-3 failed; the owner
specifically notes D was not overwritten. This establishes one successful
physical save/restart, not replacement-save validation. Distinguish foreign-data
refusal from overwrite failure before selecting a fix. Subsequent read-only
queries report all four RAM slots used; this does not establish all persisted
(b2 populates RAM after failed saves). Query evidence:
`build/sloop-b2-slot4-restart-readonly.json`. A full backup attempt timed out;
a fresh D-only backup succeeded with length/CRC verification:
`build/sloop-b2-slot-D-rescue-20261011T181017Z.json`. Earlier current/A/B backup
remains intact. No device write/reset sent. Next discriminating test: save D
again, inspect exact message before restart. D's current contents are preserved
on the PC in standard sloop-backup format (partial archive, object 5 only).

D overwrite acceptance: owner saved D again (SAVED), power-cycled, and reports
only D filled. Physical b2 first-save and replacement-save persistence therefore
PASS for D. A-C remain failing/unresolved; no foreign-data claim established.
Local b3 project list labels non-owned slots LOCKED and no-flash slots NO FLASH.
PROJECT query op 2 returns optional status 4/2 for those conditions, without
altering occupancy or reading/writing foreign payloads. Studio recognizes the
status, labels Protected data / Storage unavailable, and disables saving there;
old three-byte replies retain unknown persistence capability. Existing contents
are preserved; no automatic format or migration added. Native query/label tests,
browser error/legacy compatibility tests and both builds pass. Candidate rebuilt
and staged locally only; hardware b3 validation still pending.

### Physical b3 storage-status confirmation ? 2026-10-11

After the owner installed the local candidate, read-only MIDI INFO confirmed
`SLOOP SMK 2.6.0-b3`. PROJECT query replies were A/B/C: used=0, status=4
(protected/non-owned); D: used=1, status=0. Evidence:
`build/sloop-storage-check-20261011T182802Z.json`.
This confirms the per-slot ownership guard blocks A-C, while D remains readable
through the b2-to-b3 update. It does not identify the protected bytes as old
Felucca records: they could also be incomplete/unrecognized data. Do not erase
or migrate them on this evidence alone. Next storage work requires inspection/
backup of those raw pairs and explicit replacement handling. No device writes,
resets, format or further update were sent during this check. Other b3 hardware
acceptance checks (LEDs, samples and TRS) remain pending.

### Local b4 read-only storage inspection candidate

Prepared after owner authorized inspecting/backing up protected slots. Adds a
separate negotiated `7D SMK` v1 SysEx namespace: discovery plus bounded reads of
only the four A/B project pairs. No erase/program operation exists. Raw captures
are read twice, checked per-chunk CRC and whole-sector equality/SHA, then
classified offline. See [protocol and workflow](SMK_STORAGE_DIAGNOSTICS.md).
CLI: `tools/sloop_smk_storage_diag.py`. Python tests and native diagnostic handler
bounds/CRC/error tests passed; native persistence suite, browser engine/project
regressions and target/browser builds passed (existing warnings). Target RAM and
pool limits unchanged. Host loader simulation changed 142 application sectors,
preserving the entire saved tail. No physical b4 flash or capture performed.

The verified localhost:8774 preview now stages b4, server PID 146780. Public b2
release remains unchanged. Next: owner installs b4, stops playback and leaves
normal USB connected; run read-only capture, inspect A-C and retain the raw
archive before proposing any migration or explicit slot replacement.

b4 app: 577944 bytes; `dbd811b23232bcdc56abbe1f3b551b7d2a9d22e44f9e892796af8dcab68fd491`. Identity `SMK-37_933`; private package `51324999f3f2324d171b64f4d3cc101c57ae2f6cbaea7a08427ef5e1188d5306`.

### b4 physical capture: protected-slot cause confirmed

Owner installed b4. Read-only INFO confirmed `SLOOP SMK 2.6.0-b4`. Diagnostic
capture and independent offline recheck both succeeded, using only INFO,
capability discovery and bounded project-pair reads. Archive:
`build/project-pair-backup-b4-20261011/manifest.json` plus sixteen 4096-byte
sector files (two matching reads of each copy, chunk CRC and sector SHA-256).

- A: both copies are valid Felucca SMK containers.
- B: both copies are valid Felucca SMK containers.
- C: copy 0 is a valid Felucca SMK container; copy 1 is blank.
- D: both copies are valid Sloop storage containers.

This confirms the ownership guard caused A-C's SAVE ERROR: existing Felucca
records were deliberately preserved, not evidence of a general flash failure.
The musical payload has not been converted or interpreted. D's save/overwrite/
restart was already physically verified. All eight raw sectors are now backed
up. No erase, format, project save, reset or firmware update was sent by these
checks. An explicit per-slot replacement/migration action is still needed to
reuse A-C; it must preserve D and require authorization before erasing old data.


### Local b5: explicit backed-up A-C reset

Owner authorized replacing legacy A-C after b4's two-pass backup identified valid
Felucca containers. Added a separate negotiated v2 reset namespace and explicit
backup-checked CLI; v1 remains read-only. Reset only writes an empty Sloop commit
to each selected copy B, preserves copy A until a later normal save, and refuses
slot D. No automatic boot conversion or erase. The tool compares all eight live
sectors with the verified archive, sends mutations once and checks exact readback.

Target and browser builds passed (existing warnings). Candidate host suite,
Python diagnostic/reset tests (10), editor save/capability checks, native browser
engine checks passed. The actual verified backup passes reset/save/restart/load
simulation for A-C with D unchanged. Fault tests cover each partial header size
and erase/program failures. Updater simulation changed 142 application sectors
and preserved the saved tail; installation itself does not reset slots.

Identity: `SLOOP SMK 2.6.0-b5`, product `SMK-37_934`.
App: 579152 bytes, SHA-256 `16fa0220d791741a7bc91f76697b1dfbc756f3fa37bf5573547e64c95235a6bf`.
Private package SHA-256: `0236c3887fc062167dda179fa7a0bc7f1ff84b4b8b5525d7c56200052c052ed4`.
BSS end `0x1c193c8`, pool end `0x1c70af4` (unchanged).

Candidate staged only at localhost:8774; public b2 remains unchanged. No device
writes or firmware flash were performed during preparation. Next: owner manually
installs b5, stops playback/releases notes, leaves normal USB connected. Then
perform the already-authorized reset A-C against the original archive and verify
physical save/restart behavior. Broader LED/sample/TRS acceptance remains pending.


### Physical b5 reset completed (2026-10-11)

Read-only INFO confirmed SLOOP SMK 2.6.0-b5. Ran the owner-authorized A-C reset
with tools/sloop_smk_storage_reset.py against the original verified b4 archive.
All eight live sectors matched before mutation. A/B/C each returned success once;
no retries. Two-pass post-reset capture and exact comparison passed: only their
copy B sectors changed to empty native Sloop records. All three legacy copy A
sectors and both D sectors are byte-for-byte unchanged. Original backups retained.

Private evidence: build/project-reset-b5-20261011/receipt.jsonl, before/ and after/
manifests with sector files, and slot-status.json. Native read-only PROJECT queries
now return A/B/C empty with status 0 (writable), D used with status 0. No project
save/load or automatic firmware flashing performed in this check. Physical A-C
save/power-cycle/reload still needs the owner test; broader LED/sample/TRS checks
remain pending. Public release remains b2.


### b5 slot A power-cycle check (owner report, 2026-10-11)

After being asked to save to A, power off/on and reload A, the owner reported:
"It survived power cycle." Record slot A persistence as physically confirmed by
owner report following the legacy-slot reset. D had already passed save/overwrite/
restart testing and was byte-for-byte preserved by the reset. B and C are confirmed
empty/writable after reset, but their physical save/restart checks remain pending.
This is not a blanket hardware acceptance or public-release promotion of b5.
Next storage check: save distinguishable projects to B and C, perform one power
cycle, and load each. Keep A and D intact. Pad-light behavior, physical sample
uploads/persistence and the updated TRS routing still need candidate acceptance.


### All four project slots: persistence checks complete (2026-10-11)

Owner reported "works" after being asked to save distinct projects into B and C,
power-cycle once and reload each. Record B/C save/restart/reload as physically
confirmed by owner report on b5. Combined evidence: A passed on b5; B/C now pass
on b5; D previously passed save/overwrite/restart and its raw sectors were verified
unchanged by the A-C reset. The legacy ownership conflict is resolved on this unit.
Original Felucca raw backups remain preserved locally. This does not demonstrate
automatic migration for other users: protected legacy slots still require explicit
backup/replacement handling. No new device writes or release promotion accompanied
this report. Next candidate acceptance: pad feedback, sample upload/persistence,
and updated TRS routing. Public firmware remains b2.


### b5 pad feedback follow-up (2026-10-11)

Owner reported no blue response on pad presses; green flashes mostly absent but
one stray green flash occurred on a press. Read-only INFO/BK_LIST/BK_GET inspection
confirmed SLOOP SMK 2.6.0-b5, current settings object length 88 and matching CRC32
825325253. Its lights word is 0: PAD BRIGHTNESS OFF. Evidence retained privately
in build/b5-pad-settings-readonly.json. No settings write or firmware change was
made. OFF explains disabled blue feedback; it does not explain the stray green.
Pad-light hardware acceptance remains open. Next: set brightness MID, ensure the
light diagnostic is PLAY, then test individual taps with transport stopped.


### Owner defers sample check; resume TRS validation (2026-10-11)

Owner selected HIGH pad brightness and later reported no green seen for a while.
No definitive green-flash fix is claimed. Added the intermittent-flash watch and
physical sample upload/play/power-cycle check to docs/WORK_QUEUE.md at the owner's
request. Sample initialization/uploads were not performed. Next active hardware
check is b5 TRS output, including receiver note cleanup on disabling output and
saved TRS preference after a power cycle.


### b5 TRS receiver test and correction (2026-10-11)

Owner confirmed notes/chords reach the FM-1. On the held-note/TRS-OFF test the
initial "yes" was immediately corrected to "I mean no": the receiver note stays
on. Record disable cleanup as FAILED, not passed. No physical preference-restart
check completed yet. Code review found shutdown sends CC64/120/123 on all channels
but no individual Note Off messages. Receiver handling of these CCs has not been
captured, so it remains a hypothesis explaining this particular hardware symptom.

Local b6 adds transmitted-note tracking and explicit Note Off on original channels
after the existing controller reset sequence, before closing UART. It also covers
queue-overflow recovery, finishes partial messages, excludes unsent queued notes,
and recognizes velocity-zero releases. No change to the public release yet.


### Local b6 TRS held-note shutdown candidate

Host queue tests cover explicit offs after channel-mode resets, original channels,
velocity-zero releases, dropped unsent events, interrupted messages and repeated
recovery. UART test confirms the explicit off is transmitted before stopping DMA.
Full candidate host suite and browser engine/save/capability tests pass. Hardware
and browser builds pass with existing warnings. Application 579360 bytes, BSS end
0x1c194c8, pool end 0x1c70af4. Application SHA-256:
15a73fa4a17e2cf8c213cde8135f42dda48628466a0940653662ee238427ee60.
Private package SHA-256:
76051b37624fc47527331ca577c9f14a0abb524ad40045d3c42d9c50a1aabe80.
Identity SLOOP SMK 2.6.0-b6 / SMK-37_935. Loader simulation changes 142 application
sectors and preserves stored data. Localhost:8774 stages b6; public b2 unchanged.
No device flash performed. FM-1 receiver held-note/TRS-OFF retest pending. This
fix assumes the receiver receives normal Note Off; it cannot clean up through an
unplugged cable or a failed UART. Sample testing remains deferred in WORK_QUEUE.


### b6 physical TRS-OFF retest passed (2026-10-11)

After being asked to install b6 and repeat holding a note on the FM-1 receiver
while turning SMK TRS MIDI OUT OFF, the owner reported "worked". Record this
held-note shutdown scenario as physically passed by owner report. b5's failed
result remains in the history; b6 adds explicit per-note releases before UART
shutdown. Notes/chords had already passed on b5. This does not prove every
receiver or sustained-chord/sequence shutdown scenario, nor disconnected-cable
cleanup. TRS ON preference across a power cycle remains to be checked. Sample
upload/persistence remains deferred; public release is still b2.


### b6 physical TRS preference persistence passed (2026-10-11)

Owner answered "yes" after being asked to turn TRS MIDI OUT ON, exit with OCT-
to save, power-cycle and confirm both ON and FM-1 reception. Record preference
persistence and post-restart receiver playback as passed by owner report on b6.
Combined TRS evidence: notes/chords pass, held-note cleanup when disabling output
passes, saved ON state and reception after restart pass. This does not change
the deferred sample test or establish all expression/transport scenarios.
No firmware publication, device writes or automatic flash accompanied this report.


### b6 expression follow-up and scope correction (2026-10-11)

Owner removed public legacy-upgrade work from scope: no other units need that
transition. Existing protection/backup handling remains. Owner reports sustain
and pitch work locally on the SMK but do not affect the TRS-connected FM-1;
mod remains inconclusive. Notes/chords, TRS-OFF held-note cleanup and preference
persistence remain separately passed. Investigate expression generation/output
and FM-1 reception before proposing another firmware update.


### Controlled expression capture (2026-10-11)

Receiver identified by owner as Sloop on FM-1 (exact installed version pending).
The first capture was NOT a valid exercise test: owner did not know it had begun.
After explicit "Sloop and ready", captured 30 seconds passively from Felucca 0:
17 note-ons + 17 note-offs on channel 1; CC64 12 events per channel 1/2/3 spanning
0 and 127; CC1 channel 1 spans 0..127; pitch channel 1 spans -8192..8114 (811
messages). Other channels also have expression during pedal/held-track routing.
Private raw evidence: build/b6-expression-usb-controlled-20261011.json.
This confirms physical controls generate MIDI on USB, NOT a measurement of the
TRS electrical output or proof that FM-1 receives those bytes. Both outputs share
the MIDI event producer; TRS transport is still a separate path.

Older local FM-1 Sloop checkout E:/AI_Apps/Sloop/sloop-fm1 at bb30fb1 documents
mod/sustain ignored (README MIDI CC section) and its receiver filters out pitch
status 0xE0. The local sloop-pages release metadata says 2.4.1 but does not prove
which firmware is on the receiver. Current SMK imported 2.6 source adds pedal and
bend handling. Confirm FM-1 installed version before claiming receiver limitation
as the actual cause or making more transmitter changes. No flash was performed.


### TRS sustain/pitch passed after receiver update (2026-10-11)

Owner identified the receiving FM-1 as Sloop 2.5, updated that FM-1 to 2.6, then
answered "yes" to a test of pedal sustain/release and pitch wheel in both
directions with SMK TRS MIDI OUT ON. Record physical sustain and pitch reception
as passed by owner report: Sloop SMK b6 -> FM-1 Sloop 2.6. No transmitter firmware
change was required for this resolution. The 2.5 receiver limitation explains
the previous missing sustain/pitch behavior. Mod remains audibly unconfirmed;
USB capture confirms CC1 generation but is not TRS receiver evidence.

Combined physical acceptance now includes all project slots' persistence, TRS
notes/chords, held-note cleanup on disabling output, TRS ON persistence after
restart, and sustain/pitch on a compatible receiver. Sample testing remains
deferred; user preset persistence and newer-candidate long USB audio validation
remain pending. Public release remains b2; b6 stays the local candidate.


### b6 user-preset test prepared (2026-10-11)

Owner authorized user-preset save/restart testing. Read-only inventory found all
32 slots empty, selected synth track 1. Made a CRC-verified native backup of
objects 0..7 (current project, settings, project slots, both preset banks) in
build/b6-user-preset-check-20261011/before-backup.json. Existing project occupancy
at capture was A/B/D used and C empty; this does not overwrite the owner's earlier
reported C save/restart pass or explain a subsequent change. No project writes
were sent during this test.

Initial pre-save guard stopped without writing because track 1 level changed
by one raw step between reads. Kept the original backup and captured a fresh
current-sound snapshot immediately before saving. ED_UP_STORE saved current
track 1 to previously empty U01 as B6 PRESET, reply [0,0]. ED_UP_GET returned the
same engine and all sound parameters as the immediate pre-save snapshot. Other
sound parameters stayed unchanged; level was allowed to vary on the subsequent
live read. Exact expected preset reply and save receipt are stored in the same
private directory. No existing user presets were overwritten; no failed write
was retried. Power-cycle comparison and actual preset load remain pending.


### b6 user preset restart/load passed (2026-10-11)

After the owner power-cycled and reported ready, INFO confirmed SLOOP SMK 2.6.0-b6.
ED_UP_GET for U01 B6 PRESET exactly matched the full pre-restart reply (name,
engine, all stored parameter and pattern bytes). Sent one ED_UP_LOAD for U01:
reply [0,0]. Track 1 engine and all 71 parameters matched the preset, except
intentional param_kept mix/pattern/key fields which matched the pre-load track.
The verification allows one step of previously observed level jitter. No project
save/erase or preset rewrite was sent. U01 remains available for audition.

Private evidence: build/b6-user-preset-check-20261011/restart-receipt.json and
restart-result.json; pre-load track snapshot also retained. The initial local
comparison script omitted underscore-containing parameter names; corrected its
parser and verified the already-recorded replies offline, without repeating the
load. This was a host verification-script issue, not a device/preset mismatch.

User preset U01 save/cold-restart/readback/load is now physically verified by MIDI
readback. Audible recall and every other preset slot/bank were not separately
tested; do not claim exhaustive bank coverage. Public remains b2, local b6.


### Do not repeat passed audio checks without a regression (2026-10-11)

Owner pointed out repeated audio testing after another 44.1 kHz capture began.
Stopped this duplicate recorder; do not start the proposed 48 kHz repetition.
The retained completed evidence is the prior b2 two-minute stereo recordings at
44.1/48 kHz, signal throughout and no clipping. This interrupted attempt is not a
b6 audio pass. There is no currently identified audio regression warranting another
user-run sequence test. Do not block this round on repeating that evidence; run
new audio tests only for a concrete audio-path change or reported failure. Keep
prior evidence's version and Windows resampling limitations explicit.
