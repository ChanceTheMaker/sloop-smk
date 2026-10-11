# Read-only project storage diagnostics

Local Sloop SMK b4 adds inspection of the physical project pairs. It does not
initialize, erase, migrate or restore data. Flash ownership checks remain intact.
This diagnoses b2/b3 slots A-C reporting save errors/protected data, while D
successfully saved, overwrote and survived restart on the owner's SMK.

## Capture

Install the local b4 candidate manually, stop playback, and close Studio/other
MIDI clients. Use the exact MIDI port names reported by the host. With `mido`
and `python-rtmidi` available:

```powershell
python tools/sloop_smk_storage_diag.py capture --input "Felucca 0" --output "Felucca 1" --directory build/project-pair-backup
python tools/sloop_smk_storage_diag.py analyze build/project-pair-backup
```

The destination must not exist. The tool reads INFO and negotiates the diagnostic
capability before capturing. It reads each 4096-byte sector twice, checks each
chunk CRC and the matching sector copies, and records SHA-256 hashes. A failure
leaves partial files but no verified manifest. These private artifacts belong in
ignored `build/`, never in a public release. The archive is raw project-pair data,
not a Studio project backup; the earlier standard RAM backups remain separate.

Classification distinguishes blank sectors, valid Felucca SMK containers, valid
Sloop storage containers, recognizable but invalid containers, and unknown data.
It checks container/header/payload CRCs, IDs and bounds; it does not assert the
musical payload is compatible with the running firmware. Neither classification
nor capture authorizes overwriting existing records.

## Wire format v1

Separate experimental/noncommercial SysEx namespace, without reusing any native
Felucca/Sloop editor command ID. Bytes shown exclude F0/F7 framing:

`7D 53 4D 4B 01 op args...`

Replies use the same prefix/version/op followed by status and data. Clients must
receive the exact discovery capability before attempting reads. All multi-byte
integers below are low-seven-bits first. Version/op/length/bounds are checked;
unknown operations have no write interpretation.

- Op 0: no arguments. Reply: `status, capabilities, slots, copies,
  sector_size[2], max_chunk[2]`. Success is exactly `0,1,4,2,0,32,0,2`.
  Capability bit 0 means read-only project-pair inspection.
- Op 1: `slot, copy, offset[2], count[2]`. Slot 0-3; copy 0-1; offset within
  4096 bytes; count 1-256 entirely within that sector. Success reply:
  `0, echoed_args[6], CRC32[5], pack7(data)`. The address is computed internally
  from the existing SMK project map; clients cannot supply a flash address.
- Errors: status 1 malformed/unsupported; 2 flash probe unavailable; 3 playback
  or transport request active; 4 read failure. Error replies contain no data.
- `pack7`: one high-bit mask followed by up to seven low-seven-bit data bytes.

Only project pairs at 0xEB000 through 0xF2FFF can be read. Firmware, loader,
calibration, samples, presets and settings are outside this interface. The
handler shares the editor's main-loop reply buffer and allocates only a 256-byte
read buffer on the stack. It contains no erase/program/update dispatch.

## Validation and limits

- Native handler tests: capability negotiation, data and CRC round trip, slot/
  copy/offset/count bounds, unsupported operations/versions, busy/unavailable
  refusals, and unchanged fake flash.
- Python tests: two-pass capture and offline recheck; damaged data, changed
  reads, unsupported capability, invalid headers, and write-command refusal.
- Target b4 and browser engine compile; host updater simulation preserves the
  saved tail. Existing persistence, browser project and engine tests pass.
- Physical b4 capture succeeded on 2026-10-11: A/B have two valid Felucca SMK
  copies each; C has one valid Felucca SMK copy and one blank; D has two valid
  Sloop storage copies. Two matching reads and offline recheck passed. The
  ownership guard is preserving older Felucca data. No device writes sent.


## Explicit legacy replacement v2 (local b5 candidate)

Owner authorized resetting A-C after the verified b4 capture. This is a separate
mutating interface; v1 and its capture tool remain read-only. It is never called
at startup or automatically by the editor. Slot D is excluded in firmware and
in the tool. It does not convert legacy musical payloads.

Wire prefix: `7D 53 4D 4B 02 op args...` (F0/F7 omitted).

- Op 0 has no arguments. Exact success reply after prefix/op: `0,1,3`
  (status, explicit-reset capability, eligible slots A-C).
- Op 1 arguments: `slot, copy_A_CRC32[5], copy_B_CRC32[5], ASCII("RESET")`.
  Integers use low-seven-bits first, highest CRC byte <=15. Exact request length
  including namespace/op is 22 bytes. Reply: `status, slot`.
- Status: 0 success; 1 malformed/out of range; 2 flash unavailable; 3 transport
  or sounding voices active; 4 read error; 5 changed backup checksum;
  6 missing valid legacy copy A or existing nonempty Sloop record; 7 flash error.

The handler checks the live full-sector checksums and legacy header/payload CRC.
It erases **only copy B** and commits an empty native Sloop record there, then
verifies it. Copy A remains intact until a later ordinary Sloop save reuses it.
The temporary flash-write exception covers exactly that B sector and is cleared
on success or failure. The normal ownership guard remains in effect elsewhere.
A completed tombstone can be recognized without rewriting after a lost reply.
An interrupted write leaves legacy copy A; it does not trigger a boot erase or
blind retry. Inspect and re-back-up before any further repair in that case.

`tools/sloop_smk_storage_reset.py` requires the verified two-pass archive, exact
MIDI port names, selected A-C slots and an explicit confirmation flag. It checks
all eight live sectors against the archive before sending any reset, negotiates
v2, sends each mutation once, records a receipt, and verifies all sectors again.
The postcondition allows only the selected B sectors to become exact tombstones.
It does not flash firmware. The b5 identity check keeps this repair tied to the
candidate tested here. Example for the authorized local repair after installation:

```powershell
python tools/sloop_smk_storage_reset.py --backup build/project-pair-backup-b4-20261011 --directory build/project-reset-b5-20261011 --input "Felucca 0" --output "Felucca 1" --slots A B C --confirm-replace-felucca-projects
```

Host evidence: reset refusal/bounds/checksum/legacy validation, preservation of D
and all other sectors, failed erase/program, every incomplete 32-byte header,
reboot ownership, subsequent save/reload and lost-ack idempotence pass. The actual
private archive was also exercised in fake NOR: reset A-C, preserve D, save new
projects to A-C and cold reload all three. Python tests cover archive tampering,
changed live sectors, mutation errors/timeouts without retry, and exact post-reset
comparison. Physical b5 installation and reset remain pending; public b2 unchanged.


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
