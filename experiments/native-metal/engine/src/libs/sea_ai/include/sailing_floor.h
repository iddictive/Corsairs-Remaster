#pragma once

class ATTRIBUTES;

// Query live AI orders; the script task attribute can lag group orders.
bool IsShipSailingFloorBlocked(ATTRIBUTES *character);
