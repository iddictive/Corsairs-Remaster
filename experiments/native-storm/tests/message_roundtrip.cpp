#include "message.h"

#include <cassert>
#include <cstdint>

int main()
{
    MESSAGE message;
    const uintptr_t pointer = static_cast<uintptr_t>(0xfedcba9876543210ULL);
    const entid_t entity = static_cast<entid_t>(0x123456789abcdef0ULL);
    const float scalar = 17.25f;

    message.Reset("pif", pointer, entity, scalar);
    assert(message.Pointer() == pointer);
    assert(message.EntityID() == entity);
    assert(message.Float() == scalar);

    message.Move2Start();
    assert(message.Set(static_cast<uintptr_t>(0x1020304050607080ULL)));
    message.Move2Start();
    assert(message.Pointer() == static_cast<uintptr_t>(0x1020304050607080ULL));
}
