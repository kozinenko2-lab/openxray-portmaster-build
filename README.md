# AirXonix r371 — Android + H700

Latest source: [AirXonix-H700-Android-r371-public-source.zip](AirXonix-H700-Android-r371-public-source.zip). Public source has no proprietary game EXE or MUSIC; build via GitHub Actions with clean-room assets. H700 PortMaster code is retained.

r371 changes: fixes short Android touchscreen A/B taps where legacyPressedCode was set but InputState.action/back were false. Single touch adapter now defers input during animated menu selector and difficulty fade, serializes directional and Enter/Escape pulses, and prevents a held A from confirming two screens. Physical controller and H700 path preserved.

Includes a full native regression test exercising MainMenu DOWN, UP during slide, A, ModeSelect DOWN and UP, B cancel; keeps earlier unit tests. Icon remains the user's selected AIR XONIX artwork.

This branch is isolated from the OpenXRay main branch.
