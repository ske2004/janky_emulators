#include <stdio.h>
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include "nrom.c"
#include <SDL3/SDL.h>

uint32_t pallete[] = {
    0x626262ff, 0x001fb2ff, 0x2404c8ff, 0x5200b2ff, 0x730076ff, 0x800024ff, 0x730b00ff, 0x522800ff, 0x244400ff, 0x005700ff, 0x005c00ff, 0x005324ff, 0x003c76ff, 0x000000ff, 0x000000ff, 0x000000ff, 0xabababff, 0x0d57ffff, 0x4b30ffff, 0x8a13ffff, 0xbc08d6ff, 0xd21269ff, 0xc72e00ff, 0x9d5400ff, 0x607b00ff, 0x209800ff, 0x00a300ff, 0x009942ff, 0x007db4ff, 0x000000ff, 0x000000ff, 0x000000ff, 0xffffffff, 0x53aeffff, 0x9085ffff, 0xd365ffff, 0xff57ffff, 0xff5dcfff, 0xff7757ff, 0xfa9e00ff, 0xbdc700ff, 0x7ae700ff, 0x43f611ff, 0x26ef7eff, 0x2cd5f6ff, 0x4e4e4eff, 0x000000ff, 0x000000ff, 0xffffffff, 0xb6e1ffff, 0xced1ffff, 0xe9c3ffff, 0xffbcffff, 0xffbdf4ff, 0xffc6c3ff, 0xffd59aff, 0xe9e681ff, 0xcef481ff, 0xb6fb9aff, 0xa9fac3ff, 0xa9f0f4ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff
};

int main(int argc, char* argv[]) {
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <NES ROM>\n", argv[0]);
        return 1;
    }
    FILE *fp = fopen(argv[1], "rb");
    if (!fp)
    {
        perror(argv[1]);
        return 1;
    }
    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t *rom = malloc(fsize);
    fread(rom, 1, fsize, fp);
    fclose(fp);

    struct nrom nrom;
    if (nrom_load(rom, &nrom))
    {
        fprintf(stderr, "Invalid ROM file\n");
        return 1;
    }

    SDL_Window *window;                    // Declare a pointer
    SDL_Renderer *renderer;
    bool done = false;

    SDL_Init(SDL_INIT_VIDEO);              // Initialize SDL3

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "neske",                           // window title
        256*3,                               // width, in pixels
        240*3,                               // height, in pixels
        SDL_WINDOW_OPENGL // flags - see below
    );

    if (window == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, NULL);

    if (renderer == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create renderer: %s\n", SDL_GetError());
        return 1;
    }
    
    uint32_t texture[240*256];
    SDL_Surface *surface = SDL_CreateSurface(256, 240, SDL_PIXELFORMAT_RGBA8888);

    while (!done) {
        struct nrom_frame_result result = nrom_frame(&nrom);

        for (int i = 0; i < 240*256; i++)
        {
            texture[i] = pallete[result.screen[i]];
        }

        memcpy(surface->pixels, texture, 240*256*4);

        SDL_Texture *sdltexture = SDL_CreateTextureFromSurface(renderer, surface);
        SDL_SetTextureScaleMode(sdltexture, SDL_SCALEMODE_NEAREST);


        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (event.type == SDL_EVENT_QUIT) {
                done = true;
            }
        }   

        SDL_FRect src = {0, 0, 256, 240};
        SDL_FRect dst = {0, 0, 256*3, 240*3};
        SDL_RenderTexture(renderer, sdltexture, &src, &dst);
        SDL_RenderPresent(renderer);
        SDL_DestroyTexture(sdltexture);
    }

    // Close and destroy the window
    SDL_DestroyWindow(window);

    // Clean up
    SDL_Quit();
    return 0;
}