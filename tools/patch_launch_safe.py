
with open('experiments/native-metal/engine/src/libs/ship/src/ship_phonk.hpp', 'r', encoding='utf-8') as f:
    text = f.read()

old_block = '''        // Enable departure whenever launching up an ocean wave crest, out of a whirlpool lip,
        // or through vertical dynamic water ramp acceleration.
        const bool launchTrajectory = (bodyTangent > 4.5f) || (acceleration < -gravity * 1.5f && bodyTangent > 2.0f);
        const bool depart = !airborne && !settling && sampled &&
            bodyTangent > 0.f && launchTrajectory &&
            (!buoyant || (velocity > 0.f && bodyHeight >= height - std::max(.5f, draft)));'''

new_block = '''        // Require strong upward vertical acceleration from a wave ramp to launch into ballistic air,
        // preventing accidental detachment from descending water surfaces like whirlpools.
        const bool launchTrajectory = (acceleration < -gravity * 2.0f && bodyTangent > 5.0f && height > surface);
        const bool depart = !airborne && !settling && sampled &&
            bodyTangent > 0.f && launchTrajectory &&
            bodyHeight >= height - std::max(.5f, draft);'''

assert old_block in text, 'old_block not found in ship_phonk.hpp'
text = text.replace(old_block, new_block, 1)

with open('experiments/native-metal/engine/src/libs/ship/src/ship_phonk.hpp', 'w', encoding='utf-8') as f:
    f.write(text)
print('Fixed launchTrajectory in ship_phonk.hpp!')

