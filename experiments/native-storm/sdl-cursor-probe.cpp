#include <SDL.h>

#include <cstdio>
#include <cstring>

int main(int argc, char **argv)
{
    const bool hideBeforeShow = argc > 1 && std::strcmp(argv[1], "before") == 0;
    const bool customCursor = argc > 1 && std::strcmp(argv[1], "custom") == 0;
    const bool relativeMode = argc > 1 && std::strcmp(argv[1], "relative") == 0;

    if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_EVENTS) != 0) {
        std::fprintf(stderr, "FAIL SDL_Init: %s\n", SDL_GetError());
        return 1;
    }

    SDL_Window *window = SDL_CreateWindow("SDL invisible cursor probe",
                                          SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
                                          640, 360, SDL_WINDOW_VULKAN | SDL_WINDOW_HIDDEN);
    if (!window) {
        std::fprintf(stderr, "FAIL SDL_CreateWindow: %s\n", SDL_GetError());
        SDL_Quit();
        return 1;
    }

    SDL_Surface *cursorSurface = nullptr;
    SDL_Cursor *transparentCursor = nullptr;
    if (customCursor) {
        cursorSurface = SDL_CreateRGBSurfaceWithFormat(0, 16, 16, 32, SDL_PIXELFORMAT_RGBA32);
        if (cursorSurface)
            SDL_FillRect(cursorSurface, nullptr, SDL_MapRGBA(cursorSurface->format, 0, 0, 0, 0));
        if (cursorSurface)
            transparentCursor = SDL_CreateColorCursor(cursorSurface, 0, 0);
        if (!transparentCursor) {
            std::fprintf(stderr, "FAIL SDL_CreateColorCursor: %s\n", SDL_GetError());
            SDL_FreeSurface(cursorSurface);
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
        SDL_SetCursor(transparentCursor);
        SDL_ShowCursor(SDL_ENABLE);
    } else if (hideBeforeShow && SDL_ShowCursor(SDL_DISABLE) < 0) {
        std::fprintf(stderr, "FAIL SDL_ShowCursor(before): %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    SDL_ShowWindow(window);
    SDL_RaiseWindow(window);

    if (relativeMode) {
        if (SDL_SetRelativeMouseMode(SDL_TRUE) != 0) {
            std::fprintf(stderr, "FAIL SDL_SetRelativeMouseMode: %s\n", SDL_GetError());
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }
    } else if (!customCursor && !hideBeforeShow && SDL_ShowCursor(SDL_DISABLE) < 0) {
        std::fprintf(stderr, "FAIL SDL_ShowCursor(after): %s\n", SDL_GetError());
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 1;
    }

    for (int frame = 0; frame < 300; ++frame) {
        SDL_Event event;
        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_QUIT)
                frame = 300;
        }
        SDL_Delay(10);
    }

    if (relativeMode)
        SDL_SetRelativeMouseMode(SDL_FALSE);
    SDL_ShowCursor(SDL_ENABLE);
    if (transparentCursor) {
        SDL_SetCursor(SDL_GetDefaultCursor());
        SDL_FreeCursor(transparentCursor);
    }
    SDL_FreeSurface(cursorSurface);
    SDL_DestroyWindow(window);
    SDL_Quit();
    std::printf("PASS cursor=%s event-pump=300 SDL=%u.%u.%u\n",
                customCursor ? "transparent-color" :
                    (relativeMode ? "relative-mode" :
                        (hideBeforeShow ? "hidden-before-show" : "hidden-after-show")),
                SDL_MAJOR_VERSION, SDL_MINOR_VERSION, SDL_PATCHLEVEL);
    return 0;
}
