#pragma once
#include <algorithm>
#include <cstddef>
#include <dispatch/dispatch.h>
#include <type_traits>

// Bounded, synchronous numeric jobs on the OS-owned reusable worker pool.
// Each index must own its writes; callers keep VM, events and Metal on main.
template <class Function>
void MetalCpuParallelFor(std::size_t count, Function function) noexcept
{
    static_assert(std::is_nothrow_invocable_v<Function &, std::size_t>);
    if (count < 2)
    {
        if (count) function(0);
        return;
    }
    struct Context
    {
        std::size_t count, chunks;
        Function &function;
    } context{count, std::min(count, std::size_t{4}), function};
    dispatch_apply_f(context.chunks, DISPATCH_APPLY_AUTO, &context,
                     [](void *pointer, std::size_t chunk) noexcept {
        auto &context = *static_cast<Context *>(pointer);
        for (std::size_t index = chunk; index < context.count; index += context.chunks)
            context.function(index);
    });
    // dispatch_apply_f joins every job before any captured state can expire.
}
