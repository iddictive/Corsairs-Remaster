#include <cstdio>
#include <cstring>
#include "cloud_visibility_msl.hpp"
int main(){const char* value=storm_metal::cloudVisibilityMSL;
const size_t size=std::strlen(value);return std::fwrite(value,1,size,stdout)==size?0:1;}
