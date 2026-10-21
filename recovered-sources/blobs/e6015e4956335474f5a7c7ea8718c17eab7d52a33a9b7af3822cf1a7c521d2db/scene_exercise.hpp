#pragma once
#define initialize heldInitialize
#define beforeFrame heldBeforeFrame
#define afterFrame heldAfterFrame
#define finish heldFinish
#include "held_unchanged.hpp"
#undef initialize
#undef beforeFrame
#undef afterFrame
#undef finish
#include "postfix_checks.hpp"
namespace destruction_scene {
inline void initialize(){heldInitialize();postfixInitialize();}
inline void beforeFrame(int f){heldBeforeFrame(f);postfixBeforeFrame(f);}
inline void afterFrame(int f){heldAfterFrame(f);postfixAfterFrame(f);}
inline void finish(){postfixFinish();heldFinish();}
}
