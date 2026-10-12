# Elite resume test r1

Published test entry: https://chancethemaker.github.io/sloop-smk/midi-diagnostics.html
Firmware-writing test: https://chancethemaker.github.io/sloop-smk/elite-resume.html?public#install

The MIDI diagnostics and stage-one-only handoff capture are adapted from Felucca SMK's diag/elite-handoff-capture commit 541b7e6. Their modules are isolated under web/elite-test; they do not change the normal installer.

The resume installer uses the community-supplied instalador sloop smk37.zip resume approach (archive SHA256 604971ecfca3f5a3277c5199e3e0a28a337b46e180351cd233ef72a00cfd66c2), with unrelated-identity filtering and an Elite-only generic-loader option. Exact model loaders resume directly. A generic ota-FM-1 loader requires a default-off checkbox and a second explicit confirmation. Users must disconnect every real FM-1. This is a user-identified experimental exception, not proof of model from the generic identity. The option cannot be used for Pro, and cancellation sends no programming START.

Firmware remains the existing Sloop SMK 2.6.0-b7 public application, identity SMK-37_936, SHA256 aafc79a98733a923e77afc51afa2178671b636914f70292b857d19f011678c19. No firmware binary, normal installer module or private-device data changed. No device access or flashing occurred in preparing this change.

Validation: identity-only diagnostic tests; handoff allowlist/single-START/bounds/abort/disconnect/port-cleanup tests; resume tests for exact loader, normal stock, missing loader, generic option off/cancelled/approved, Pro refusal, unrelated-device refusal, write failure and wrong returned identity. Actual b7 Pro/Elite packages validated offline in the isolated trial. Physical Elite resume remains for djStrombo to test and return the full log.
