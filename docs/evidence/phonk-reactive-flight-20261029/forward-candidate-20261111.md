# Forward candidate, 2026-11-11

Two scratch-only complete builds used `build.sh`, the bundled CMake/Ninja and
the already available Jolt 5.6.0 source. Each build verified 8243 portable
inputs and built both `engine` and `sea-fleet-benchmark`. The independent
snapshots had 1156 fingerprinted engine inputs; 51 differed from the Git index
or were not tracked. They are mixed inherited working state, not a reviewed
release batch. Both per-file manifests include Git index blob IDs and Jolt
source hashes. The file sets and Jolt/input manifests matched; exactly one
engine input differed between the two builds: `ship_reactive_jet.cpp`.

* Before: source fingerprint `d4b8956febc0b86632135b533c848318b5e9d68c6c39c8675b200128f961966e`,
  engine `c8b1590cdbc2abacc5bdc086af74869faa55eda7f2dd49957f79ebf56b8842a9`.
* After: source fingerprint `0240cf34110d13484a4bd8410aad53b6279ad178ec012ed4e43af5c323a4c0ca`,
  engine `bd0c636041ac515fc50570af51b35132a1e78a486a9f736fd7ec7802138accf8`,
  native benchmark `a15fb6b55f12fa20ca2910d27dda741dcdd0a3985d7ec2a58ad65e3f4246c78c`.

The precise six-line working-source delta is in
`forward-candidate-modeoff-fix.patch`: a previously active physical flight
continues when Phonk mode is disabled. Before the delta the native fixture
failed `modeoff_preserves_COM_ballistic`: active became 0 and horizontal
speed became zero. After it, the fixture preserved active flight and momentum,
passed its in-memory SHIP save/load continuation and Nevis contact checks, and
completed `NATIVE_PASS`. This is not a replay of the user's old save.

The second build's real `AIBalls`/`ISLAND`/`AIFort` fixture registered one
island, one fort and one water hit, an active miss and no unexpected ship hit.
The independent Brig1/Nevis solver replay passed ordinary-sea, single-move and
terrain-response checks. The candidate-linked harpoon fixture hit the intended
ship, shortened cable 427.93173 to 427.45172, released it, and showed a missed
projectile expire. These are bounded component/benchmark scenarios, not player
acceptance. The adjusted codec fixture excludes its prior zero-weight admission
case because that case's semantics were not established; it does not mask the
mode-off or save/load assertions.

The after fixture reports a mean 8.206 ms over 180 Nevis flight steps; its
96/48/16 ms recovery steps cost 33.006/16.838/5.558 ms CPU respectively.
Those numbers are not game FPS or a before/after performance comparison.

`forward-candidate-manifest-20261111.json` retains the before result and
`forward-candidate-fixed-manifest-20261111.json` the after manifest and complete
probe output. The corresponding producer files and
`forward-candidate-replay-20261111.py` retain the procedure. After the recorded
run, the fixed producer gained an output-only suffix option to avoid replacing
the first manifest or build log. With unchanged matching inputs, reproduce via:

```sh
FORWARD_EVIDENCE_SUFFIX=-replay-1 ~/.codex/bin/codex-run-scratch run --max-bytes 8589934592 --max-files 120000 --seconds 1800 --reserve-bytes 10737418240 -- /usr/bin/python3 docs/evidence/phonk-reactive-flight-20261029/forward-candidate-fixed-producer-20261111.py.txt
```

The scratch source snapshot and binaries were removed. The local recovery ref
described below retains the 51 changed source inputs; reconstruction also
requires the separately supplied portable inputs and cached Jolt source, which
the ref and manifests do not include. Compiler and SDK: Apple clang
21.0.0, macOS SDK 27.2, deployment target 15.0, two compiler jobs.

## Subsequent local installation

The replay above is historical scratch-build evidence, not an installed-game
replay. Subsequently the canonical build used the same 1156-file source
fingerprint and 1250 matching Jolt source hashes. The normal installer delivered
signed engine SHA-256 `8fcfff7fa5b5b43050e75bd14526604aa999002f9f41f129bc98ebfa8dfbfc24`;
the installed receipt records that hash with owner `stage`, and the outer bundle
signature verified. Gameplay delivery reported zero pending files in both
targets. The immediately preceding engine is retained in the installer's backup
as `metal-engine.b48cfc7767c343e038262d3cdceed3763c98b372dd2db7a0249f4d08b6513f05`.
The older `metal-engine.c1f1ba94cabf2fdbfd4bfd091c756d11a9a657c441bf8667eadf0289eddcb0d4`
is retained under `.local-archives/retained-engine-backups/`; both backup bytes
were independently verified against their names. No player profile or SAVE was
changed.

Installation does not establish gameplay acceptance. The user alone checks game
handling, old-save recovery, visible island/fort contact, projectile behaviour
and representative contact performance.

## Local source recovery

`refs/recovery/installed-native/8fcfff7fa5b5` points to local root commit
`dbda4442b6ec431682031cedf2e8c6b079498bdd`, tree
`e04d0da67cc3504352f8a7b23b97d2de9dc8872d`. It contains exactly the 51
changed/untracked manifest paths, as raw Git blobs; it is not a branch, release,
or published source. Each blob was read back and its SHA-256 matched the fixed
manifest. Retrieve a path with `git show refs/recovery/installed-native/8fcfff7fa5b5:<path>`.
The other 1105 manifest paths were independently read from base
`04bb306ad1fcff6227e79a326d23e84315236bbf` and matched their manifest SHA-256;
overlay the 51 recovery paths on that base to reconstruct this source snapshot.
Neither this ref nor the manifest contains the separately required Jolt source,
portable dependencies or installed binary. The ref is local only and is not a
remote backup. Creation used a separate temporary index and verified that HEAD,
the normal index, working status and all other refs did not change.

The six-line mode-off deletion is already absent in HEAD and the separate solver
patch contains inherited changes: neither is a standalone verified
implementation delta suitable for a focused commit on this HEAD.
