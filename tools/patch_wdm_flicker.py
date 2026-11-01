
with open('experiments/native-metal/engine/src/libs/worldmap/src/wdm_islands.cpp', 'r', encoding='utf-8') as f:
    text = f.read()

old_block = '''                    const bool projected = std::any_of(chart->Regions().begin(), chart->Regions().end(),
                        [&](const auto &region) { return storm::iEquals(region.id, label.name); });
                    // The composed Mainland tree includes its prepared inland
                    // fill. Stock mein also embeds obsolete island miniatures;
                    // loading it again overlays both coasts and whole islands.
                    if (projected || storm::iEquals(label.name, "mein")) continue;
                    const std::string name = std::string("islands\\") + label.name;
                    auto *background = static_cast<WdmRenderModel *>(wdmObjects->wm->CreateModel(
                        new WdmRenderModel(), name.c_str(), false, false, true, 2, wdm::render_levels::Island));'''

new_block = '''                    const bool projected = std::any_of(chart->Regions().begin(), chart->Regions().end(),
                        [&](const auto &region) { return storm::iEquals(region.id, label.name); });
                    const bool bodyMatch = std::any_of(chart->Bodies().begin(), chart->Bodies().end(),
                        [&](const auto &b) { return storm::iEquals(b.id, label.name); });
                    // Prevent duplicate rendering of islands: skip if already loaded via chart physical bodies or regions,
                    // or stock mein which contains overlapping island meshes causing severe Z-fighting flicker.
                    if (projected || bodyMatch || storm::iEquals(label.name, "mein")) continue;
                    const std::string name = std::string("islands\\") + label.name;
                    auto *background = static_cast<WdmRenderModel *>(wdmObjects->wm->CreateModel(
                        new WdmRenderModel(), name.c_str(), false, false, true, 2, wdm::render_levels::Island));'''

assert old_block in text, 'old_block not found in wdm_islands.cpp'
text = text.replace(old_block, new_block, 1)

with open('experiments/native-metal/engine/src/libs/worldmap/src/wdm_islands.cpp', 'w', encoding='utf-8') as f:
    f.write(text)
print('Fixed duplicate island model loading and Z-fighting in wdm_islands.cpp!')

