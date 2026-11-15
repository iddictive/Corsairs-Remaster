# Volley target type regression, 2026-11-15

## Confirmed source defect
`testVolleyTarget` stores either a SHIP entity ID or a fort MODEL entity ID. The old unconditional `static_cast<SHIP*>` treated the latter as a ship and immediately called its virtual `GetACharacter()`. This is undefined behavior before the later fort-target resolution. The one-line dynamic cast rejects the MODEL while preserving the separate fort resolution path.

## Reproduce
Run with a C++20 compiler supporting UBSan/vptr instrumentation:

```sh
python3 docs/evidence/volley-target-type-20261115/probe.py --repo .
```

`CXX` overrides the compiler (default `g++`). The probe extracts the actual target/captain two statements from the supplied checkout, uses temporary build files, and does not change the repository. It requires the fixed dynamic cast to exist in source. Minimal polymorphic Entity/SHIP/MODEL mocks isolate dispatch semantics.

## Results
Executed using Linux g++, C++20, warnings-as-errors, `-fsanitize=undefined,vptr -fno-sanitize-recover=all`:
- Actual source: a valid ship receives exactly one character call.
- Actual source: fort MODEL resolves to null in ship dispatch; no ship call.
- Actual source: missing/stale entity ID returning null is safe; no ship call.
- Mutation replacing only dynamic_cast with the original static_cast fails under UBSan: `runtime error: downcast ... does not point to an object of type 'SHIP'`, actual object type MODEL.

## Limits
This proves the source type error and its local repair, not the identity of a specific crash instruction. The matching binary/dSYM for UUID 18FA2AC3-F47C-3CEC-8483-8D5C4757AA80 is absent; ExecutePhonkOrdnance+16660 cannot be mapped precisely here. Mocks are not the complete engine inheritance hierarchy. Null stale-ID handling is tested, not dangling pointers. Full fort launch, Metal/runtime stability, and visual acceptance remain untested in this Linux environment. No source edits or remote writes were made by this probe worker.
