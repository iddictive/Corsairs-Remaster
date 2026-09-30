#include <SDL.h>
#include <SDL_vulkan.h>
#include <d3d9.h>

#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

namespace {

int fail(const char *stage, long code = 0)
{
    if (code) {
        std::fprintf(stderr, "FAIL %s HRESULT=0x%08lx SDL=%s\n", stage,
                     static_cast<unsigned long>(static_cast<std::uint32_t>(code)),
                     SDL_GetError());
    } else {
        std::fprintf(stderr, "FAIL %s SDL=%s\n", stage, SDL_GetError());
    }
    return 1;
}

std::vector<std::uint8_t> readFile(const char *path)
{
    std::ifstream stream(path, std::ios::binary | std::ios::ate);
    if (!stream)
        return {};
    const auto size = stream.tellg();
    if (size <= 0)
        return {};
    std::vector<std::uint8_t> bytes(static_cast<std::size_t>(size));
    stream.seekg(0);
    stream.read(reinterpret_cast<char *>(bytes.data()), size);
    return stream ? bytes : std::vector<std::uint8_t>{};
}

} // namespace

int main(int argc, char **argv)
{
    if (argc < 2) {
        std::fprintf(stderr,
                     "usage: %s /absolute/path/to/libMoltenVK.dylib "
                     "[shader.vso|shader.pso ...]\n",
                     argv[0]);
        return 2;
    }

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0)
        return fail("SDL_Init");

    const bool preloadMoltenVk = std::string(argv[1]) != "-";
    if (preloadMoltenVk && SDL_Vulkan_LoadLibrary(argv[1]) != 0) {
        SDL_Quit();
        return fail("SDL_Vulkan_LoadLibrary");
    }

    SDL_Window *window = SDL_CreateWindow("DXVK Native D3D9 probe",
                                          SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                                          640, 360,
                                          SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
    if (!window) {
        if (preloadMoltenVk)
            SDL_Vulkan_UnloadLibrary();
        SDL_Quit();
        return fail("SDL_CreateWindow");
    }

    IDirect3D9 *d3d = Direct3DCreate9(D3D_SDK_VERSION);
    if (!d3d) {
        SDL_DestroyWindow(window);
        if (preloadMoltenVk)
            SDL_Vulkan_UnloadLibrary();
        SDL_Quit();
        return fail("Direct3DCreate9");
    }

    const UINT adapterCount = d3d->GetAdapterCount();
    std::printf("Direct3DCreate9 OK adapters=%u\n", adapterCount);
    if (!adapterCount) {
        d3d->Release();
        SDL_DestroyWindow(window);
        if (preloadMoltenVk)
            SDL_Vulkan_UnloadLibrary();
        SDL_Quit();
        return fail("GetAdapterCount");
    }

    D3DADAPTER_IDENTIFIER9 identifier = {};
    HRESULT hr = d3d->GetAdapterIdentifier(D3DADAPTER_DEFAULT, 0, &identifier);
    if (FAILED(hr)) {
        d3d->Release();
        SDL_DestroyWindow(window);
        if (preloadMoltenVk)
            SDL_Vulkan_UnloadLibrary();
        SDL_Quit();
        return fail("GetAdapterIdentifier", hr);
    }
    std::printf("adapter=%s driver=%s vendor=0x%04x device=0x%04x\n",
                identifier.Description, identifier.Driver,
                identifier.VendorId, identifier.DeviceId);

    D3DPRESENT_PARAMETERS params = {};
    params.BackBufferWidth = 640;
    params.BackBufferHeight = 360;
    params.BackBufferFormat = D3DFMT_UNKNOWN;
    params.BackBufferCount = 1;
    params.MultiSampleType = D3DMULTISAMPLE_NONE;
    params.SwapEffect = D3DSWAPEFFECT_DISCARD;
    params.hDeviceWindow = reinterpret_cast<HWND>(window);
    params.Windowed = TRUE;
    params.EnableAutoDepthStencil = FALSE;
    params.PresentationInterval = D3DPRESENT_INTERVAL_IMMEDIATE;

    IDirect3DDevice9 *device = nullptr;
    hr = d3d->CreateDevice(D3DADAPTER_DEFAULT, D3DDEVTYPE_HAL,
                           reinterpret_cast<HWND>(window),
                           D3DCREATE_HARDWARE_VERTEXPROCESSING,
                           &params, &device);
    if (FAILED(hr)) {
        d3d->Release();
        SDL_DestroyWindow(window);
        if (preloadMoltenVk)
            SDL_Vulkan_UnloadLibrary();
        SDL_Quit();
        return fail("CreateDevice", hr);
    }
    std::puts("CreateDevice OK");

    unsigned shaderCount = 0;
    for (int i = 2; i < argc; ++i) {
        const std::string path = argv[i];
        const auto bytes = readFile(argv[i]);
        if (bytes.empty() || bytes.size() % sizeof(DWORD) != 0) {
            std::fprintf(stderr, "FAIL read shader path=%s bytes=%zu\n",
                         argv[i], bytes.size());
            device->Release();
            d3d->Release();
            SDL_DestroyWindow(window);
            if (preloadMoltenVk)
                SDL_Vulkan_UnloadLibrary();
            SDL_Quit();
            return 1;
        }

        if (path.size() >= 4 && path.compare(path.size() - 4, 4, ".vso") == 0) {
            IDirect3DVertexShader9 *shader = nullptr;
            hr = device->CreateVertexShader(
                reinterpret_cast<const DWORD *>(bytes.data()), &shader);
            if (SUCCEEDED(hr))
                shader->Release();
        } else if (path.size() >= 4 &&
                   path.compare(path.size() - 4, 4, ".pso") == 0) {
            IDirect3DPixelShader9 *shader = nullptr;
            hr = device->CreatePixelShader(
                reinterpret_cast<const DWORD *>(bytes.data()), &shader);
            if (SUCCEEDED(hr))
                shader->Release();
        } else {
            std::fprintf(stderr, "FAIL unknown shader extension path=%s\n", argv[i]);
            hr = D3DERR_INVALIDCALL;
        }

        if (FAILED(hr)) {
            std::fprintf(stderr, "FAIL CreateShader path=%s HRESULT=0x%08lx\n",
                         argv[i],
                         static_cast<unsigned long>(static_cast<std::uint32_t>(hr)));
            device->Release();
            d3d->Release();
            SDL_DestroyWindow(window);
            if (preloadMoltenVk)
                SDL_Vulkan_UnloadLibrary();
            SDL_Quit();
            return 1;
        }
        ++shaderCount;
        std::printf("CreateShader OK path=%s bytes=%zu\n", argv[i], bytes.size());
    }
    std::printf("Original shaders accepted=%u\n", shaderCount);

    hr = device->Clear(0, nullptr, D3DCLEAR_TARGET,
                       D3DCOLOR_XRGB(18, 52, 86), 1.0f, 0);
    if (SUCCEEDED(hr))
        hr = device->BeginScene();
    if (SUCCEEDED(hr))
        hr = device->EndScene();
    if (SUCCEEDED(hr))
        hr = device->Present(nullptr, nullptr, nullptr, nullptr);

    if (FAILED(hr)) {
        device->Release();
        d3d->Release();
        SDL_DestroyWindow(window);
        if (preloadMoltenVk)
            SDL_Vulkan_UnloadLibrary();
        SDL_Quit();
        return fail("Clear/BeginScene/EndScene/Present", hr);
    }
    std::puts("Clear+Present OK");

    device->Release();
    d3d->Release();
    SDL_DestroyWindow(window);
    if (preloadMoltenVk)
        SDL_Vulkan_UnloadLibrary();
    SDL_Quit();
    return 0;
}
