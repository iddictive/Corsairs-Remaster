#pragma once
#include "render_value_types.hpp"
#include <cstdio>
#include <set>
#include <string>

inline storm_render::Result unsupported(const char* method) {
    static std::set<std::string> seen;
    if (seen.insert(method).second)
        std::fprintf(stderr,"[StormMetal] unsupported: %s\n",method);
    return storm_render::result::unsupported;
}
