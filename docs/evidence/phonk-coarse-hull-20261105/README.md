# Coarse reactive hull investigation

The tested path constructs an enclosing 22-plane hull from the current retained
material. Projectile traces and visible damage remain on their existing material
owners. This evidence is from hidden native fixtures, not gameplay acceptance.

On Brig1 with two neighbors, 100 repeated contact predictions had median
0.0308 ms, p95 0.0325 ms, p99 0.0370 ms, and maximum 0.0382 ms; the earlier
same-consumer 2 cm baseline was 576.69 ms for the warm overlap. The full cold
Brig1 admission was 6.64 ms. Current-material preparation before admission is
not included. `repeat-final.txt` contains the raw scene samples.

`dynamic-water-*.txt` records five seconds for several initial rolls, with
nominal or artificially reduced buoyancy. At nominal buoyancy the lowest point
of either hull remained below the water surface. At 10% nominal buoyancy the
fixtures sank; this does not establish that the game's flooding state supplies
that reduction. An upside-down Manowar remained inverted after five seconds.
The medium is calm and these are not gameplay observations.

`damaged-impact-*.txt` records 20 m/s impacts after an actual native material
cut. `outer-damage-output.txt` records a cut followed by the native terrain
fixture. The Nevis vertical traces in `nevis-clearance.txt` measured a minimum
proxy-vertex clearance of -1.44 cm in one frame. The flat-ground stress probes
record temporary penetrations up to 0.36 m at 10 m/s and 1.70 m at 100 m/s with
initial roll; bodies finished on the entry side. Those are known limitations,
not a proof of continuous rotational collision or visual quality.

`native-probe.cpp.txt` is the temporary native probe source. It uses staged
resources, a matching configured native build, and the established fixture setup
from `../phonk-reactive-flight-20261029/native-consumer-runner.py.txt`; it does
not load player saves. For a replay, prepare the empty fixture directory using
that runner, compile current reactive owners into isolated archives with
`../phonk-reactive-flight-20261029/isolated-reactive-build.py.txt`, copy this
probe to the fixture as `probe.cpp`, and run `build-probe.py.txt` with the repo
root and fixture directory as its arguments. Run the resulting probe with
`JET_MODEL_PATH='ships\Brig1\Brig1'`; `JET_WATER_ONLY=1` selects water probes,
and `JET_IMPACT_ONLY=1 JET_DAMAGE_IMPACT=1` additionally selects the cut-impact
case. The 300 m/s experiment changes the probe's initial impact speed. The
component flat-ground cases use the existing production component runner, the
adjacent `floor-10mps.cpp.txt`/`floor-100mps.cpp.txt` inputs and their outputs.
