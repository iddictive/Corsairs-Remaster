#include "message.h"

#include <cassert>
#include <cstdio>

int main()
{
    // COMPILER::CreateMessage stops at an integer supplied for an 'i' slot.
    // FUNC_SEND_MESSAGE still delivers the message; the slot retains uint8_t{}.
    for (const int32_t command : {1, 2, 5})
    {
        MESSAGE malformed;
        malformed.Reset("li");
        malformed.Set(command);
        malformed.Move2Start();
        assert(malformed.Long() == command);
        bool reproduced = false;
        try { (void)malformed.EntityID(); }
        catch (const std::bad_variant_access &) { reproduced = true; }
        assert(reproduced);
        std::printf("before command=%d li+integer-zero: bad_variant_access reproduced\n", command);

        MESSAGE shore;
        shore.Reset("l");
        shore.Set(command);
        shore.Move2Start();
        assert(shore.Long() == command);
        const entid_t shoreCamera = shore.ParamValid() ? shore.EntityID() : invalid_entity;
        assert(shoreCamera == invalid_entity);
        std::printf("after command=%d l: absent camera remains invalid_entity\n", command);

        const entid_t expected = UINT64_C(0x123456789abcdef0);
        MESSAGE deck;
        deck.Reset("li");
        deck.Set(command);
        deck.SetEntity(expected);
        deck.Move2Start();
        assert(deck.Long() == command);
        const entid_t deckCamera = deck.ParamValid() ? deck.EntityID() : invalid_entity;
        assert(deckCamera == expected);
        assert(!deck.ParamValid());
        std::printf("negative command=%d li+entity: full 64-bit deck camera preserved\n", command);
    }
}
