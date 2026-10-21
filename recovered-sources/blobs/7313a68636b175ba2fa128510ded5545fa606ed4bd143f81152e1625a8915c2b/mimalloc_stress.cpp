#include <mimalloc-new-delete.h>
#include <mimalloc.h>
#define main cacheStressMain
#include "cache_stress.cpp"
#undef main
int main(int argc, char **argv)
{
    mi_option_set(mi_option_show_errors, 1);
    mi_option_set(mi_option_show_stats, 0);
    mi_option_set(mi_option_eager_commit, 1);
    mi_option_set(mi_option_eager_region_commit, 1);
    mi_option_set(mi_option_large_os_pages, 1);
    mi_option_set(mi_option_reserve_huge_os_pages, 1);
    const int result = cacheStressMain(argc, argv);
    mi_collect(true);
    return result;
}
