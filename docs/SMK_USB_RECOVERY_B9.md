# Sloop SMK USB updater repair

Selected public release 2.6.0-b9-dc2, runtime `SMK-37_938`, prepared and tested on the
owner's SMK-37 Pro on October 12, 2026 (October 13 UTC). Publication authorized after the Pro checks.
Elite and the serial comparison build are not hardware validated.

## Failure and evidence

The Pro booted Sloop b8-dc1 but attempts to install other firmware stopped in
`ota-SMK-37_937`. The supplied browser log targeted Felucca SMK 0.9.23 Beta.
A power cycle returned to `SMK-37_937`, but did not repair the updater.

Two fresh independent 1 MiB recovery reads matched. Their decoded application
exactly matched published b8-dc1, the boot region matched profile CRC32
`3a04ad23`, and the whole 4 KiB USRTRIM sector was erased. The old public loader
unconditionally required a valid populated calibration record before processing
the application package. Its rejection therefore affected every destination.
The cause of the existing blank calibration sector is not established.

## Change

Port the focused fix from Doom `da621ca`: accept either valid key calibration or
a wholly erased 4 KiB sector; reject invalid nonblank/partially erased records.
Snapshot and verify the entire sector across the update. No calibration defaults
are written and write bounds remain `0x4000..0x9d000`. Preserve the synthesis,
dual-core renderer, storage layout, controls and transport commands of b8-dc1.

Base: Sloop SMK `203c304`. Musical upstream remains
`4365a1d0075b9194de1c44826325abbd39751bc1` (2.6). Existing credits and licenses
are retained; no upstream submission was made. The b9-dc2 app is selected for publication.

Application: 580,804 bytes (760 bytes below the conservative limit).
SHA-256: `aa9ae1651fc6ae206ecfe0c0a860085663d7b11f25a712b147b8a40c674a31f7`.
RAM loader: 8,228 bytes. Existing 8 KiB CPU1 stack retained.

## Validation

- Public/private updater tests passed with address/undefined-behavior sanitizers,
  including blank-sector acceptance, partial corruption rejection, full-sector
  preservation, package corruption, interrupted programming and write bounds.
- Actual fresh backup plus candidate package passed the host loader simulation:
  56 changed application sectors, all protected bytes preserved.
- Target compilation and SRAM/dual-core audit passed. Dual-core lifecycle and
  serial/concurrent PCM comparison passed. Storage, samples, routing, persistence,
  reset, TRS/MIDI, pad and lighting regression suites passed.
- Recovery installation re-read the device immediately before writing, verified
  each of 56 sectors, then verified the full 1 MiB image. Bootloader and every
  byte at/above `0x9d000` remained identical, including calibration and saved data.
- Owner reported Sloop booted. Direct USB returned `SMK-37_938`.
- A normal-USB reinstall through `ota-SMK-37_938` completed all 2,450 package
  requests, loader readback and restart to `SMK-37_938`. This was a same-version
  reinstall; unchanged sectors do not exercise new USB erase/program operations.
  The first host attempt stopped during Windows port reconnection before package
  delivery; after making port-close tolerant of removal, loader resume succeeded.
- Post-USB telemetry showed CPU1 online with zero faults, timeouts, rejected
  requests or warm-reset bar. Those idle samples contained zero worker jobs and
  are not an audio workload measurement. Owner then confirmed working keys,
  knobs and sound after the normal-USB reinstall.
- The owner subsequently reported "Sloop to Doom worked SMK Pro". A read-only
  follow-up query returned `ota-Doom-SMK-37_015`; Doom's Release 15 updater was
  active at that time. No update or reset command was sent in that check. This
  adds owner-reported cross-firmware success, separate from the controlled
  same-version reinstall above. The owner then reported "Doom to Felucca SMK Pro
  worked", explaining the active updater and confirming an onward USB transition.
  Felucca's returned version and its own outgoing update behavior were not tested
  in this follow-up.

Private backups, generated local package and write journal stay in ignored
`build/private-recovery/20261013T014052Z`. No old full-device image was used.
The recovery helper has no automatic reboot or write retry. Public b8-dc1 files
remain immutable; they still contain the old rejection and must not be described
as having this fix. Reinstalling old firmware may reintroduce the updater issue.
