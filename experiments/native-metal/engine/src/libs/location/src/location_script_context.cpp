#include "location_script_context.hpp"
#include "location.h"

namespace
{
VDATA *LocationElement(VDATA *locations, uint32_t index)
{
    if (!locations || !locations->IsArray() || index >= locations->GetElementsNum()) return nullptr;
    // DATA implements VDATA; the public VM interface exposes array elements as
    // its forward-declared concrete type, as in location_script_lib.cpp.
    return reinterpret_cast<VDATA *>(locations->GetArrayElement(index));
}
}

LocationScriptContext::LocationScriptContext(entid_t locationId)
{
    auto *location = dynamic_cast<Location *>(core.GetEntityPointerSafe(locationId));
    auto *attributes = location ? location->AttributesPointer : nullptr;
    if (!attributes || !attributes->GetAttributeClass("PortContext") ||
        !attributes->GetAttribute("index")) return;
    const char *logicalId = attributes->GetAttribute("id");
    if (!logicalId || !logicalId[0]) return;
    auto *variable = static_cast<VDATA *>(core.GetScriptVariable("loadedLocation"));
    auto *locations = static_cast<VDATA *>(core.GetScriptVariable("Locations"));
    auto *capture = static_cast<VDATA *>(core.GetScriptVariable("LAi_IsCapturedLocation"));
    auto *target = LocationElement(locations, attributes->GetAttributeAsDword("index"));
    if (!variable || !variable->IsReference() || !variable->GetReference() || !capture ||
        !target || target->GetAClass() != attributes || target->GetEntityID() != locationId) return;
    auto *prior = variable->GetAClass();
    if (!ATTRIBUTES::IsLive(prior) || prior == attributes || !prior->GetAttribute("index")) return;
    previousIndex = prior->GetAttributeAsDword("index");
    auto *previous = LocationElement(locations, previousIndex);
    if (!previous || previous->GetAClass() != prior || !capture->Get(previousCaptured)) return;
    previousLocation = previous->GetEntityID();
    auto *previousEntity = dynamic_cast<Location *>(core.GetEntityPointerSafe(previousLocation));
    if (!previousEntity || previousEntity->AttributesPointer != prior) return;

    activeGeneration = Location::ActiveContextGeneration();
    if (auto *active = Location::ActiveContext()) activeLocation = active->GetId();
    loaded = variable;
    previousReference = variable->GetReference();
    previousAttributes = prior;
    borrowedReference = locations->GetArrayElement(attributes->GetAttributeAsDword("index"));
    // Keep the capture policy in its script owner (IsLocationCaptured).
    // Resolve before borrowing so a failed script query cannot leak a reference.
    auto *result = core.Event("Location_GetScriptContextCaptured", "s", logicalId);
    if (!result || !result->Get(borrowedCaptured)) borrowedCaptured = previousCaptured;
    loaded->SetReference(borrowedReference);
    capture->Set(borrowedCaptured);
}

LocationScriptContext::~LocationScriptContext()
{
    if (!loaded || Location::ActiveContextGeneration() != activeGeneration ||
        core.GetScriptVariable("loadedLocation") != loaded || loaded->GetReference() != borrowedReference) return;
    auto *active = Location::ActiveContext();
    if ((active ? active->GetId() : entid_t{}) != activeLocation) return;
    auto *previousEntity = dynamic_cast<Location *>(core.GetEntityPointerSafe(previousLocation));
    if (!previousEntity || previousEntity->AttributesPointer != previousAttributes ||
        !ATTRIBUTES::IsLive(previousAttributes)) return;
    auto *locations = static_cast<VDATA *>(core.GetScriptVariable("Locations"));
    auto *previous = LocationElement(locations, previousIndex);
    if (!previous || previous->GetAClass() != previousAttributes || previous->GetEntityID() != previousLocation ||
        locations->GetArrayElement(previousIndex) != previousReference) return;
    loaded->SetReference(previousReference);
    auto *capture = static_cast<VDATA *>(core.GetScriptVariable("LAi_IsCapturedLocation"));
    int32_t current = 0;
    if (capture && capture->Get(current) && current == borrowedCaptured) capture->Set(previousCaptured);
}
