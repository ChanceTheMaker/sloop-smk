# SMK flash compatibility update

Release: `sloop-smk-2.6.0-b10-dc2`; runtime: `SMK-37_949`.
Source baseline: `af029d1d591dfc122682a7896b80298efcbddcf2`. No musical/emulator upstream update is included.

The SMK updater previously rejected every flash ID except `856014`. This release
accepts exactly `856014` and `EB6014`, matching the focused Felucca 0.9.28 fix.
Unknown IDs remain rejected before requesting a package or erasing application
sectors. Sloop and Melodee storage probes use the same policy, while retaining
their valid-calibration and storage-ownership checks.

Credit: andyddd's measured chip ID and recovery report in
[Felucca issue #17](https://github.com/ChanceTheMaker/felucca-smk/issues/17#issuecomment-6249932641).
The reporter's ROM-based recovery provides compatibility evidence; it does not
constitute hardware validation of this project's SPI driver on EB6014.

No SPI commands, timing, calibration policy, storage layout or writable ranges
changed. Existing boot/calibration protections and complete package preflight,
sector verification and application readback remain in place. FM-1 firmware
artifacts are unchanged. The dual-core engines and existing features are retained.

## Failure reporting and existing installations

The loader reports checksummed `SMKU` version-1 diagnostics using OTA command
`7E`: error code, whether an erase/program attempt started, and observed flash ID.
Install and restore transports, including the Windows USB helper, display the
failure immediately. Older hosts ignore this extra message. Users must update
older Windows helper checkouts to this release before using its installer.

A website update cannot replace an already-installed loader that rejects its
flash chip before accepting firmware. Such units still need a separately
validated recovery route. The contributor's experimental ZIP is limited to
Felucca 0.9.27 / SMK Pro / EB6014; it is not authorized or validated as a generic
recovery tool for this firmware. No recovery ZIP is executed or redistributed.
Issue #12 has not been established as the same cause. Software recovery cannot
be guaranteed if the application and ROM-entry route are unavailable.

## Artifact and validation

SMK Pro application: 580996 bytes.
SHA-256: `d091c3b34fd30d847997bdb1c41670ba43236639a9d1a0a66f69818bf8a1074f`.

Private/public host updater suites with address/undefined-behavior sanitizers
pass, including EB6014 acceptance and unknown-ID rejection before erase.
Policy tests reject unrelated same-capacity chips. Browser tests exercise the
actual install/restore transport loops, and Python checks validate diagnostic
checksums and error wording. Firmware builds pass their memory/loader audits.

SMK Pro: installed and restarted as SMK-37_949, then restored original Pro 016 with successful transfer/readback completion and exact restart identity. EB6014 and Elite hardware confirmation remain pending; UI, sound and long-run validation are not claimed. EB6014 and
Elite physical validation, audio/gameplay and long-duration checks remain
unconfirmed for this release. Historical functional testing belongs to the
older releases described in the existing project notes.

This is an owner-authorized fork release. No upstream PR or issue reply is sent.
