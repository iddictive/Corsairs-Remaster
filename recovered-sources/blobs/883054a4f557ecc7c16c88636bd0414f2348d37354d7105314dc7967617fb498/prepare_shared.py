from pathlib import Path
import difflib

root = Path('/REQUIRED_EXTERNAL_INPUT/Corsairs')
temp = Path(__file__).resolve().parent
relative = 'experiments/native-metal/engine/src/libs/ship/src/ship_phonk_laser.cpp'
source = (root / relative).read_text()
candidate = source
if 'Impacts crush a bounded pocket' in source and 'sample.wetArea = opening;' in source:
    (temp/'shared-before.cpp').write_text(source)
    (temp/'shared-after.cpp').write_text(source)
    (temp/'local-impact.patch').write_text('')
    (temp/'flood-state.patch').write_text('')
    print('Using integrated canonical finite-impact source')
    raise SystemExit(0)

def replace(old, new):
    global candidate
    assert candidate.count(old) == 1, old
    candidate = candidate.replace(old, new)

replace('piece.radiusSquared = .25f;', 'piece.surfaceArea = body.surfaceArea;\n            piece.radiusSquared = .25f;')
replace('const float elapsed = std::min(phonkLaser.floodClock, .5f);',
        'const float elapsed = phonkLaser.floodClock;')
replace('''            if (depth <= 0.f) continue;
            const float opening = sample.area > 0.f ? sample.area : sample.length * std::min(2.5f, .18f + sample.age * 2.f);
            phonkLaser.flooded''', '''            sample.wetArea = 0.f;
            if (depth <= 0.f) continue;
            const float opening = sample.area > 0.f ? sample.area : sample.length * std::min(2.5f, .18f + sample.age * 2.f);
            sample.wetArea = opening;
            phonkLaser.flooded''')
replace('phonkLaser.flooded += .62f * opening * std::sqrt(2.f * 9.81f * depth) * elapsed;',
        'phonkLaser.flooded = std::min(phonkLaser.floodCapacity, phonkLaser.flooded +\n                .62f * opening * std::sqrt(2.f * 9.81f * depth) * elapsed);')
replace('    const float threshold = 4.f + 36.f * integrity;\n', '')
replace('    plane.normal = !plane.normal;\n    GEOS::INFO info{};', '''    plane.normal = !plane.normal;
    const float threshold = (4.f + 36.f * integrity) *
        float(GetHullContactStrength(point.z, plane.normal.z) / 75.0);
    GEOS::INFO info{};''')
replace('''    const float depth = support * std::clamp((std::sqrt(energy) - std::sqrt(threshold)) * .025f,
                                            .015f, .55f);''', '''    const float depth = support * std::clamp((std::sqrt(energy) - std::sqrt(threshold)) * .0125f,
                                            .005f, .12f);''')
replace('    plane.distance = (plane.normal | point) - depth;', '''    // Trace the retained skin before choosing a finite crush volume. Convex
    // contact can lie in an old cavity; cut the material the ram actually meets.
    auto *root = GetModel()->GetNode(0);
    const CVECTOR worldNormal = !outwardNormal;
    const CVECTOR rayStart = worldPoint + worldNormal * (support + .5f);
    const CVECTOR rayEnd = worldPoint - worldNormal * (support + .5f);
    CVECTOR a, b;
    root->glob_mtx.MulToInv(rayStart, a);
    root->glob_mtx.MulToInv(rayEnd, b);
    GEOS::VERTEX from{a.x, a.y, a.z}, to{b.x, b.y, b.z};
    const float hit = root->geo->Trace(from, to);
    if (!std::isfinite(hit) || hit < 0.f || hit > 1.f) return;
    GetModel()->mtx.MulToInv(rayStart + (rayEnd - rayStart) * hit, point);
    plane.distance = (plane.normal | point) - depth;''')
replace('''    plane.phase = point.x * .31f + point.y * .17f + point.z * .13f + float(phonkLaser.cuts.size()) * .47f;
    const size_t firstPiece''', '''    plane.phase = point.x * .31f + point.y * .17f + point.z * .13f + float(phonkLaser.cuts.size()) * .47f;
    {
        // Impacts crush a bounded pocket of boards, not a complete
        // half-hull. Unequal overlapping lobes give the edge broken planking;
        // shallow pockets retain a recessed cap, deep ones open real ingress.
        // Reuse the saved finite cutter used by collision/support/crew/rebind.
        const CVECTOR inward = -plane.normal;
        const CVECTOR along = !(up ^ plane.normal);
        const float radius = std::clamp(.3f + .22f * std::sqrt(std::max(0.f, energy - threshold)),
                                        .3f, std::min(2.f, std::max(.3f, timberSpan * .2f)));
        for (unsigned n = 0; n < 3; ++n)
        {
            const float shift = n == 0 ? -.55f : n == 1 ? 0.f : .6f;
            const float scale = n == 0 ? .72f : n == 1 ? 1.f : .8f;
            const auto start = point + plane.normal * .025f + along * (radius * shift);
            storm_hull_damage::BeamSample sample;
            sample.start = {start.x, start.y, start.z};
            sample.direction = {inward.x, inward.y, inward.z};
            sample.tangent = {up.x, up.y, up.z};
            sample.radius = radius * scale;
            sample.depth = .025f + depth * scale;
            plane.beams.push_back(sample);
        }
        plane.roughness = 0.f;
    }
    const size_t firstPiece''')

start = candidate.index('    // A catastrophic broadside load near midship breaks across the keel.')
end = candidate.index('    const CVECTOR up =', start)
candidate = candidate[:start] + candidate[end:]

(temp/'shared-before.cpp').write_text(source)
(temp/'shared-after.cpp').write_text(candidate)
patch = ''.join(difflib.unified_diff(source.splitlines(True), candidate.splitlines(True),
    fromfile='a/' + relative, tofile='b/' + relative))
(temp/'local-impact.patch').write_text(patch)
relative_hpp = 'experiments/native-metal/engine/src/libs/ship/src/ship_phonk_laser.hpp'
header = (root / relative_hpp).read_text()
old = '    float area = 0.f; // Finite removed skin; zero retains legacy separating-seam growth.\n'
assert header.count(old) == 1
next_header = header.replace(old,old+'    float wetArea = 0.f; // derived by the actual4Hz ingress/wave sampler; never saved\n')
(temp/'flood-state.patch').write_text(''.join(difflib.unified_diff(
    header.splitlines(True),next_header.splitlines(True),fromfile='a/'+relative_hpp,tofile='b/'+relative_hpp)))
print('Prepared finite-impact / elapsed-ingress / fragment-measure patch:', len(patch), 'bytes')
