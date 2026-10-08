# Adaptive fencing

## Status

October 7, 2026: native source/CPU model/defence proofs and canonical engine build
pass after ordered integration. Installed activation and player combat replay
remain pending with root, held while the reopened airburst repair is prepared.
No installed runtime or SAVE changed for this feature.

## Contract and owners

Each live NPC learns the hero's observed melee sequence and gradually adjusts
its ordinary block/parry choice. Repeating a block-breaking thrust becomes more
likely to meet a parry; ordinary strikes favour cheaper blocks. Alternating
attacks can also be learned. A change of tactic gradually replaces old evidence.

`experiments/native-metal/adaptive-fencing.patch` owns the native
`location/src/fencing_learner.h` model and the `NPCharacter` integration in
`np_character.h`/`np_character.cpp`. The canonical ordered patch list in
`experiments/native-metal/build.sh` must consume it before delivery.

The existing AI still owns when to defend, stamina admission, mandatory parry,
recoil, target selection and difficulty. Scripted actor fights retain their
existing nonadaptive branch. Damage, HP, equipment, player controls, animations
and NPC-versus-NPC combat remain unchanged.

## Mechanism and lifecycle

Five attack classes use a fixed 5-by-5 first-order transition table and five
global frequencies, with 0.5 prior counts. Each completed observation decays old
evidence by 0.92. Conditional evidence blends with overall frequency according
to its support; both predict the next block-breaking attack.

An animation enters training only when it leaves its attack class. Holding an
animation for many frames produces one sample. Feint and its counter continuation
share one class. The current incoming animation is never its own training label.
`DoFightAction` observes only the current player target in existing melee range.

The first four samples preserve the original probabilities exactly. Thereafter
confidence ramps over eight samples, multiplied by the NPC's existing fencing
skill. The parry probability shift is capped at +25/-8 percentage points and
final weights at their valid bounds; no additional random draw is introduced.
Low-energy and forced-parry branches return before the learned choice.

Memory belongs to the native NPC instance and observed player identity. Changing
to another target resets it; scene recreation/load starts fresh. There are no
new saved attributes, serialization fields, files, timers, worker processes or
model downloads. Each sample updates 30 fixed floats; inference runs only at an
existing defence decision. This is a source work budget, not measured game FPS.

## Solution choice

The current engine already exposes attack classes and makes a weighted defence
choice. Its script-provided weights are cached in `NPCharacter::PostInit`, so a
script-only change cannot teach an active NPC during combat.

The bounded ready-solution comparison examined
[mlpack incremental NaiveBayesClassifier](https://www.mlpack.org/doc/user/methods/naive_bayes_classifier.html).
Its numeric vectors and variance model do not match this five-category transition
problem. A fixed native categorical estimator fits the existing C++ consumer
without adding the library/matrix dependency or a Python service. Decision:
build the small domain model and retain the engine's existing combat policy.

## Evidence and acceptance

The disposable C++17 CPU driver ran under AddressSanitizer and UBSan with
`-Wall -Wextra -Werror`. It proved completed-only observations, exact four-sample
cold start, zero-skill neutrality, bounded skill response, sequence prediction,
tactic-change forgetting, reset and 10,000 finite/bounded observations.

At skill 1, repeated break attacks reached +0.25 parry probability; switching
to fast attacks produced -0.0638883. For an alternating fast/break sequence,
predicted next break was 0.630176 after fast and 0.223311 after break. These are
model results, not a player fight. The initial probe assumed strict linear skill
scaling past the explicit cap; the correct oracle is monotonic bounded response.

The actual old and candidate `NPCharacter::DoFightBlock` bodies were extracted
into a disposable native driver. Across 2,048 identical seeds per scenario,
cold start, forced parry, insufficient stamina and zero-skill decisions, block
durations and subsequent RNG outputs match exactly. A trained skill-0.7 defender
parried 788/2,048 times versus the original 413/2,048. ASan/UBSan passed. These
prove the decision function and its nearby branches, not whole-scene balance.

The candidate NPC translation unit passes the existing canonical Ninja compiler
command with only source/output replacement for `-fsyntax-only` (exit 0,
25 compiler warnings, no build/object writes). Patch admission against the
current ordered-stack source passes `git apply --check`.

Source discovery binds the prior NPC header SHA-256
`0a0e8d264082973f51d1bb05bef1fad29e0b4e1c124749cec4bc805c072eb229`
and implementation SHA-256
`cecbe6907810cfd506716b966cbc27557a79263705875f59644edb5c41f71516`.
No raw NPCharacter memory serialization or NPCharacter-specific save/load owner
exists in the inspected location sources.

Must not accept this feature from the model probe or build alone. In the played
app, replay repeated thrusts against one sufficiently durable skilled opponent,
then change to ordinary strikes and compare defence choices. Also replay normal
stamina exhaustion/forced parry and an NPC-versus-NPC encounter. Keep identical
HP/damage/difficulty and retain the player's saves. Player scene/balance evidence
and canonical build consumption remain unresolved.
