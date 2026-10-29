# Completed NPC crew-service plans

Existing traffic policy initializes ordinary new hulls at OptCrew and recruits
back toward that complement during port service. MinCrew remains a departure
floor, so real casualties and scarce resources are allowed.

A read-only private world snapshot confirmed40of177live ordinary hulls exactly
at MinCrew. Every one retained a completed version2 service plan whose target
was also MinCrew, sometimes across later port reviews. The snapshot and its
source-derived parser remain in the private game diagnostics owner; no save or
player attributes are included here.

WdmTrafficStockService returned immediately for every completed seaworthy plan,
so its old minimum-crew target exempted it from the existing optimal-crew
planner. The candidate reconsiders such a plan only when the actual planner
can fund positive additional recruits. Full crews, active plans, exhausted
reserves and holds with no physical room retain their prior behavior. It never
heals crews at sea or requires unattainable OptCrew for departure.

## Native VM observation

The frozen [driver](replay.py.txt) stages the canonical whole PROGRAM into a
disposable directory, loads actual cannon/goods initialization and invokes the
real service/planning/weight/availability functions. Synthetic hull Min20,
Opt100, Max125 and ample space/reserves starts with an old completed20-man plan.
Baseline returns Run=1 because it remains20; candidate returns Run=0 and
AfterLoad=0, with script_errors=0. [Raw result](observations.json) records the
negative cases and exact input values.

The candidate reserves a target100but leaves actual crew20while work is
pending. A native VM codec roundtrip preserves that state; after24game hours,
FinishService applies100once and repeated completion stays100. Full-game
OnSave/OnLoad hooks are explicitly no-op fixture seams because the synthetic
world has no initialized player, towns or actors. This is codec/component
acceptance, not normal game-load or whole-world acceptance.

To replay, use an existing disposable CODEX_SCRATCH_DIR and run the frozen
Python source with the repository path as its first argument. It reuses the
existing script-vm-probe binary and creates no maintained test target. Baseline
source is main b325c24; the candidate changes only StockService. The first
baseline observation predates the added negative/codec scenarios.

Cost stays on the existing daily stock-review cadence. The previously skipped
completed-undercrew branch can run the existing bounded paid planner; no new
per-frame work or world scan is added. The short VM timings cover different
scenario counts and support no performance claim.

