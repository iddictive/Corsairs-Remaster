#pragma once
#include <string>
#include <string_view>

namespace storm_ship_wood {
// Explicit stock root-GM surfaces, not a texture-name/material heuristic.
// Values author the visible inward shell thickness; they are not measured timber
// dimensions or a solid-volume/mass model. Unlisted/mixed materials stay intact.
struct Surface { std::string_view model, texture; float thickness; };
inline constexpr Surface surfaces[] = {
    {"arabellaship1", "arabellaship.tga", .22f},
    {"arabellaship1", "deck.tga", .18f},
    {"arabellaship_o1", "arabellaship.tga", .22f},
    {"barkas1", "bortouttartane1.tga", .14f},
    {"barkas1", "deck.tga", .12f},
    {"barkentine1", "bortoutbarkentine1.tga", .22f},
    {"barque1", "bortoutbarque1.tga", .22f},
    {"barque1", "deckplankst3.tga", .18f},
    {"barque_w1", "bortoutbarque1.tga", .22f},
    {"battleship1", "superbe2.tga", .22f},
    {"battleship1", "deck.tga", .18f},
    {"bilancetta1", "hull.tga", .22f},
    {"bilancetta1", "deck.tga", .18f},
    {"boat", "boatt1.tga", .12f},
    {"brig1", "brighull1.tga", .22f},
    {"brig1", "deck_brig.tga", .18f},
    {"brig_l1", "bortoutbrig1.tga", .22f},
    {"brig_l1", "deck.tga", .18f},
    {"brigantine1", "bortoutbrigantine1.tga", .22f},
    {"brigantine1", "deck.tga", .18f},
    {"brigqeen1", "us_hull1.tga", .22f},
    {"brigqeen1", "dek3.tga", .18f},
    {"brigsw1", "brigssw.tga", .22f},
    {"brigsw1", "deck.tga", .18f},
    {"caracca1", "bortoutcarraca1.tga", .22f},
    {"caracca1", "deck_carraca.tga", .18f},
    {"caravel1", "bortoutcaravel1.tga", .22f},
    {"caravel1", "deck.tga", .18f},
    {"caravel_w1", "bortoutcaravel1.tga", .22f},
    {"caravel_w1", "deck.tga", .18f},
    {"corvette1", "bortoutcorvette1.tga", .22f},
    {"corvette_l1", "bortoutcorvette1.tga", .22f},
    {"corvette_l1", "deck.tga", .18f},
    {"corvette_quest1", "corvettequest1.tga", .22f},
    {"corvette_quest1", "deck.tga", .18f},
    {"corvette_quest_o1", "corvettequest1.tga", .22f},
    {"corvette_quest_o1", "deck.tga", .18f},
    {"cutter1", "hull.tga", .22f},
    {"cutter1", "deck.tga", .18f},
    {"eastindiaman1", "aop_wood.tga", .22f},
    {"eastindiaman1", "plank_blue.tga", .22f},
    {"eastindiaman1", "woodgrainu1.tga", .22f},
    {"eastindiaman1", "plank_red_inner.tga", .22f},
    {"eastindiaman1", "plank_brown.tga", .22f},
    {"eastindiaman1", "plank_white.tga", .22f},
    {"eastindiaman1", "wooddeck2.tga", .18f},
    {"enslaver1", "bortoutbrignew1.tga", .22f},
    {"enslaver1", "deckx.tga", .18f},
    {"feluca1", "woodre1.tga", .18f},
    {"feluca1", "woodplanksuo3.tga", .18f},
    {"feluca1", "woodplanksuu3.tga", .18f},
    {"feluca1", "deckplanksu3.tga", .14f},
    {"fleut1", "fleut1.tga", .22f},
    {"fleut1", "deck_fleut.tga", .18f},
    {"flyingdutchman1", "flyingdutchman1.tga", .22f},
    {"flyingdutchman1", "deckdutchman.tga", .18f},
    {"flyingdutchman_o1", "flyingdutchman1.tga", .22f},
    {"flyingdutchman_o1", "deckdutchman.tga", .18f},
    {"frigate1", "bortoutfrigate1.tga", .22f},
    {"frigate1", "deck.tga", .18f},
    {"frigate_h1", "bortoutheavy1.tga", .22f},
    {"frigate_m1", "hull1.tga", .22f},
    {"frigate_m1", "deck.tga", .18f},
    {"frigatequeen1", "arabellaship.tga", .22f},
    {"frigatequeen1", "deck.tga", .18f},
    {"galeon1", "bortoutgaleon_h1.tga", .22f},
    {"galeon1", "deck.tga", .18f},
    {"galeon_h1", "spgaleon_h1.tga", .22f},
    {"galeon_h1", "spgaleon_deck.tga", .18f},
    {"galeon_l1", "bortoutgaleon_l1.tga", .22f},
    {"galeon_l1", "deck.tga", .18f},
    {"ketch1", "ketch_hull.tga", .22f},
    {"ketch1", "ketch_deck.tga", .18f},
    {"lineship1", "linehullag.tga", .22f},
    {"lineship1", "linedeckag.tga", .18f},
    {"liuto1", "bortouttartane1.tga", .14f},
    {"lugger1", "bortoutlugger1.tga", .22f},
    {"lugger1", "deck.tga", .18f},
    {"luggerquest1", "bortoutluggerhi1.tga", .22f},
    {"manowar1", "victory2.tga", .22f},
    {"manowar1", "deck.tga", .18f},
    {"pinnace1", "bortoutpinnace1.tga", .22f},
    {"pinnace1", "deck.tga", .18f},
    {"polacre1", "polacrag1.tga", .22f},
    {"polacre1", "poldeck.tga", .18f},
    {"polacre_q1", "qpolag.tga", .22f},
    {"polacre_q1", "qdeckag.tga", .18f},
    {"santisima1", "sanfelipe.tga", .22f},
    {"santisima1", "deck.tga", .18f},
    {"schooner1", "bortschoontrad1.tga", .22f},
    {"schooner1", "deck.tga", .18f},
    {"schooner_w1", "bort_warschoon1.tga", .22f},
    {"schooner_w1", "deck_warschoon1.tga", .18f},
    {"shnyava1", "snauw1.tga", .22f},
    {"shnyava1", "deck_snauw.tga", .18f},
    {"sloop1", "bortoutsloop1.tga", .22f},
    {"sloop1", "deck.tga", .18f},
    {"sloop_w1", "bortoutsloop1.tga", .22f},
    {"soleyru1", "soleilroyal1.tga", .22f},
    {"soleyru1", "deck.tga", .18f},
    {"tartane1", "boatt1.tga", .12f},
    {"tartane1", "bortouttartane1.tga", .12f},
    {"veinard1", "bortout1.tga", .22f},
    {"veinard1", "khil.tga", .22f},
    {"veinard1", "paluba.tga", .18f},
    {"warship1", "raa4.tga", .22f},
    {"warship1", "deck.tga", .18f},
    {"wartartane1", "bortouttartane1.tga", .14f},
    {"xebek_l1", "xebecvml.tga", .22f},
    {"xebek_l1", "deck.tga", .18f},
    {"xebekvml1", "bortoutpolacr1.tga", .22f},
    {"xebekvml1", "deck.tga", .18f},
};
// No entry for Lugger_w1: its sole atlas/object merges unresolved surface roles.
// Barkentine1/Barque_w1 deck.tga is shared by cannon-named objects; unclassified.
// Frigate_h1 deck_new01 has an unresolved alpha-heavy atlas (5.72% clear).
// Corvette1/LuggerQuest1/Sloop_w1/Liuto1/WarTartane1 have no separately proven
// broad deck material. Boat/Tartane1 integrate wood surfaces into their hulls.
// WarShip1's additional upper-hull/trim atlases retain their authored surfaces.
// All cutout ornament, rigging, cannon, metal, lantern and other unknown atlases
// remain unlisted; alpha or a wood-like filename never grants admission.

inline std::string normalized(std::string_view value) {
    std::string result(value);
    for (char &c : result) {
        if (c == '\\') c = '/';
        else if (c >= 'A' && c <= 'Z') c = char(c - 'A' + 'a');
    }
    return result;
}
inline std::string_view basename(std::string_view value) {
    const auto slash = value.find_last_of('/');
    return slash == std::string_view::npos ? value : value.substr(slash + 1);
}
inline float thickness(std::string_view modelPath, std::string_view authoredBaseTexture) {
    const auto path = normalized(modelPath), texturePath = normalized(authoredBaseTexture);
    std::string_view file = basename(path);
    if (file.size() < 4 || file.substr(file.size() - 3) != ".gm") return 0.f;
    const auto model = file.substr(0, file.size() - 3);
    const auto slash = path.find_last_of('/');
    if (slash == std::string::npos) return 0.f;
    const std::string_view folder(path.data(), slash);
    if (basename(folder) != model) return 0.f; // masts/yards/shatter files are not roots.
    const auto parentSlash = folder.find_last_of('/');
    if (parentSlash == std::string_view::npos || basename(folder.substr(0, parentSlash)) != "ships") return 0.f;
    const auto texture = basename(texturePath);
    for (const auto &surface : surfaces)
        if (surface.model == model && surface.texture == texture) return surface.thickness;
    return 0.f;
}
} // namespace storm_ship_wood
