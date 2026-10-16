#pragma once

#include "entity.h"

namespace storm::port
{
// A canonical character may own a live naval ship and a land actor concurrently.
// Keep script attributes shared; never restore them from a copied object.
Entity *FindShipRole(ATTRIBUTES *character);
bool IsShipRoleMessage(int32_t message);
}
