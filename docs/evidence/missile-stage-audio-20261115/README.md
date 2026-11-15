# Missile stage audio policy

Fresh main files fetched via GitHub, blob identities in SOURCE.json. No checkout-wide build claimed.

## Evidence / defect
All eleven phonk_missile aliases already exist, with authored distance/volume. Live source triggers launch, cruise ignition, flyby, sonic transition and impact. Sonic currently shares missile-impact.wav; no distinct sonic asset verified. Cruise is an ignition one-shot, not continuous attached motor loop. Current SpatialPolicy gives missiles linear distance falloff, but IsClusteredImpactAlias excludes every missile alias, bypassing repeated-nearby-event suppression and per-alias concurrency/mix attenuation. Multiple ships' per-ship cooldowns do not bound the aggregate sound service.

## Candidate
Extend existing sound-service owner instead of duplicating per-ship logic. All missile launches/interceptors share one launch key; keep cruise, flyby, sonic and impact independently audible. For current 11 aliases: maximum stage counts 6/4/3/4/8, respectively; existing local limit 2 and 50ms/20m repeat suppression. Current known missile one-shots thus have at most 25 ordinary active one-shot voices per sound type (not an absolute engine-wide cap; pause/resume and duplication need runtime QA) (plus separate existing hull impact layers). Existing spatial distance culling remains. New audio-only deterministic hashed counter changes gain within 0.90–1.00 of authored gain, never increases volume, and never consumes gameplay RNG. No pitch randomization or global pitch alteration.

## Verification
C++20 policy probe with -Wall -Wextra -Werror and UBSan PASS: all 11 real aliases, five stage groups, 25 known-voice bound, separate sonic/impact budgets, legacy splash mapping, legacy loop/charge exclusion, 100000 bounded deterministic gain inputs. Existing service code uses returned key for admission and rebalance.
ASan/LSan runner failed in this environment (LeakSanitizer does not work under ptrace), not counted as pass. Full miniaudio service build and runtime listening not performed: macOS Metal/audio input libraries and device are not available here. Existing service replay requires managed macOS build graph and installed application, which this task forbids using.

## Assets / scope
No assets downloaded or replaced, no new third-party license introduced. Existing generator tools/prepare_phonk_missile_audio.py synthesizes five base effects without external recordings. Extra named recordings exist in repository paths but their complete provenance was not audited and they were not changed. Do not claim sonic is newly distinct or all flight stages have a continuous sound. Listening/long-range/slowmo/dense salvos and focus/pause acceptance remain OPEN.

## Integration
Only audio/audio_voice_policy.h and audio/sound_service.cpp (both under experiments/native-metal) change. No ordnance/VFX/gameplay-script changes, so tandem worker can proceed independently. Two *.patch files are relative to captured base files. No installed app changes or runtime listening acceptance.

## Run
From repository root:
```sh
c++ -std=c++20 -Wall -Wextra -Werror -fsanitize=undefined -Iexperiments/native-metal/audio docs/evidence/missile-stage-audio-20261115/probe.cpp -o /tmp/missile-audio-probe && /tmp/missile-audio-probe
```
Independent source review GO; reviewer reran UBSan probe. Main root tree has no .github directory/workflows. Source integration is not an installed application rebuild.
