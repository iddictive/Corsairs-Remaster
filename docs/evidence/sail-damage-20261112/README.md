# Sail damage crash and hang

The [crash report](crash-report.txt) records an ARM64 invalid access in
`SAIL::ProcessMessage`; its executable UUID matched the installed binary.
The [process sample](hang.sample.txt) later captured the main thread repeatedly
calling `DoRandomsSailsDmg` through the conventional-missile damage event.
The hung process was terminated after ordinary termination did not finish.

The original code indexed a 256-element local selection array by the global
sail index. It also allowed a positive returned damage too small to change the
remaining float, so the loop could repeat without progress. The repair uses a
dynamic selection, indexes it by the selected slot, and bounds work per sail.

The attached self-contained probes retain the actual old and repaired function
with synthetic event and sail objects. On Apple clang 21, compiling the overflow
probes with `clang++ -std=c++20 -O0 -fsanitize=address -x c++` reproduced a
stack-buffer-overflow in the old version; the repaired version passed the
nonzero global index, 300-sail and empty-list cases. The two hang probes were
compiled without ASan and run with a two-second timeout: the old version timed
out, while the repaired version returned. The installed game still requires
player replay; these fixtures do not establish gameplay or frame-rate acceptance.
