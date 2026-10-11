# Felucca Salt work queue

Deferred requests for a future round. Listing an item here does not add it to
the current implementation or publish.

## Website

- [x] **Dedicated SMK simulator page (local, 2026-10-11).**
  `web/simulator.html` reuses the native browser engine and simulator controls,
  with a top-navigation link from the homepage and Studio. Includes audio,
  session import/export, shared browser storage, finish swatches, tutorial and
  fullscreen. Normal navigation between the running browser Studio and the
  simulator saves the session before leaving. Existing embedded simulator mode
  remains available. Preview: `http://localhost:8773/simulator.html`.
  Native runtime remains based on the SMK branch at `4831471`, with upstream
  Felucca 1.5.1 pinned at `a4d7ca2e5d52a4c35e2468401f00d883cadbda03`.
  Web protocol regression suite and JavaScript syntax checks pass; local Chrome
  startup, display and navigation checked. No device firmware or publication
  changed. New simulator-specific labels currently remain in English.

- [x] **Make the header logo link to the homepage.** Implemented locally after
  the owner brought it into scope on 2026-10-05. The “Felucca [Salt]” logo on both
  pages links to the installer homepage, with its size/styling preserved and
  visible keyboard focus. The owner authorized merging into the fork's main;
  live deployment is separate.

## SMK firmware

- [ ] **Investigate browser-audio regression retry.** The commit-time run against
  localhost:8771 ended with a timeout in its error-path Playwright screenshot,
  masking the original error. Recover the original failure and rerun; do not
  infer a browser-audio pass or a specific product defect from this result.

- [ ] **TRS OFF leaves a held FM-1 note sounding (Test21).** On 2026-10-10 the
  owner confirmed that switching TRS OUT OFF with the cable connected does not
  stop a held receiver note. Chords, sustain and pitch otherwise worked; mod
  remains inconclusive. Root cause unknown. The owner explicitly deferred repair
  to continue sample/storage testing. No shutdown fix is included. See
  [Test21 hardware results](smk37/TEST21.md#hardware-follow-up).

- [ ] **Review saving TRS OUT across power cycles.** The owner confirmed Test21
  restarts with TRS OFF and requested recording this for later. This is the
  current intentional startup behavior, not evidence of a failed settings write.
  Decide and implement the desired persistence in a later round.

- [ ] **Finish user-sample persistence validation.** USR1 test upload and playback
  were reported successful, but the post-restart check was inconclusive while
  testing the FM-1 receiver. Reconnect Studio to inspect USR1, select SAMPLE / USR1
  and listen directly through the SMK headphone output after a power cycle.
  TRS sends MIDI events, not the uploaded sample audio. Do not mark persistence
  passed or sample data lost without that check. Deferred at the owner's request.


## Sloop SMK candidate follow-up

- [ ] **Physical sample upload and power-cycle persistence (b5).** Deferred by
  the owner on 2026-10-11 after project persistence checks. Use the Sloop Studio
  and an available user-sample slot; verify upload, direct headphone playback,
  then playback after power off/on. If sample storage needs preparation, review
  its replacement warning before proceeding; do not initialize it implicitly.
  This is separate from the earlier Felucca Test21 sample result above.
- [ ] **Watch for intermittent stray green pad flashes (b5).** Owner selected
  HIGH brightness and reported not seeing green for a while. The missing blue
  had coincided with a read-only-confirmed OFF setting. Do not call the rare
  green flash conclusively fixed; retain the individual-pad/color diagnostic
  if it recurs. An unambiguous blue-on-touched-pad confirmation remains useful.

- [x] **Sloop b6 TRS-OFF receiver retest.** Owner confirmed "worked" after
  installing b6 and repeating the held-note/TRS-OFF test with the FM-1 receiver.
  Individual Note Off messages now accompany sustain/channel resets. Physical
  pass applies to this scenario; notes/chords previously passed on b5.
- [x] **Sloop TRS ON preference across a power cycle.** Owner confirmed on
  2026-10-11 that after turning TRS ON, exiting to save and powering off/on,
  the setting remained ON and notes still reached the FM-1 on b6.
  Separate from the earlier Felucca Test21 persistence report above.

- [x] **Sloop b6 TRS sustain/pitch receiver compatibility.** Owner identified
  FM-1 receiver as Sloop 2.5, upgraded it to 2.6, then confirmed both sustain
  (including pedal-up release) and bidirectional pitch bend work over TRS.
  No further SMK change was needed. USB capture had independently confirmed
  expression generation. The earlier receiver lacked these controls.
- [ ] **TRS mod-wheel audibility.** USB CC1 output spans 0..127, but audible
  modulation on the FM-1 receiver has not been confirmed. Use a receiver patch
  with a known mod-wheel response for any future physical check.

Public legacy-upgrade/migration workflow is excluded from this round at the
owner's request (2026-10-11): no other users need that transition. Retain the
existing backup/ownership protections and private repair evidence.


- [x] **Sloop b6 user preset save/restart/load smoke test.** U01 B6 PRESET was
  saved into an empty slot after backup. Following the owner's power cycle,
  its full preset payload matched exactly and loading it on track 1 succeeded;
  all 71 parameters checked with intentional mix/pattern preservation. Test
  preset remains in U01. Other slots/banks were not exhaustively exercised.


Validation policy for this round (owner feedback, 2026-10-11): prior completed
44.1/48 kHz audio tests stand with their recorded b2 version and limitations.
Do not repeat them as a routine b6 acceptance task without a concrete regression
or relevant new audio-path change. Duplicate b6 capture stopped; no new pass claimed.

- [x] **Physical b7 oscilloscope check.** Local candidate removes full-screen
  pre-redraw black clears and doubles scope scheduling to every 40 ms UI tick.
  Host regression/builds passed; owner reported "Looks good" and authorized publication.
