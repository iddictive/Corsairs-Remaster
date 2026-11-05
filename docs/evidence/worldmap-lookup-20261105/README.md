# World map traffic lookup proxy

`proxy.py` extracts the actual `WdmEnemyShip::GetTrafficData` and
`WdmMerchantShip::KillTest` bodies from baseline commit
`6b674af7cef8642aba7f3cf902bf11a93cea5be8` and the current working source,
compiles each against the same small mock attribute tree, and runs both.
Run `python3 docs/evidence/worldmap-lookup-20261105/proxy.py "$WORK_DIR" > result.json`
from the repository root with an existing disposable work directory. Candidate source is the working tree at run time;
the generated translation-unit hashes and all raw runs are in `results.json`.
The compiler was Apple clang 21.0.0, arm64, `-O2 -std=c++17`.

All ten outcome records matched after excluding the lookup counter: ordinary
far and close, service, paused, needDelete, authored patrol, quest, raider
return, active steering, and no save attribute. The close arrival case also
matched lifecycle/intent mutation, event, and move counts; raider return
matched its route mutation. Lookups fell from 2 to 1 for the far case and
3 to 1 for close cases; authored patrol remains 0 and steering remains 1.

Five consecutive 2,000,000-call ordinary-far mock loops had median wall times
168.8 ms baseline and 84.9 ms candidate. This is a lookup microbenchmark
with mocked storage and no rendering or real VM; it is not a frame-time or FPS
measurement. The separate `WdmEnemyShip::Update` reuse is source-reviewed but
is not executed by this proxy. A full native build and live gameplay remain
necessary to establish integrated behavior and total performance.
