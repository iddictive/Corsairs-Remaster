void SHIP::ExecutePhonkLaser(uint32_t deltaTime)
{
    using namespace storm::ship::phonk;
    if (!bMounted || !GetACharacter()) return;
    if (!phonkLaser.restored)
    {
        phonkLaser.restored = true;
        auto *saved = GetACharacter()->FindAClass(GetACharacter(), "Ship.PhonkLaserCuts");
        if (saved && saved->GetAttribute("model") &&
            std::string(saved->GetAttribute("model")) == cShipIniName)
            for (unsigned n = 0; n < maxLaserCuts; ++n)
            {
                auto *entry = saved->GetAttributeClass("c" + std::to_string(n));
                if (!entry) break;
                LaserPlane plane{CVECTOR(entry->GetAttributeAsFloat("x"), entry->GetAttributeAsFloat("y"),
                                         entry->GetAttributeAsFloat("z")), entry->GetAttributeAsFloat("d"),
                                 entry->GetAttributeAsDword("charred", 1) != 0};
                plane.grain = CVECTOR(entry->GetAttributeAsFloat("gx"), entry->GetAttributeAsFloat("gy", 1.f),
                                      entry->GetAttributeAsFloat("gz"));
                plane.roughness = entry->GetAttributeAsFloat("roughness");
                plane.spacing = entry->GetAttributeAsFloat("spacing", 1.f);
                plane.phase = entry->GetAttributeAsFloat("phase");
                const auto kind = entry->GetAttributeAsDword("kind");
                if (kind > 1 || !storm_hull_damage::readBeamSamples(entry, plane.beams) ||
                    (kind == 1) != !plane.beams.empty()) break;
                if (!ApplyPhonkLaserCut(plane, true)) break;
            }
        auto *flood = GetACharacter()->FindAClass(GetACharacter(), "Ship.PhonkFlood");
        if (flood && !phonkLaser.cuts.empty())
        {
            phonkLaser.flooded = std::max(0.f, flood->GetAttributeAsFloat("volume"));
            phonkLaser.floodOwner = int32_t(flood->GetAttributeAsDword("owner", uint32_t(-1)));
            // Saved ingress resumes with the same open cut, already separated.
            for (auto &sample : phonkLaser.breaches) sample.age = 6.f;
        }
    }
    // A renderer reset recreates GEOM independently of SHIP. Rebind the
    // existing planes without duplicating contact, ingress, damage or pieces.
    if (!phonkLaser.cuts.empty())
    {
        auto *model = GetModel();
        for (int n = 0; auto *node = model->GetNode(n); ++n)
        {
            if (!node->geo) continue;
            const std::string name = node->GetName() ? node->GetName() : "";
            const auto binding = std::find_if(phonkLaser.bindings.begin(), phonkLaser.bindings.end(), [&](const auto &b)
                { return b.node == name && b.generation == node->geo->GeometryGeneration() && b.planes == phonkLaser.cuts.size(); });
            if (binding != phonkLaser.bindings.end()) continue;
            unsigned completed = 0;
            for (const auto &plane : phonkLaser.cuts)
            {
                if (IsAuthoredMastNode(node) && plane.beams.empty()) { ++completed; continue; }
                storm_ship_laser::Result result;
                if (!node->geo->CutByLaser(uint64_t(blots_id), nodeCutField(model->mtx, node, plane), result)) break;
                if (result.fragment) node->geo->ReleaseLaserFragment(uint64_t(blots_id), result.fragment);
                ++completed;
            }
            rememberBinding(phonkLaser, node, completed);
        }
    }
    if (!deltaTime) return;
    PollPhonkLaserWork(deltaTime);
    phonkLaser.time += deltaTime * .001;
    if (phonkLaser.structuralFailure && !phonkLaser.failureReported)
        PublishHullCut(0.f, false);
    const float seconds = deltaTime * .001f;
    const auto commitBeam = [&] {
        if (phonkLaser.pendingBeam.empty()) return true;
        const int32_t owner = int32_t(GetACharacter()->GetAttributeAsDword("index"));
        bool admitted = false;
        if (phonkLaser.pendingWorld)
        { if (pIsland) admitted = pIsland->CutWorldBeam(phonkLaser.pendingContact, phonkLaser.pendingBeam, owner); }
        else if (auto *receiver = dynamic_cast<SHIP *>(core.GetEntityPointer(phonkLaser.pendingContact)); receiver && receiver->bMounted)
        {
            LaserPlane field; field.beams = phonkLaser.pendingBeam;
            const auto &direction = field.beams.front().direction;
            field.normal = CVECTOR(direction[0], direction[1], direction[2]);
            admitted = receiver->ApplyPhonkLaserCut(field, false, owner, GetACharacter());
        }
        else { phonkLaser.pendingBeam.clear(); phonkLaser.burn = 0.f; return true; } // receiver unloaded
        if (admitted) { phonkLaser.pendingBeam.clear(); phonkLaser.burn = 0.f; }
        return admitted;
    };
    phonkLaser.impact.age = std::min(2.f, phonkLaser.impact.age + seconds);
    if (phonkLaser.impact.age > 1.f) phonkLaser.impact.energy = 0.f;
    phonkLaser.reload = std::max(0.f, phonkLaser.reload - seconds);
    phonkLaser.beamAge += seconds;
    phonkLaser.sampleClock += seconds;
    for (auto &edge : phonkLaser.edges) edge.age += seconds;
    std::erase_if(phonkLaser.edges, [](const auto &edge) { return edge.age >= 5.f; });
    phonkLaser.floodClock += seconds;
    if (!isDead() && pSea && !phonkLaser.breaches.empty() && phonkLaser.floodClock >= .25f)
    {
        const float elapsed = phonkLaser.floodClock;
        phonkLaser.floodClock = 0.f;
        for (auto &sample : phonkLaser.breaches)
        {
            sample.age += elapsed;
            const auto point = GetModel()->mtx * sample.point;
            const float depth = pSea->WaveXZ(point.x, point.z) - point.y;
            sample.wetArea = 0.f;
            if (depth <= 0.f) continue;
            const float opening = sample.area > 0.f ? sample.area : sample.length * std::min(2.5f, .18f + sample.age * 2.f);
            sample.wetArea = opening;
            phonkLaser.flooded = std::min(phonkLaser.floodCapacity, phonkLaser.flooded +
                .62f * opening * std::sqrt(2.f * 9.81f * depth) * elapsed);
        }
        const float fraction = std::clamp(phonkLaser.flooded / std::max(1.f, phonkLaser.floodCapacity), 0.f, 1.f);
        auto *flood = GetACharacter()->VerifyAttributeClass("Ship")->VerifyAttributeClass("PhonkFlood");
        flood->SetAttributeUseFloat("volume", phonkLaser.flooded);
        flood->SetAttributeUseDword("owner", uint32_t(phonkLaser.floodOwner));
        core.Event("SeaPhonk_Flood", "alf", GetACharacter(), phonkLaser.floodOwner, fraction);
    }
    if (!PhonkActive())
    {
        commitBeam();
        phonkLaser.charge = std::min(1.f, phonkLaser.charge + seconds * .10f);
        phonkLaser.beamAge = 1.f; phonkLaser.reload = 0.f; phonkLaser.contact = 0;
        phonkLaser.burn = phonkLaser.triggerAge = phonkLaser.sampleClock = 0.f;
        return;
    }
    CONTROL_STATE fire;
    core.Controls->GetControlState("Ship_Fire", fire);
    const bool held = phonkOrdnance.weapon == 6 &&
        (fire.state == CST_ACTIVE || fire.state == CST_ACTIVATED);
    if (!held) phonkLaser.charge = std::min(1.f, phonkLaser.charge + seconds * .10f);
    else if (!phonkLaser.rays.empty())
        phonkLaser.charge = std::max(0.f, phonkLaser.charge - seconds * .16f);
    pSea->AttributesPointer->VerifyAttributeClass("Phonk")->SetAttributeUseFloat("LaserCharge", phonkLaser.charge);
    if (!held || phonkLaser.charge <= 0.f)
    {
        commitBeam();
        phonkLaser.contact = 0; phonkLaser.burn = 0.f;
        phonkLaser.triggerAge = phonkLaser.sampleClock = 0.f;
        if (phonkLaser.charge <= 0.f) phonkLaser.beamAge = 1.f;
        return;
    }
    if (!phonkLaser.rays.empty()) phonkLaser.beamAge = 0.f;
    phonkLaser.triggerAge += seconds;
    if (phonkLaser.sampleClock < laserSampleTime) return;
    if (phonkLaser.pendingBeam.size() >= storm_hull_damage::MaxBeamSamples && !commitBeam()) return;
    const float sampleTime = std::min(phonkLaser.sampleClock, .1f);
    phonkLaser.sampleClock = 0.f;
    CMatrix view;
    pRS->GetTransform(storm_render::TransformView, view);
    view.Transposition();
    const auto camera = view.Pos(), direction = !view.Vz();
    if (!finite(camera) || !finite(direction)) return;
    const bool automatic = automaticLaserCamera();
    const auto end = camera + direction * laserRange;
    float nearest = 1.f;
    SHIP *target = nullptr;
    entid_t worldTarget = 0;
    if (!automatic && pIsland) nearest = std::min(nearest, pIsland->TraceWorldWeapon(camera, end, worldTarget));
    struct Receiver { SHIP *ship; storm_ship_hull::Bounds bounds; bool bounded; };
    std::vector<Receiver> receivers;
    for (const auto id : core.GetEntityIds("ship"))
    {
        auto *ship = static_cast<SHIP *>(core.GetEntityPointer(id));
        if (!ship || ship == this || ship->isDead() || !ship->bMounted || !ship->GetModel()) continue;
        Receiver receiver{ship, {}, false};
        receiver.bounded = completeModelBounds(ship->GetModel(), receiver.bounds);
        receivers.push_back(receiver);
        if (automatic) continue;
        if (receiver.bounded && !storm_ship_hull::rayBounds(receiver.bounds,
            {camera.x, camera.y, camera.z}, {end.x, end.y, end.z})) continue;
        const float hit = ship->Trace(camera, end);
        if (hit >= 0.f && hit < nearest) { nearest = hit; target = ship; worldTarget = 0; }
    }
    phonkLaser.endpoint = camera + (end - camera) * nearest;
    auto *ai = dynamic_cast<AIShip *>(AIHelper::FindAIInnerObj(GetACharacter()));
    auto *cannons = ai ? ai->GetCannonController() : nullptr;
    // Manual aim keeps the crosshair's picked geometry. Automatic selection
    // must reach its hostile receiver through this same real muzzle ray.
    const auto traceLaser = [&](const CVECTOR &picked, LaserRay &ray, SHIP *&receiver,
                                entid_t &worldReceiver, CVECTOR *origin = nullptr)
    {
        ray = LaserRay{};
        const auto muzzles = cannons ? cannons->GetLaserMuzzles(picked) : std::vector<CVECTOR>{};
        if (muzzles.empty()) return false;
        auto muzzle = muzzles[muzzles.size() / 2];
        // A lower gun deck may be under a crest. Prefer the nearest intact
        // exposed gun on this same bort; keep its real origin and occlusion.
        if (pSea && muzzle.y < pSea->WaveXZ(muzzle.x, muzzle.z) + .15f)
        {
            size_t nearestGun = muzzles.size();
            for (size_t n = 0; n < muzzles.size(); ++n)
                if (muzzles[n].y >= pSea->WaveXZ(muzzles[n].x, muzzles[n].z) + .15f &&
                    (nearestGun == muzzles.size() ||
                     std::abs(int(n) - int(muzzles.size() / 2)) < std::abs(int(nearestGun) - int(muzzles.size() / 2))))
                    nearestGun = n;
            if (nearestGun < muzzles.size()) muzzle = muzzles[nearestGun];
        }
        if (origin) *origin = muzzle;
        if (!finite(picked) || (~(picked - muzzle)) < .001f ||
            (automatic && (~(picked - muzzle)) > laserRange * laserRange)) return false;
        const auto rayEnd = picked + !(picked - muzzle) * .5f;
        float fraction = 1.f;
        receiver = nullptr;
        worldReceiver = 0;
        if (pIsland) fraction = std::min(fraction, pIsland->TraceWorldWeapon(muzzle, rayEnd, worldReceiver));
        for (const auto &candidate : receivers)
        {
            auto *ship = candidate.ship;
            if (candidate.bounded && !storm_ship_hull::rayBounds(candidate.bounds,
                {muzzle.x, muzzle.y, muzzle.z}, {rayEnd.x, rayEnd.y, rayEnd.z})) continue;
            const float hit = ship->Trace(muzzle, rayEnd);
            if (hit >= 0.f && hit < fraction) { fraction = hit; receiver = ship; worldReceiver = 0; }
        }
        GetModel()->mtx.MulToInv(muzzle, ray.muzzle);
        ray.endpoint = muzzle + (rayEnd - muzzle) * fraction;
        if (receiver)
        {
            ray.target = uint64_t(receiver->GetId());
            receiver->GetModel()->mtx.MulToInv(ray.endpoint, ray.localHit);
        }
        else if (worldReceiver)
        {
            ray.target = uint64_t(worldReceiver);
            ray.worldTarget = true;
            ray.localHit = ray.endpoint; // Static world records use authored world coordinates.
        }
        return true;
    };
    LaserRay ray;
    target = nullptr;
    worldTarget = 0;
    bool aimed = false;
    if (automatic)
    {
        for (const auto &candidate : hostileTargets(GetACharacter(), GetPos(), laserRange))
        {
            const auto reachesTarget = [&](const CVECTOR &point, CVECTOR *origin = nullptr)
            {
                SHIP *receiver = nullptr;
                entid_t worldReceiver = 0;
                if (!traceLaser(point, ray, receiver, worldReceiver, origin) ||
                    (candidate.ship ? (!receiver || receiver->GetId() != candidate.receiver) :
                                      worldReceiver != candidate.receiver)) return false;
                target = receiver; worldTarget = worldReceiver;
                return true;
            };
            CVECTOR origin(std::numeric_limits<float>::quiet_NaN());
            aimed = reachesTarget(candidate.point, &origin);
            if (!aimed && candidate.ship && finite(origin))
            {
                const auto surfaces = laserHullCandidates(candidate.ship, origin);
                for (size_t n = 0; n < surfaces.count; ++n)
                    if (reachesTarget(surfaces.points[n])) { aimed = true; break; }
            }
            if (aimed) break;
        }
    }
    else aimed = traceLaser(phonkLaser.endpoint, ray, target, worldTarget);
    phonkLaser.rays.clear();
    if (!aimed)
    { commitBeam(); phonkLaser.beamAge = 1.f; phonkLaser.contact = 0; phonkLaser.burn = 0.f; return; }
    phonkLaser.beamAge = 0.f;
    phonkLaser.muzzle = GetModel()->mtx * ray.muzzle;
    phonkLaser.endpoint = ray.endpoint;
    phonkLaser.rays.push_back(ray);
    if (!target && !worldTarget) { commitBeam(); phonkLaser.contact = 0; phonkLaser.burn = 0.f; return; }
    CVECTOR hit = phonkLaser.endpoint;
    if (target) target->GetModel()->mtx.MulToInv(phonkLaser.endpoint, hit);
    const auto contact = worldTarget ? uint64_t(worldTarget) : uint64_t(target->GetId());
    if (phonkLaser.contact != contact || phonkLaser.contactWorld != bool(worldTarget))
    {
        if (!commitBeam()) return;
        phonkLaser.contact = contact; phonkLaser.contactWorld = bool(worldTarget);
        phonkLaser.firstHit = hit; phonkLaser.burn = 0.f;
    }
    phonkLaser.lastHit = hit;
    if (phonkLaser.triggerAge * 16000.f < std::sqrt(~(phonkLaser.endpoint - phonkLaser.muzzle))) return;
    const auto directionWorld = !(phonkLaser.endpoint - phonkLaser.muzzle);
    auto directionLocal = directionWorld;
    if (target) target->GetModel()->mtx.MulToInvNorm(directionWorld, directionLocal);
    directionLocal = !directionLocal;
    storm_hull_damage::BeamSample sample;
    sample.start = {hit.x, hit.y, hit.z}; sample.direction = {directionLocal.x, directionLocal.y, directionLocal.z};
    const auto tangent = storm_ship_laser::beamTangent(sample); sample.tangent = {tangent.x, tangent.y, tangent.z};
    // Metres of material removed per second delivered at this actual contact.
    // Stationary dwell deepens one bore. New aim samples never become a chord
    // or a bounding rectangle spanning unvisited material.
    sample.depth = sampleTime * 4.f;
    if (!storm_ship_laser::valid(sample)) return;
    phonkLaser.pendingContact = contact; phonkLaser.pendingWorld = bool(worldTarget);
    bool merged = false;
    for (auto &prior : phonkLaser.pendingBeam)
    {
        const auto offset = storm_ship_laser::beamStart(sample) - storm_ship_laser::beamStart(prior);
        if (storm_ship_hull::length(offset) < .005f &&
            storm_ship_hull::dot(storm_ship_laser::beamDirection(prior), storm_ship_laser::beamDirection(sample)) > .99999f)
        { prior.depth += sample.depth; merged = true; break; }
    }
    if (!merged) phonkLaser.pendingBeam.push_back(sample);
    phonkLaser.burn += sampleTime;
    if (phonkLaser.burn >= laserBurnTime || phonkLaser.pendingBeam.size() >= storm_hull_damage::MaxBeamSamples) commitBeam();
}
