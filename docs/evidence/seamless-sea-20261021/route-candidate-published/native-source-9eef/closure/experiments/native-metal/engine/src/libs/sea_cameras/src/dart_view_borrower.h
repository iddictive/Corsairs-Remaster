#pragma once

namespace storm_dart_view
{
// Non-owning camera borrow only: callers remain responsible for actor lifetime.
bool ReferencesCharacter(int index);
}
