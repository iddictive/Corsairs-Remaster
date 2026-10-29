# Impact-camera input

The impact view previously replaced the visible pose with the same
target-relative position every Realize, even while the ordinary camera had
processed mouse and zoom input. Its fixed `.9` projection also discarded deck
zoom. The source-linked baseline has six failing control cases.

## Ownership and contract

`SEA_CAMERAS::RealizeDartView` remains the transient target-view owner.
`COMMON_CAMERA::GetOrbitInput` exposes existing processed input through a
read-only query. All three current implementations participate:

- Ship exports its input angles, with pitch converted to renderer convention,
  and positive radius factor `1 + fDistance`. Its existing distance clamp,
  sensitivity, inversion and inertia still belong to `SHIP_CAMERA::Move`.
- Deck exports local input angles and unit radius. Its existing aim
  magnification remains the zoom owner.
- Free exports local input angles and unit radius.

The impact view anchors that state on each approaching shot. Subsequent
angle differences orbit continuously around the moving accepted target;
distance ratios change radius, and native magnification reaches the projection.
The elevation guard keeps the orbit above its focus plane and away from the
vertical singularity. No input mappings, settings, save fields or fire/target
selection rules are added.

The normal camera keeps processing input and owns the return pose. Renderer
pose, hull translation/heading/rocking and camera collision are deliberately
excluded from the input query: applying their pose differences would make the
impact camera drift without user input. A future unsupported camera yields the
impact override safely. Pause, control locks, player death, telescope, misses,
explicit cancellation and camera changes retain their existing cancellation
owners. Destroyed targets retain the serialized aim-point fallback.

## Reproduction and results

`baseline-probe.cpp.txt` and `candidate-probe.cpp.txt` freeze the actual
Restore/Cancel/Realize methods and the candidate input getters. They use native
`CVECTOR` and mocked core/entities/attributes/render calls; they do not open a
game window. `inputs.json` binds the source and toolchain, while `baseline.json`
and `candidate.json` retain unmodified native process output.

From the repository root, copy one frozen C++ input to a temporary `probe.cpp`
and compile with Apple clang using `-std=c++20 -O2`,
`-Iexperiments/native-metal/engine/src/libs/math/include`, and
`-DCANDIDATE=0` for baseline or `1` for candidate. Run the resulting executable.
The baseline exits1 with six control failures; candidate exits0 with all22
checks passing. No maintained test target or game harness was added.

Cases cover Ship/Deck/Free orbit, Ship distance monotonicity and repeated clamp
state, Deck projection magnification, yaw wrap, a moving ordinary hull/view with
unchanged input, new-shot reset, target death/removal, and restoration of the
current ordinary pose and zoom. The cancellation negatives include disabled
view, player death, pause revision, inactive camera, telescope, locked control
and a miss; an unsupported camera also yields safely.

The delta adds one virtual input query, constant scalar/vector and trigonometric
math, and16 bytes of transient orbit anchor state while an impact view is active.
It adds no polling, allocation, GPU passes or work to inactive normal views.
Whole-frame timing and rendered gameplay are not established by this component
replay; they remain separate runtime acceptance.
