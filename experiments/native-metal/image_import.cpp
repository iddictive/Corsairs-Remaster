
#include "image_import.hpp"
#include "render_device_api.hpp"

#include <cstdio>
#include <cstring> // for memcpy
#include <algorithm>

#define WARN(...) fprintf(stdout, __VA_ARGS__)

///////////////////////// Parts from WINE source code for d3dx9 licensed under GPLv2 /////////////////////////


namespace storm_render {
namespace {
constexpr Result imageInvalidData = static_cast<Result>(0x88760b59u);

struct vec4
{
    float x, y, z, w;
};

struct volume
{
    std::uint32_t width;
    std::uint32_t height;
    std::uint32_t depth;
};

enum format_type {
    FORMAT_ARGB,   /* unsigned */
    FORMAT_ARGBF16,/* float 16 */
    FORMAT_ARGBF,  /* float */
    FORMAT_DXT,
    FORMAT_INDEX,
    FORMAT_UNKNOWN
};

struct pixel_format_desc {
    AssetFormat format;
    std::uint8_t bits[4];
    std::uint8_t shift[4];
    std::uint32_t bytes_per_pixel;
    std::uint32_t block_width;
    std::uint32_t block_height;
    std::uint32_t block_byte_count;
    enum format_type type;
    void (*from_rgba)(const struct vec4 *src, struct vec4 *dst);
    void (*to_rgba)(const struct vec4 *src, struct vec4 *dst, const PaletteEntry *palette);
};

/************************************************************
 * pixel format table providing info about number of bytes per pixel,
 * number of bits per channel and format type.
 *
 * Call get_format_info to request information about a specific format.
 */
static const struct pixel_format_desc formats[] =
{
    /* format              bpc               shifts             bpp blocks   type            from_rgba     to_rgba */
    {AssetFormat::R8G8B8,        { 0,  8,  8,  8}, { 0, 16,  8,  0},  3, 1, 1,  3, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::A8R8G8B8,      { 8,  8,  8,  8}, {24, 16,  8,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::X8R8G8B8,      { 0,  8,  8,  8}, { 0, 16,  8,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::A8B8G8R8,      { 8,  8,  8,  8}, {24,  0,  8, 16},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::X8B8G8R8,      { 0,  8,  8,  8}, { 0,  0,  8, 16},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::R5G6B5,        { 0,  5,  6,  5}, { 0, 11,  5,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::X1R5G5B5,      { 0,  5,  5,  5}, { 0, 10,  5,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::A1R5G5B5,      { 1,  5,  5,  5}, {15, 10,  5,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::R3G3B2,        { 0,  3,  3,  2}, { 0,  5,  2,  0},  1, 1, 1,  1, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::A8R3G3B2,      { 8,  3,  3,  2}, { 8,  5,  2,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::A4R4G4B4,      { 4,  4,  4,  4}, {12,  8,  4,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::X4R4G4B4,      { 0,  4,  4,  4}, { 0,  8,  4,  0},  2, 1, 1,  2, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::A2R10G10B10,   { 2, 10, 10, 10}, {30, 20, 10,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::A2B10G10R10,   { 2, 10, 10, 10}, {30,  0, 10, 20},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::A16B16G16R16,  {16, 16, 16, 16}, {48,  0, 16, 32},  8, 1, 1,  8, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::G16R16,        { 0, 16, 16,  0}, { 0,  0, 16,  0},  4, 1, 1,  4, FORMAT_ARGB,    NULL,         NULL      },
    {AssetFormat::A8,            { 8,  0,  0,  0}, { 0,  0,  0,  0},  1, 1, 1,  1, FORMAT_ARGB,    NULL,         NULL      },
    //{AssetFormat::A8L8,          { 8,  8,  0,  0}, { 8,  0,  0,  0},  2, 1, 1,  2, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
    //{AssetFormat::A4L4,          { 4,  4,  0,  0}, { 4,  0,  0,  0},  1, 1, 1,  1, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
    //{AssetFormat::L8,            { 0,  8,  0,  0}, { 0,  0,  0,  0},  1, 1, 1,  1, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
    //{AssetFormat::L16,           { 0, 16,  0,  0}, { 0,  0,  0,  0},  2, 1, 1,  2, FORMAT_ARGB,    la_from_rgba, la_to_rgba},
    {AssetFormat::DXT1,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4,  8, FORMAT_DXT,     NULL,         NULL      },
    {AssetFormat::DXT2,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
    {AssetFormat::DXT3,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
    {AssetFormat::DXT4,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
    {AssetFormat::DXT5,          { 0,  0,  0,  0}, { 0,  0,  0,  0},  1, 4, 4, 16, FORMAT_DXT,     NULL,         NULL      },
    {AssetFormat::R16F,          { 0, 16,  0,  0}, { 0,  0,  0,  0},  2, 1, 1,  2, FORMAT_ARGBF16, NULL,         NULL      },
    {AssetFormat::G16R16F,       { 0, 16, 16,  0}, { 0,  0, 16,  0},  4, 1, 1,  4, FORMAT_ARGBF16, NULL,         NULL      },
    {AssetFormat::A16B16G16R16F, {16, 16, 16, 16}, {48,  0, 16, 32},  8, 1, 1,  8, FORMAT_ARGBF16, NULL,         NULL      },
    {AssetFormat::R32F,          { 0, 32,  0,  0}, { 0,  0,  0,  0},  4, 1, 1,  4, FORMAT_ARGBF,   NULL,         NULL      },
    {AssetFormat::G32R32F,       { 0, 32, 32,  0}, { 0,  0, 32,  0},  8, 1, 1,  8, FORMAT_ARGBF,   NULL,         NULL      },
    {AssetFormat::A32B32G32R32F, {32, 32, 32, 32}, {96,  0, 32, 64}, 16, 1, 1, 16, FORMAT_ARGBF,   NULL,         NULL      },
    //{AssetFormat::P8,            { 8,  8,  8,  8}, { 0,  0,  0,  0},  1, 1, 1,  1, FORMAT_INDEX,   NULL,         index_to_rgba},
    /* marks last element */
    {AssetFormat::Unknown,       { 0,  0,  0,  0}, { 0,  0,  0,  0},  0, 1, 1,  0, FORMAT_UNKNOWN, NULL,         NULL      },
};

/************************************************************
 * get_format_info
 *
 * Returns information about the specified format.
 * If the format is unsupported, it's filled with the AssetFormat::Unknown desc.
 *
 * PARAMS
 *   format [I] format whose description is queried
 *
 */
const struct pixel_format_desc *get_format_info(AssetFormat format)
{
    unsigned int i = 0;
    while(formats[i].format != format && formats[i].format != AssetFormat::Unknown) i++;
    if (formats[i].format == AssetFormat::Unknown) {
        WARN("Unknown format %#x.\n", static_cast<std::uint32_t>(format));
    }
    return &formats[i];
}

static inline bool SetRect(Rect *rect, std::int32_t left, std::int32_t top, std::int32_t right, std::int32_t bottom)
{
    if (!rect) return false;
    rect->left   = left;
    rect->right  = right;
    rect->top    = top;
    rect->bottom = bottom;
    return true;
}

Result lock_surface(Surface *surface, const Rect *surface_rect, LockedRect *lock,
                     Surface **temp_surface, bool write)
{
    unsigned int width, height;
    Device *device;
    SurfaceDesc desc;
    std::uint32_t lock_flag;
    Result hr;

    lock_flag = write ? 0 : lock::readOnly;
    *temp_surface = NULL;
    if (failed(hr = surface->LockRect(lock, surface_rect, lock_flag)))
    {
        surface->GetDevice(&device);
        surface->GetDesc(&desc);

        if (surface_rect)
        {
            width = surface_rect->right - surface_rect->left;
            height = surface_rect->bottom - surface_rect->top;
        }
        else
        {
            width = desc.Width;
            height = desc.Height;
        }

        hr = write ? device->CreateOffscreenPlainSurface(width, height,
                                                                  desc.Format, temp_surface)
                   : device->CreateRenderTarget(width, height,
                                                         desc.Format, temp_surface);
        if (failed(hr))
        {
            WARN("Failed to create temporary surface, surface %p, format %#x,"
                 " usage %#x, pool %#x, write %#x, width %u, height %u.\n",
                 static_cast<void *>(surface), static_cast<std::uint32_t>(desc.Format),
                 desc.Usage, static_cast<std::uint32_t>(desc.Pool), write, width, height);
            device->Release();
            return hr;
        }

        if (write || succeeded(hr = device->StretchRect(surface, surface_rect,
                                                                 *temp_surface, NULL, TextureFilterNone)))
            hr = (*temp_surface)->LockRect(lock, NULL, lock_flag);

        device->Release();
        if (failed(hr))
        {
            WARN("Failed to lock surface %p, write %#x, usage %#x, pool %#x.\n",
                 static_cast<void *>(surface), write, desc.Usage,
                 static_cast<std::uint32_t>(desc.Pool));
            (*temp_surface)->Release();
            *temp_surface = NULL;
            return hr;
        }
    }
    return hr;
}

Result unlock_surface(Surface *surface, const Rect *surface_rect,
                       Surface *temp_surface, bool update)
{
    Device *device;
    Point surface_point;
    Result hr;

    if (!temp_surface)
    {
        hr = surface->UnlockRect();
        return hr;
    }

    hr = temp_surface->UnlockRect();
    if (update)
    {
        if (surface_rect)
        {
            surface_point.x = surface_rect->left;
            surface_point.y = surface_rect->top;
        }
        else
        {
            surface_point.x = 0;
            surface_point.y = 0;
        }
        surface->GetDevice(&device);
        if (failed(hr = device->UpdateSurface(temp_surface, NULL, surface, &surface_point)))
            WARN("Updating surface failed, hr %#x, surface %p, temp_surface %p.\n",
                 hr, static_cast<void *>(surface), static_cast<void *>(temp_surface));
        device->Release();
    }
    temp_surface->Release();
    return hr;
}

/************************************************************
 * copy_pixels
 *
 * Copies the source buffer to the destination buffer.
 * Works for any pixel format.
 * The source and the destination must be block-aligned.
 */
void copy_pixels(const std::uint8_t *src, std::uint32_t src_row_pitch, std::uint32_t src_slice_pitch,
                 std::uint8_t *dst, std::uint32_t dst_row_pitch, std::uint32_t dst_slice_pitch, const struct volume *size,
                 const struct pixel_format_desc *format)
{
    std::uint32_t row, slice;
    std::uint8_t *dst_addr;
    const std::uint8_t *src_addr;
    std::uint32_t row_block_count = (size->width + format->block_width - 1) / format->block_width;
    std::uint32_t row_count = (size->height + format->block_height - 1) / format->block_height;

    for (slice = 0; slice < size->depth; slice++)
    {
        src_addr = src + slice * src_slice_pitch;
        dst_addr = dst + slice * dst_slice_pitch;

        for (row = 0; row < row_count; row++)
        {
            memcpy(dst_addr, src_addr, row_block_count * format->block_byte_count);
            src_addr += src_row_pitch;
            dst_addr += dst_row_pitch;
        }
    }
}

static inline bool is_conversion_from_supported(const struct pixel_format_desc *format)
{
    if (format->type == FORMAT_ARGB || format->type == FORMAT_ARGBF16
        || format->type == FORMAT_ARGBF || format->type == FORMAT_DXT)
        return true;
    return !!format->to_rgba;
}

static inline bool is_conversion_to_supported(const struct pixel_format_desc *format)
{
    if (format->type == FORMAT_ARGB || format->type == FORMAT_ARGBF16
        || format->type == FORMAT_ARGBF || format->type == FORMAT_DXT)
        return true;
    return !!format->from_rgba;
}

/************************************************************
 * helper functions for ImageLoadSurfaceFromMemory
 */
struct argb_conversion_info
{
    const struct pixel_format_desc *srcformat;
    const struct pixel_format_desc *destformat;
    std::uint32_t srcshift[4], destshift[4];
    std::uint32_t srcmask[4], destmask[4];
    bool process_channel[4];
    std::uint32_t channelmask;
};

#ifndef NOMINMAX
#define NOMINMAX
#undef min
#undef max
#endif

inline int min(int x,int y){ return x < y ? x : y; }
inline float min(float x,float y){ return x < y ? x : y; }
inline double min(double x,double y){ return x < y ? x : y; }

inline int max(int x,int y){ return x > y ? x : y; }
inline float max(float x,float y){ return x > y ? x : y; }
inline double max(double x,double y){ return x > y ? x : y; }

static void init_argb_conversion_info(const struct pixel_format_desc *srcformat, const struct pixel_format_desc *destformat, struct argb_conversion_info *info)
{
    std::uint32_t i;
    //ZeroMemory(info->process_channel, 4 * sizeof(bool));
    memset(info->process_channel, 0, 4 * sizeof(bool));
    info->channelmask = 0;

    info->srcformat  =  srcformat;
    info->destformat = destformat;

    for(i = 0;i < 4;i++) {
        /* srcshift is used to extract the _relevant_ components */
        info->srcshift[i]  =  srcformat->shift[i] + max( srcformat->bits[i] - destformat->bits[i], 0);

        /* destshift is used to move the components to the correct position */
        info->destshift[i] = destformat->shift[i] + max(destformat->bits[i] -  srcformat->bits[i], 0);

        info->srcmask[i]  = ((1 <<  srcformat->bits[i]) - 1) <<  srcformat->shift[i];
        info->destmask[i] = ((1 << destformat->bits[i]) - 1) << destformat->shift[i];

        /* channelmask specifies bits which aren't used in the source format but in the destination one */
        if(destformat->bits[i]) {
            if(srcformat->bits[i]) info->process_channel[i] = true;
            else info->channelmask |= info->destmask[i];
        }
    }
}

/************************************************************
 * get_relevant_argb_components
 *
 * Extracts the relevant components from the source color and
 * drops the less significant bits if they aren't used by the destination format.
 */
static void get_relevant_argb_components(const struct argb_conversion_info *info, const std::uint8_t *col, std::uint32_t *out)
{
    unsigned int i, j;
    unsigned int component, mask;

    for (i = 0; i < 4; ++i)
    {
        if (!info->process_channel[i])
            continue;

        component = 0;
        mask = info->srcmask[i];
        for (j = 0; j < 4 && mask; ++j)
        {
            if (info->srcshift[i] < j * 8)
                component |= (col[j] & mask) << (j * 8 - info->srcshift[i]);
            else
                component |= (col[j] & mask) >> (info->srcshift[i] - j * 8);
            mask >>= 8;
        }
        out[i] = component;
    }
}

/************************************************************
 * make_argb_color
 *
 * Recombines the output of get_relevant_argb_components and converts
 * it to the destination format.
 */
static std::uint32_t make_argb_color(const struct argb_conversion_info *info, const std::uint32_t *in)
{
    std::uint32_t i;
    std::uint32_t val = 0;

    for(i = 0;i < 4;i++) {
        if(info->process_channel[i]) {
            /* necessary to make sure that e.g. an X4R4G4B4 white maps to an R8G8B8 white instead of 0xf0f0f0 */
            signed int shift;
            for(shift = info->destshift[i]; shift > info->destformat->shift[i]; shift -= info->srcformat->bits[i]) val |= in[i] << shift;
            val |= (in[i] >> (info->destformat->shift[i] - shift)) << info->destformat->shift[i];
        }
    }
    val |= info->channelmask;   /* new channels are set to their maximal value */
    return val;
}

/* It doesn't work for components bigger than 32 bits (or somewhat smaller but unaligned). */
static void format_to_vec4(const struct pixel_format_desc *format, const std::uint8_t *src, struct vec4 *dst)
{
    std::uint32_t mask, tmp;
    unsigned int c;

    for (c = 0; c < 4; ++c)
    {
        static const unsigned int component_offsets[4] = {3, 0, 1, 2};
        float *dst_component = (float *)dst + component_offsets[c];

        if (format->bits[c])
        {
            mask = ~0u >> (32 - format->bits[c]);

            memcpy(&tmp, src + format->shift[c] / 8,
                    std::min<std::size_t>(sizeof(std::uint32_t), (format->shift[c] % 8 + format->bits[c] + 7) / 8));

            if (format->type == FORMAT_ARGBF16)
                WARN("Unimplemented for FORMAT_ARGBF16");
                //*dst_component = float_16_to_32(tmp);
            else if (format->type == FORMAT_ARGBF)
                *dst_component = *(float *)&tmp;
            else
                *dst_component = (float)((tmp >> format->shift[c] % 8) & mask) / mask;
        }
        else
            *dst_component = 1.0f;
    }
}

/* It doesn't work for components bigger than 32 bits. */
static void format_from_vec4(const struct pixel_format_desc *format, const struct vec4 *src, std::uint8_t *dst)
{
    std::uint32_t v, mask32;
    unsigned int c, i;

    memset(dst, 0, format->bytes_per_pixel);

    for (c = 0; c < 4; ++c)
    {
        static const unsigned int component_offsets[4] = {3, 0, 1, 2};
        const float src_component = *((const float *)src + component_offsets[c]);

        if (!format->bits[c])
            continue;

        mask32 = ~0u >> (32 - format->bits[c]);

        if (format->type == FORMAT_ARGBF16)
            WARN("Unimplemented for FORMAT_ARGBF16");
            //v = float_32_to_16(src_component);
        else if (format->type == FORMAT_ARGBF)
            v = *(std::uint32_t *)&src_component;
        else
            v = (std::uint32_t)(src_component * ((1 << format->bits[c]) - 1) + 0.5f);

        for (i = format->shift[c] / 8 * 8; i < format->shift[c] + format->bits[c]; i += 8)
        {
            std::uint8_t mask, byte;

            if (format->shift[c] > i)
            {
                mask = mask32 << (format->shift[c] - i);
                byte = (v << (format->shift[c] - i)) & mask;
            }
            else
            {
                mask = mask32 >> (i - format->shift[c]);
                byte = (v >> (i - format->shift[c])) & mask;
            }
            dst[i / 8] |= byte;
        }
    }
}

/************************************************************
 * convert_argb_pixels
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion and color keying.
 * Pixels outsize the source rect are blacked out.
 */
void convert_argb_pixels(const std::uint8_t *src, std::uint32_t src_row_pitch, std::uint32_t src_slice_pitch, const struct volume *src_size,
                         const struct pixel_format_desc *src_format, std::uint8_t *dst, std::uint32_t dst_row_pitch, std::uint32_t dst_slice_pitch,
                         const struct volume *dst_size, const struct pixel_format_desc *dst_format, PackedColor color_key,
                         const PaletteEntry *palette)
{
    struct argb_conversion_info conv_info, ck_conv_info;
    const struct pixel_format_desc *ck_format = NULL;
    std::uint32_t channels[4];
    std::uint32_t min_width, min_height, min_depth;
    std::uint32_t x, y, z;

    memset(channels, 0, sizeof(channels));
    init_argb_conversion_info(src_format, dst_format, &conv_info);

    min_width = std::min(src_size->width, dst_size->width);
    min_height = std::min(src_size->height, dst_size->height);
    min_depth = std::min(src_size->depth, dst_size->depth);

    if (color_key)
    {
        /* Color keys are always represented in AssetFormat::A8R8G8B8 format. */
        ck_format = get_format_info(AssetFormat::A8R8G8B8);
        init_argb_conversion_info(src_format, ck_format, &ck_conv_info);
    }

    for (z = 0; z < min_depth; z++) {
        const std::uint8_t *src_slice_ptr = src + z * src_slice_pitch;
        std::uint8_t *dst_slice_ptr = dst + z * dst_slice_pitch;

        for (y = 0; y < min_height; y++) {
            const std::uint8_t *src_ptr = src_slice_ptr + y * src_row_pitch;
            std::uint8_t *dst_ptr = dst_slice_ptr + y * dst_row_pitch;

            for (x = 0; x < min_width; x++) {
                if (!src_format->to_rgba && !dst_format->from_rgba
                    && src_format->type == dst_format->type
                    && src_format->bytes_per_pixel <= 4 && dst_format->bytes_per_pixel <= 4)
                {
                    std::uint32_t val;

                    get_relevant_argb_components(&conv_info, src_ptr, channels);
                    val = make_argb_color(&conv_info, channels);

                    if (color_key)
                    {
                        std::uint32_t ck_pixel;

                        get_relevant_argb_components(&ck_conv_info, src_ptr, channels);
                        ck_pixel = make_argb_color(&ck_conv_info, channels);
                        if (ck_pixel == color_key)
                            val &= ~conv_info.destmask[0];
                    }
                    if (dst_format->format == AssetFormat::Q8W8V8U8) {
                        WARN("Unimplemented for AssetFormat::Q8W8V8U8");
                        //val = from_rgba_qwvu(val);
                    }
                    memcpy(dst_ptr, &val, dst_format->bytes_per_pixel);
                }
                else
                {
                    struct vec4 color, tmp;

                    format_to_vec4(src_format, src_ptr, &color);
                    if (src_format->to_rgba)
                        src_format->to_rgba(&color, &tmp, palette);
                    else
                        tmp = color;

                    if (ck_format)
                    {
                        std::uint32_t ck_pixel;

                        format_from_vec4(ck_format, &tmp, (std::uint8_t *)&ck_pixel);
                        if (ck_pixel == color_key)
                            tmp.w = 0.0f;
                    }

                    if (dst_format->from_rgba)
                        dst_format->from_rgba(&tmp, &color);
                    else
                        color = tmp;

                    format_from_vec4(dst_format, &color, dst_ptr);
                }

                src_ptr += src_format->bytes_per_pixel;
                dst_ptr += dst_format->bytes_per_pixel;
            }

            if (src_size->width < dst_size->width) /* black out remaining pixels */
                memset(dst_ptr, 0, dst_format->bytes_per_pixel * (dst_size->width - src_size->width));
        }

        if (src_size->height < dst_size->height) /* black out remaining pixels */
            memset(dst + src_size->height * dst_row_pitch, 0, dst_row_pitch * (dst_size->height - src_size->height));
    }
    if (src_size->depth < dst_size->depth) /* black out remaining pixels */
        memset(dst + src_size->depth * dst_slice_pitch, 0, dst_slice_pitch * (dst_size->depth - src_size->depth));
}

/************************************************************
 * point_filter_argb_pixels
 *
 * Copies the source buffer to the destination buffer, performing
 * any necessary format conversion, color keying and stretching
 * using a point filter.
 */
void point_filter_argb_pixels(const std::uint8_t *src, std::uint32_t src_row_pitch, std::uint32_t src_slice_pitch, const struct volume *src_size,
                              const struct pixel_format_desc *src_format, std::uint8_t *dst, std::uint32_t dst_row_pitch, std::uint32_t dst_slice_pitch,
                              const struct volume *dst_size, const struct pixel_format_desc *dst_format, PackedColor color_key,
                              const PaletteEntry *palette)
{
    struct argb_conversion_info conv_info, ck_conv_info;
    const struct pixel_format_desc *ck_format = NULL;
    std::uint32_t channels[4];
    std::uint32_t x, y, z;

    memset(channels, 0, sizeof(channels));
    init_argb_conversion_info(src_format, dst_format, &conv_info);

    if (color_key)
    {
        /* Color keys are always represented in AssetFormat::A8R8G8B8 format. */
        ck_format = get_format_info(AssetFormat::A8R8G8B8);
        init_argb_conversion_info(src_format, ck_format, &ck_conv_info);
    }

    for (z = 0; z < dst_size->depth; z++)
    {
        std::uint8_t *dst_slice_ptr = dst + z * dst_slice_pitch;
        const std::uint8_t *src_slice_ptr = src + src_slice_pitch * (z * src_size->depth / dst_size->depth);

        for (y = 0; y < dst_size->height; y++)
        {
            std::uint8_t *dst_ptr = dst_slice_ptr + y * dst_row_pitch;
            const std::uint8_t *src_row_ptr = src_slice_ptr + src_row_pitch * (y * src_size->height / dst_size->height);

            for (x = 0; x < dst_size->width; x++)
            {
                const std::uint8_t *src_ptr = src_row_ptr + (x * src_size->width / dst_size->width) * src_format->bytes_per_pixel;

                if (!src_format->to_rgba && !dst_format->from_rgba
                    && src_format->type == dst_format->type
                    && src_format->bytes_per_pixel <= 4 && dst_format->bytes_per_pixel <= 4)
                {
                    std::uint32_t val;

                    get_relevant_argb_components(&conv_info, src_ptr, channels);
                    val = make_argb_color(&conv_info, channels);

                    if (color_key)
                    {
                        std::uint32_t ck_pixel;

                        get_relevant_argb_components(&ck_conv_info, src_ptr, channels);
                        ck_pixel = make_argb_color(&ck_conv_info, channels);
                        if (ck_pixel == color_key)
                            val &= ~conv_info.destmask[0];
                    }
                    memcpy(dst_ptr, &val, dst_format->bytes_per_pixel);
                }
                else
                {
                    struct vec4 color, tmp;

                    format_to_vec4(src_format, src_ptr, &color);
                    if (src_format->to_rgba)
                        src_format->to_rgba(&color, &tmp, palette);
                    else
                        tmp = color;

                    if (ck_format)
                    {
                        std::uint32_t ck_pixel;

                        format_from_vec4(ck_format, &tmp, (std::uint8_t *)&ck_pixel);
                        if (ck_pixel == color_key)
                            tmp.w = 0.0f;
                    }

                    if (dst_format->from_rgba)
                        dst_format->from_rgba(&tmp, &color);
                    else
                        color = tmp;

                    format_from_vec4(dst_format, &color, dst_ptr);
                }

                dst_ptr += dst_format->bytes_per_pixel;
            }
        }
    }
}

////////////////////////////////////////////////////////////////////////////////////////////////////////////////////////

/************************************************************
 * WINE doc:
 * ImageLoadSurfaceFromMemory
 *
 * Loads data from a given memory chunk into a surface,
 * applying any of the specified filters.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcMemory   [I] pointer to the source data
 *   SrcFormat    [I] format of the source pixel data
 *   SrcPitch     [I] number of bytes in a row
 *   pSrcPalette  [I] palette used in the source image
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on stretching
 *   Colorkey     [I] colorkey
 *
 * RETURNS
 *   Success: result::ok, if we successfully load the pixel data into our surface or
 *                    if pSrcMemory is NULL but the other parameters are valid
 *   Failure: result::invalidCall, if pDestSurface, SrcPitch or pSrcRect is NULL or
 *                                if SrcFormat is an invalid format (other than AssetFormat::Unknown) or
 *                                if DestRect is invalid
 *            imageInvalidData, if we fail to lock pDestSurface
 *            result::failure, if SrcFormat is AssetFormat::Unknown or the dimensions of pSrcRect are invalid
 *
 * NOTES
 *   pSrcRect specifies the dimensions of the source data;
 *   negative values for pSrcRect are allowed as we're only looking at the width and height anyway.
 *
 */
} // namespace

Result ImageLoadSurfaceFromMemory(Surface *dst_surface,
                                         const PaletteEntry *dst_palette, const Rect *dst_rect, const void *src_memory,
                                         AssetFormat src_format, std::uint32_t src_pitch, const PaletteEntry *src_palette, const Rect *src_rect,
                                         std::uint32_t filter, PackedColor color_key)
{
    const struct pixel_format_desc *srcformatdesc, *destformatdesc;
    struct volume src_size, dst_size, dst_size_aligned;
    Rect dst_rect_temp, dst_rect_aligned;
    Surface *surface;
    SurfaceDesc surfdesc;
    LockedRect lockrect;
    Result hr;

    if (!dst_surface || !src_memory || !src_rect)
    {
        return result::invalidCall;
    }
    if (src_format == AssetFormat::Unknown
        || src_rect->left >= src_rect->right
        || src_rect->top >= src_rect->bottom)
    {
        return result::failure;
    }

    srcformatdesc = get_format_info(src_format);
    if (srcformatdesc->type == FORMAT_UNKNOWN)
    {
        WARN("Unsupported format %#x.\n", static_cast<std::uint32_t>(src_format));
        return result::unsupported;
    }

    src_size.width = src_rect->right - src_rect->left;
    src_size.height = src_rect->bottom - src_rect->top;
    src_size.depth = 1;

    dst_surface->GetDesc(&surfdesc);
    destformatdesc = get_format_info(surfdesc.Format);
    if (!dst_rect)
    {
        dst_rect = &dst_rect_temp;
        dst_rect_temp.left = 0;
        dst_rect_temp.top = 0;
        dst_rect_temp.right = surfdesc.Width;
        dst_rect_temp.bottom = surfdesc.Height;
    }
    else
    {
        if (dst_rect->left > dst_rect->right || dst_rect->right > surfdesc.Width
            || dst_rect->top > dst_rect->bottom || dst_rect->bottom > surfdesc.Height
            || dst_rect->left < 0 || dst_rect->top < 0)
        {
            WARN("Invalid dst_rect specified.\n");
            return result::invalidCall;
        }
        if (dst_rect->left == dst_rect->right || dst_rect->top == dst_rect->bottom)
        {
            WARN("Empty dst_rect specified.\n");
            return result::ok;
        }
    }

    dst_rect_aligned = *dst_rect;
    if (dst_rect_aligned.left & (destformatdesc->block_width - 1))
        dst_rect_aligned.left = dst_rect_aligned.left & ~(destformatdesc->block_width - 1);
    if (dst_rect_aligned.top & (destformatdesc->block_height - 1))
        dst_rect_aligned.top = dst_rect_aligned.top & ~(destformatdesc->block_height - 1);
    if (dst_rect_aligned.right & (destformatdesc->block_width - 1) && dst_rect_aligned.right != surfdesc.Width)
        dst_rect_aligned.right = std::min((dst_rect_aligned.right + destformatdesc->block_width - 1)
                                     & ~(destformatdesc->block_width - 1), surfdesc.Width);
    if (dst_rect_aligned.bottom & (destformatdesc->block_height - 1) && dst_rect_aligned.bottom != surfdesc.Height)
        dst_rect_aligned.bottom = std::min((dst_rect_aligned.bottom + destformatdesc->block_height - 1)
                                      & ~(destformatdesc->block_height - 1), surfdesc.Height);

    dst_size.width = dst_rect->right - dst_rect->left;
    dst_size.height = dst_rect->bottom - dst_rect->top;
    dst_size.depth = 1;
    dst_size_aligned.width = dst_rect_aligned.right - dst_rect_aligned.left;
    dst_size_aligned.height = dst_rect_aligned.bottom - dst_rect_aligned.top;
    dst_size_aligned.depth = 1;

    if (filter == ImageDefault)
        filter = image_filter::triangle | image_filter::dither;

    if (failed(hr = lock_surface(dst_surface, &dst_rect_aligned, &lockrect, &surface, true)))
        return hr;

    src_memory = (std::uint8_t *)src_memory + src_rect->top / srcformatdesc->block_height * src_pitch
                 + src_rect->left / srcformatdesc->block_width * srcformatdesc->block_byte_count;

    if (src_format == surfdesc.Format
        && dst_size.width == src_size.width
        && dst_size.height == src_size.height
        && color_key == 0
        && !(src_rect->left & (srcformatdesc->block_width - 1))
        && !(src_rect->top & (srcformatdesc->block_height - 1))
        && !(dst_rect->left & (destformatdesc->block_width - 1))
        && !(dst_rect->top & (destformatdesc->block_height - 1)))
    {
        copy_pixels(static_cast<const std::uint8_t*>(src_memory), src_pitch, 0, static_cast<std::uint8_t*>(lockrect.pBits),
                    lockrect.Pitch, 0, &src_size, srcformatdesc);
    }
    else /* Stretching or format conversion. */
    {
        const struct pixel_format_desc *dst_format;
        unsigned int dst_pitch;
        std::uint8_t *dst_mem;

        if (!is_conversion_from_supported(srcformatdesc)
            || !is_conversion_to_supported(destformatdesc))
        {
            WARN("Unsupported format conversion %#x -> %#x.\n",
                 static_cast<std::uint32_t>(src_format), static_cast<std::uint32_t>(surfdesc.Format));
            unlock_surface(dst_surface, &dst_rect_aligned, surface, false);
            return result::unsupported;
        }

        if (srcformatdesc->type == FORMAT_DXT || destformatdesc->type == FORMAT_DXT)
        {
            WARN("Src/Dst FORMAT_DXT unsupported\n");
            return result::unsupported;
        }
        else
        {
            dst_mem = static_cast<std::uint8_t*>(lockrect.pBits);
            dst_pitch = lockrect.Pitch;
            dst_format = destformatdesc;
        }

        if ((filter & 0xf) == image_filter::none || destformatdesc->format == AssetFormat::Q8W8V8U8)
        {
            convert_argb_pixels(static_cast<const std::uint8_t*>(src_memory), src_pitch, 0, &src_size, srcformatdesc,
                                dst_mem, dst_pitch, 0, &dst_size, dst_format, color_key, src_palette);
        }
        else /* if ((filter & 0xf) == image_filter::point) */
        {
            if ((filter & 0xf) != image_filter::point) {
                WARN("Unhandled filter %#x.\n", filter);
            }

            /* Always apply a point filter until image_filter::linear,
             * image_filter::triangle and image_filter::box are implemented. */
            point_filter_argb_pixels(static_cast<const std::uint8_t*>(src_memory), src_pitch, 0, &src_size, srcformatdesc,
                                     dst_mem, dst_pitch, 0, &dst_size, dst_format, color_key, src_palette);
        }
    }

    return unlock_surface(dst_surface, &dst_rect_aligned, surface, true);
}

/************************************************************
 * WINE doc:
 * ImageLoadSurfaceFromSurface
 *
 * Copies the contents from one surface to another, performing any required
 * format conversion, resizing or filtering.
 *
 * PARAMS
 *   pDestSurface [I] pointer to the destination surface
 *   pDestPalette [I] palette to use
 *   pDestRect    [I] to be filled area of the surface
 *   pSrcSurface  [I] pointer to the source surface
 *   pSrcPalette  [I] palette used for the source surface
 *   pSrcRect     [I] area of the source data to load
 *   dwFilter     [I] filter to apply on resizing
 *   Colorkey     [I] any ARGB value or 0 to disable color-keying
 *
 * RETURNS
 *   Success: result::ok
 *   Failure: result::invalidCall, if pDestSurface or pSrcSurface is NULL
 *            imageInvalidData, if one of the surfaces is not lockable
 *
 */
Result ImageLoadSurfaceFromSurface(Surface *dst_surface,
        const PaletteEntry *dst_palette, const Rect *dst_rect, Surface *src_surface,
        const PaletteEntry *src_palette, const Rect *src_rect, std::uint32_t filter, PackedColor color_key)
{
    const struct pixel_format_desc *src_format_desc, *dst_format_desc;
    SurfaceDesc src_desc, dst_desc;
    struct volume src_size, dst_size;
    Surface *temp_surface;
    TextureFilter native_filter = TextureFilterNone;
    bool supported_filter = true;
    Device *device;
    LockedRect lock;
    Rect dst_rect_temp;
    Result hr;
    Rect s;

    /*
    TRACE("dst_surface %p, dst_palette %p, dst_rect %s, src_surface %p, "
            "src_palette %p, src_rect %s, filter %#x, color_key 0x%08x.\n",
            dst_surface, dst_palette, wine_dbgstr_rect(dst_rect), src_surface,
            src_palette, wine_dbgstr_rect(src_rect), filter, color_key);
     */

    if (!dst_surface || !src_surface)
        return result::invalidCall;

    src_surface->GetDesc(&src_desc);
    src_format_desc = get_format_info(src_desc.Format);
    if (!src_rect)
    {
        SetRect(&s, 0, 0, src_desc.Width, src_desc.Height);
        src_rect = &s;
    }
    else if (src_rect->left == src_rect->right || src_rect->top == src_rect->bottom)
    {
        WARN("Empty src_rect specified.\n");
        return filter == image_filter::none ? result::ok : result::failure;
    }
    else if (src_rect->left > src_rect->right || src_rect->right > src_desc.Width
            || src_rect->left < 0 || src_rect->left > src_desc.Width
            || src_rect->top > src_rect->bottom || src_rect->bottom > src_desc.Height
            || src_rect->top < 0 || src_rect->top > src_desc.Height)
    {
        WARN("Invalid src_rect specified.\n");
        return result::invalidCall;
    }

    src_size.width = src_rect->right - src_rect->left;
    src_size.height = src_rect->bottom - src_rect->top;
    src_size.depth = 1;

    dst_surface->GetDesc(&dst_desc);
    dst_format_desc = get_format_info(dst_desc.Format);
    if (!dst_rect)
    {
        SetRect(&dst_rect_temp, 0, 0, dst_desc.Width, dst_desc.Height);
        dst_rect = &dst_rect_temp;
    }
    else if (dst_rect->left == dst_rect->right || dst_rect->top == dst_rect->bottom)
    {
        WARN("Empty dst_rect specified.\n");
        return filter == image_filter::none ? result::ok : result::failure;
    }
    else if (dst_rect->left > dst_rect->right || dst_rect->right > dst_desc.Width
            || dst_rect->left < 0 || dst_rect->left > dst_desc.Width
            || dst_rect->top > dst_rect->bottom || dst_rect->bottom > dst_desc.Height
            || dst_rect->top < 0 || dst_rect->top > dst_desc.Height)
    {
        WARN("Invalid dst_rect specified.\n");
        return result::invalidCall;
    }

    dst_size.width = dst_rect->right - dst_rect->left;
    dst_size.height = dst_rect->bottom - dst_rect->top;
    dst_size.depth = 1;

    if (!dst_palette && !src_palette && !color_key)
    {
        if (src_desc.Format == dst_desc.Format
                && dst_size.width == src_size.width
                && dst_size.height == src_size.height
                && color_key == 0
                && !(src_rect->left & (src_format_desc->block_width - 1))
                && !(src_rect->top & (src_format_desc->block_height - 1))
                && !(dst_rect->left & (dst_format_desc->block_width - 1))
                && !(dst_rect->top & (dst_format_desc->block_height - 1)))
        {
            native_filter = TextureFilterNone;
        }
        else
        {
            switch (filter)
            {
                case image_filter::none:
                    native_filter = TextureFilterNone;
                    break;

                case image_filter::point:
                    native_filter = TextureFilterPoint;
                    break;

                case image_filter::linear:
                    native_filter = TextureFilterLinear;
                    break;

                default:
                    supported_filter = false;
                    break;
            }
        }

        if (supported_filter)
        {
            src_surface->GetDevice(&device);
            hr = device->StretchRect(src_surface, src_rect, dst_surface, dst_rect, native_filter);
            device->Release();
            if (succeeded(hr))
                return result::ok;
        }
    }

    if (failed(lock_surface(src_surface, NULL, &lock, &temp_surface, false)))
        return imageInvalidData;

    hr = ImageLoadSurfaceFromMemory(dst_surface, dst_palette, dst_rect, lock.pBits,
            src_desc.Format, lock.Pitch, src_palette, src_rect, filter, color_key);

    if (failed(unlock_surface(src_surface, NULL, temp_surface, false)))
        return imageInvalidData;

    return hr;
}


} // namespace storm_render
