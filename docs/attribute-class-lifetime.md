# Attribute class parent chain and liveness

This document preserves the historical failure contracts and their current
source owners. Component evidence does not establish real-game acceptance.

## Contract

Name-based lookup is a non-mutating case-insensitive comparison of the complete
`string_view`; it must not intern absent keys. An October17 lookup optimization
called `STRING_CODEC::Convert(data,length)` on every lookup: that codec uses a
fixed1024-byte stack buffer and treats embedded NUL/empty views differently.
The two name-based loops are restored; existing code-based VM lookup remains.
An embedded NUL must not turn a longer bounded view into a prefix hit. This
lookup repair does not repair the codec's separate length-taking conversion API.
The actual-source UBSan probe reproduced an index2048 overrun in that buffer;
the repaired probe checked long/missing/bounded/empty keys without codec growth.
An unrelated pre-existing scalar-delete/array-new mismatch in
`STRING_CODEC::Release` was excluded from that probe, so this is not a clean
whole-codec sanitizer verdict. This is separate from the parent-chain fix.

An `ATTRIBUTES` node must never be reachable from a live node through `GetThisName()`,
`GetParent()` or the entity `AttributeChanged` dispatch after it has been destroyed. Child
ownership and the child's `parent_` pointer must agree at all times, including after the
owning object is moved.

## Scope

In scope: the engine-core `ATTRIBUTES` tree in `src/libs/core`, the script builtin that
replaces a class wholesale (`CopyAttributes`), and every entity `AttributeChanged`
implementation that walks the ancestor chain. Out of scope: the world-map label content
itself, the renderer, and gameplay scripts.

## Owners

| Concern | Owner |
| --- | --- |
| Node lifetime, parent pointer, tree copy | `src/libs/core/src/attributes.cpp`, `src/libs/core/include/attributes.h` |
| Liveness registry used by guards | `ATTRIBUTES::IsLive`, populated by the constructors and removed by `~ATTRIBUTES` |
| Wholesale class replacement from script | `COMPILER` `FUNC_COPYATTRIBUTES` -> `*pRoot = pA->Copy();` in `src/libs/core/src/internal_functions.cpp` |
| Class copies attached to DATA variables | `src/libs/core/src/data.cpp` `SetReference`/`Set` paths that assign `... = ...->Copy()` |
| Crash-site guard | `WorldMap::AttributeChanged` ancestor walk in `src/libs/worldmap/src/world_map.cpp` |
| Build | Tracked sources under `experiments/native-metal/engine`; ordinary `build.sh` compilation |

The `src/libs` paths above are relative to `experiments/native-metal/engine`.
The current [attribute implementation](../experiments/native-metal/engine/src/libs/core/src/attributes.cpp)
and [world-map consumer](../experiments/native-metal/engine/src/libs/worldmap/src/world_map.cpp)
retain these repairs directly; no historical patch application is required.

## The defect

`ATTRIBUTES::Copy()` builds the copy with `result.CreateAttribute(...)` and then overwrites
each child with `new_child = attribute->Copy();`. Both that assignment and `return result;`
move the child vector. The move constructor and move-assignment operator moved
`attributes_` (a `std::vector<std::unique_ptr<ATTRIBUTES>>`) without re-pointing the moved
children's `parent_` at their new owner.

Every non-root node of a copied tree therefore kept a `parent_` pointing at an object that
was already gone, and the first ancestor walk over such a tree read freed memory. This is a
provable source defect, independent of the crash below: ownership and `parent_` disagree
after any move.

## Evidence

The recorded 2026-09-19 incident faults in `ATTRIBUTES::GetThisName() const + 8` called from
`WorldMap::AttributeChanged(ATTRIBUTES*) + 976`, reached through a script bytecode chain
from `XINTERFACE::MouseDeClick`. `GetThisName` is `stringCodec_.Convert(nameCode_)`; the
faulting instruction is the load through the codec pointer, so the node itself was no longer
live.

Disassembling `WorldMap::AttributeChanged` puts the `bl ATTRIBUTES::GetThisName` whose
return address is exactly symbol + 976 at the `for (auto *pa = apnt; pa; pa = pa->GetParent())`
walk, not at the function's own `apnt->GetThisName()` tests. The node that faulted was an
ancestor reached through `GetParent()`, which is exactly what the copy defect produces.

## Rejected

- Guarding `CoreImpl::Entity_AttributeChanged` only. At that point `pLeftOperandAClass` has
  just been written through, so the passed node is live; the dead object is an ancestor.
- Treating it as a script error. The same statement is correct on trees that were never
  copied, and `CopyAttributes` is used in about thirty script files.
- Increasing the renderer buffer table or reusing the sail fix. Those have
  different ownership and do not repair a stale attribute ancestor.

## Fix

1. `ATTRIBUTES::ReparentChildren()` is called from the move constructor and the
   move-assignment operator, so a moved child vector always agrees with `parent_`.
2. `WorldMap::AttributeChanged` terminates its ancestor walk when a node is no longer live:
   `for (auto *pa = apnt; pa && ATTRIBUTES::IsLive(pa); pa = pa->GetParent())`.

## Acceptance criteria

- Paying crew wages or debt, and any other action that copies an attribute class, must not
  fault; the crash signature `WorldMap::AttributeChanged` -> `ATTRIBUTES::GetThisName`
  must not recur.
- Must not: silence the walk by removing `SetIslandsData` or by dropping the world-map
  label update for live trees. A live `labels` ancestor must still reach `SetIslandsData`.

## Discovery and evidence owners

Read `src/libs/core/src/attributes.cpp` (move constructor, move assignment, `Copy`,
`DeleteAttributeClassX`) and the crash frames before editing. The falsifier is the real
game surface: pay wages or debt, then walk up from the affected class; the nearest negative
is an ordinary world-map update, which must still refresh the island labels.

## Unresolved

- Which specific release destroyed the node in the 2026-09-19 incident is not proven. A
  `Copy()` temporary is the proven producer of the broken `parent_`; an owned class
  released by `DeleteAttributeClassX` or by DATA teardown would look identical at the crash
  site, and the liveness guard covers both without distinguishing them.
- The historical component evidence does not prove replay of the recorded
  encounter-interface action on the installed game.
