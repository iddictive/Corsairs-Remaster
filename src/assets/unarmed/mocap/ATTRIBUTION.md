# Boxing motion source

**Open Motion Project by ACCAD/The Ohio State University**, Male 2 boxing
captures, licensed under [Creative Commons Attribution 3.0 Unported](https://creativecommons.org/licenses/by/3.0/).

- [Source and licence statement](https://accad.osu.edu/research/motion-lab/mocap-system-and-data)
- [Original Male2 BVH archive](https://accad.ohio-state.edu/sites/accad.osu.edu/files/Male2_bvh.zip)

The three retained BVH members are unchanged source bytes. `recipe.json`
records their SHA-256 hashes, archive hash, exact members, frame trims and
contact-preserving resampling. The left uppercut is mirrored, with semantic
left/right swap, into a rear-right uppercut in the same orthodox stance as the
jab and cross. Generated Storm animations are adaptations:
anatomical retargeting, native skeleton proportions, floor support and shorter
lead/recovery timing. The capture does not contain finger articulation.

The native helper is `tools/boxing_mocap.py`; its consumer is the unarmed bank
generator `tools/prepare_unarmed.py`. No Blender installation or external
runtime dependency is needed to read and retarget these captures.
