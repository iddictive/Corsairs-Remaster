#include <sanitizer/asan_interface.h>
#include <cstdio>
#include <cstdlib>
int main(int argc, char**) {
    volatile unsigned char *p = new unsigned char[8];
    const bool poisoned = __asan_address_is_poisoned(const_cast<unsigned char*>(p + 8));
    std::printf("redzone_poisoned=%d\n", poisoned);
    std::fflush(stdout);
    if (!poisoned) return 2;
    if (argc > 1) p[8] = 1;
    delete[] p;
    return 0;
}
