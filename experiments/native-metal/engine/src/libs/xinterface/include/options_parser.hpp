#pragma once

#include "attributes.h"

#include <string>

namespace storm {

void removeCarriageReturn(std::string &str);

void parseOptions(const std::string_view &str, ATTRIBUTES &attribute);

[[nodiscard]] std::string serializeOptions(const ATTRIBUTES &attribute);

} // namespace storm
