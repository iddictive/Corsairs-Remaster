# Kinzhal terminal-speed correction, 2026-11-16

Base 8c8d2274e105397170481b89185a72fb45bd67c5. Reviewed patch SHA256 369319219f9e56bb5bb4c84da7e46a5116b9132eea64c70ccea05b8d55e61ddd. Published source SHA256 d43168b88004e4e803714e576fcbe2b5419ccbb707521f6aa19d0f7a8d4dcc48 (cloud materializer's single extra final LF excluded).

User requested game terminal flight without curvature-induced speed loss. initializeKinzhalTiming samples curvature in launch-local coordinates, removes terminal curvature braking after half progress, and protects terminal rate through backward propagation. advanceEvasion preserves terminal speed magnitude during replanning, extends remaining duration when needed, and finishes within the last reachable step at target rather than clipping flight prematurely. Existing collision path after advancement retained. No tandem-spacing changes included.

Scoped C++/UBSan evidence: 360 translated trajectory cases; 180 evasion cases; 540 moving-target cases; independent reviewer additionally exercised 1080 early/late evasion cases. Candidate harnesses extract actual timing/evasion functions and use actual header evaluators; these are NOT full native engine collision/AA/gameplay tests. Integration reran three prepared binaries: terminal loss 0.000391982; evasion relative loss 1.15705e-05; endpoint miss0. Independent source review found no blocker. Exact patch applied to fetched remote preimage with fuzz0 and compared to reviewed candidate.

Native build, installed-artifact verification and in-game terminal/evasion/terrain/AA acceptance remain open for this commit. Source publication alone is not a release or installation. Separate tandem pending-release redesign remains unfinished and is not included.
