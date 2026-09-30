#include "texture_mip_layout.hpp"

#include <array>
#include <cstdio>
#include <cstdlib>

using storm::renderer::TextureMipCount;
using storm::renderer::TextureMipHeaderIsValid;
using storm::renderer::TextureMipPayloadAvailable;
using storm::renderer::TextureMipReadIsValid;
using storm::renderer::TextureMipSize;
using storm::renderer::kDxt1;
using storm::renderer::kDxt5;

static void require(bool condition, const char *message)
{
    if (!condition)
    {
        std::fputs(message, stderr);
        std::fputc('\n', stderr);
        std::exit(1);
    }
}

int main()
{
    require(TextureMipCount(8, 2) == 4, "rectangular chain count");
    const std::array<uint32_t, 4> rectangular = {16, 8, 8, 8};
    uint32_t width = 8, height = 2;
    for (const auto expected : rectangular)
    {
        require(TextureMipSize(kDxt1, width, height) == expected, "rectangular DXT1 mip extent");
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }

    width = 2048;
    height = 1024;
    std::array<uint32_t, 3> actualRectangularTail{};
    for (uint32_t mip = 0; mip < 12; ++mip)
    {
        const auto bytes = TextureMipSize(kDxt1, width, height);
        if (mip >= 9)
        {
            actualRectangularTail[mip - 9] = bytes;
        }
        width = std::max(1u, width / 2);
        height = std::max(1u, height / 2);
    }
    require(actualRectangularTail == std::array<uint32_t, 3>{8, 8, 8},
            "2048x1024 ship DXT1 tail");

    require(TextureMipSize(kDxt1, 1, 1) == 8, "DXT1 1x1 block minimum");
    require(TextureMipSize(kDxt5, 1, 1) == 16, "DXT5 1x1 block minimum");
    require(TextureMipHeaderIsValid(kDxt1, 8, 2, 4, 16), "valid rectangular header");
    require(!TextureMipHeaderIsValid(kDxt1, 8, 2, 5, 16), "excess mip count rejected");
    require(!TextureMipHeaderIsValid(kDxt1, 8, 2, 4, 8), "undersized level zero rejected");
    require(TextureMipReadIsValid(8, 8), "surface-sized read accepted");
    require(!TextureMipReadIsValid(8, 2), "undersized surface read rejected");
    require(!TextureMipReadIsValid(8, 16), "oversized surface read rejected");
    require(TextureMipPayloadAvailable(8, 8), "complete payload accepted");
    require(!TextureMipPayloadAvailable(7, 8), "truncated payload rejected");

    std::puts("PASS: shared TX layout helper clamps rectangular DXT tails and rejects invalid reads");
}
