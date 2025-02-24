#include <stdio.h>
#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_oldnames.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include <SDL3/SDL.h>
#include "font8x8.h"
#include "neske.h"

uint32_t pallete[] = {
    0x626262ff, 0x001fb2ff, 0x2404c8ff, 0x5200b2ff, 0x730076ff, 0x800024ff, 0x730b00ff, 0x522800ff, 0x244400ff, 0x005700ff, 0x005c00ff, 0x005324ff, 0x003c76ff, 0x000000ff, 0x000000ff, 0x000000ff, 0xabababff, 0x0d57ffff, 0x4b30ffff, 0x8a13ffff, 0xbc08d6ff, 0xd21269ff, 0xc72e00ff, 0x9d5400ff, 0x607b00ff, 0x209800ff, 0x00a300ff, 0x009942ff, 0x007db4ff, 0x000000ff, 0x000000ff, 0x000000ff, 0xffffffff, 0x53aeffff, 0x9085ffff, 0xd365ffff, 0xff57ffff, 0xff5dcfff, 0xff7757ff, 0xfa9e00ff, 0xbdc700ff, 0x7ae700ff, 0x43f611ff, 0x26ef7eff, 0x2cd5f6ff, 0x4e4e4eff, 0x000000ff, 0x000000ff, 0xffffffff, 0xb6e1ffff, 0xced1ffff, 0xe9c3ffff, 0xffbcffff, 0xffbdf4ff, 0xffc6c3ff, 0xffd59aff, 0xe9e681ff, 0xcef481ff, 0xb6fb9aff, 0xa9fac3ff, 0xa9f0f4ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff
};

SDL_Texture *mk_font_texture(SDL_Renderer *renderer)
{
    const int width = 128*8;

    SDL_Surface *surface = SDL_CreateSurface(width, 8, SDL_PIXELFORMAT_RGBA8888);

    for (int c = 0; c < 128; c++)
    {
        for (int x = 0; x < 8; x++)
        {
            for (int y = 0; y < 8; y++)
            {
                ((uint32_t*)surface->pixels)[y*width+x+c*8] = font8x8_basic[c][y] & (1<<x) ? 0xFFFFFFFF : 0xFFFFFF00;
            }
        }
    }

    SDL_Texture *texture = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_SetTextureScaleMode(texture, SDL_SCALEMODE_NEAREST);

    SDL_DestroySurface(surface);

    return texture;
}

void draw_char(SDL_Renderer *renderer, SDL_Texture *font_texture, int chr, SDL_Point xy, int scale)
{
    SDL_FRect src = {chr*8, 0, 8, 8};
    SDL_FRect dst = {xy.x, xy.y, scale*8, scale*8};
    SDL_RenderTexture(renderer, font_texture, &src, &dst);
}

void draw_string(SDL_Renderer *renderer, SDL_Texture *font_texture, const char *string, SDL_Point xy, int scale)
{
    for (int i = 0; string[i]; i++)
    {
        draw_char(renderer, font_texture, string[i], (SDL_Point){xy.x+i*8*scale, xy.y}, scale);
    }
}

SDL_Point size_string(const char *string, int scale)
{
    return (SDL_Point){strlen(string) * 8 * scale, 8};
}

struct ui_draw
{
    SDL_Renderer *renderer;
    SDL_Texture *font;
};

struct ui_draw ui_draw_mk(SDL_Renderer *renderer)
{
    struct ui_draw result = { 0 };
    result.renderer = renderer;
    result.font = mk_font_texture(renderer);
    return result;
}

void _ui_set_draw_color_rgba(struct ui_draw *it, uint32_t rgba)
{
    SDL_SetRenderDrawColor(it->renderer, rgba>>24, rgba>>16, rgba>>8, rgba);
}

void ui_draw_rect(struct ui_draw *it, SDL_FRect rect)
{
    _ui_set_draw_color_rgba(it, 0x2222AAFF);
    SDL_RenderRect(it->renderer, &rect);
}

void ui_draw_text(struct ui_draw *it, const char *text, SDL_Point xy, int scale)
{
    draw_string(it->renderer, it->font, text, xy, scale);
}

void ui_draw_item(struct ui_draw *it, SDL_FRect rect, bool hover)
{
    _ui_set_draw_color_rgba(it, 0x2222AAFF);
    SDL_RenderFillRect(it->renderer, &rect);
    if (hover) {
        _ui_set_draw_color_rgba(it, 0xFFFFFFFF);
    } else {
        _ui_set_draw_color_rgba(it, 0x77AAFFFF);
    }
    SDL_RenderRect(it->renderer, &rect);
}

void audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    printf("Yo\n");
}

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
        256*4,                               // width, in pixels
        240*3,                               // height, in pixels
        SDL_WINDOW_OPENGL  // flags - see below
    );

    if (window == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    SDL_SetRenderVSync(renderer, 1);

    if (renderer == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create renderer: %s\n", SDL_GetError());
        return 1;
    }
    
    uint32_t texture[240*256];
    SDL_Surface *surface = SDL_CreateSurface(256, 240, SDL_PIXELFORMAT_RGBA8888);
    struct ui_draw uidraw = ui_draw_mk(renderer);
    struct imap *imap = imap_mk();

    struct ricoh_mem_interface mem = nrom_get_memory_interface(&nrom);
    imap_populate(imap, &nrom.decoder, &mem, nrom_get_vector(&nrom, VEC_NMI));
    imap_populate(imap, &nrom.decoder, &mem, nrom_get_vector(&nrom, VEC_RESET));
    imap_populate(imap, &nrom.decoder, &mem, nrom_get_vector(&nrom, VEC_IRQ));

    SDL_SetRenderScale(renderer, 2, 2);

    struct controller_state controller = { 0 };


    SDL_AudioSpec audio_in = { 0 };

    audio_in.channels = 1;
    audio_in.format = SDL_AUDIO_U8;
    audio_in.freq = 22050;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);
    SDL_AudioStream *audio_device_stream = SDL_OpenAudioDeviceStream(audio_device, &audio_in, audio_callback, NULL);

    SDL_ResumeAudioDevice(audio_device);
    SDL_ResumeAudioStreamDevice(audio_device_stream);

    while (!done) {
        uint8_t buf[1024] = { 0 };
        for (int i = 0; i < 1024; i++)
        {
            buf[i] = (i/64%2) ? 255 : 0;
        }

        SDL_PutAudioStreamData(audio_device_stream, buf, 1024);
        SDL_FlushAudioStream(audio_device_stream);
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
            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                done = true;
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                if (event.key.key == SDLK_X) controller.btns[BTN_A] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_Z) controller.btns[BTN_B] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_RETURN) controller.btns[BTN_START] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_TAB) controller.btns[BTN_SELECT] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_UP) controller.btns[BTN_UP] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_DOWN) controller.btns[BTN_DOWN] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_LEFT) controller.btns[BTN_LEFT] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_RIGHT) controller.btns[BTN_RIGHT] = event.type == SDL_EVENT_KEY_DOWN;
                nrom_update_controller(&nrom, controller);
                break;
            }
        }

        SDL_FRect screen = {0, 0, 512, 360};
        SDL_SetRenderDrawColor(renderer, 0, 0, 127, 255);
        SDL_RenderFillRect(renderer, &screen);

        SDL_FRect srcf = {0, 0, 128*8, 8};
        SDL_FRect src = {0, 0, 256, 240};
        SDL_FRect dst = {0, 0, 256, 240};
        SDL_RenderTexture(renderer, sdltexture, &src, &dst);

        // for (int x = 0; x < 8; x++)
        // {
        //     for (int y = 0; y < 8; y++)
        //     {
        //         ui_draw_rect(&uidraw, (SDL_FRect){x*32, y*32, 32, 32});
        //     }
        // }

        // ui_draw_item(&uidraw, (SDL_FRect){10, 10, 200, 200}, true);
        struct print_instr *instrs[10];
        imap_list_range(imap, nrom.cpu.pc, instrs, -5, 4);

        ui_draw_text(&uidraw,  "CPU --------------", (SDL_Point){256, 0}, 1);

        for (int i = 0; i < 10; i++)
        {
            if (instrs[i]) 
            {
                char s[128];
                const char *lbl_name = "";
                if (instrs[i]->at == nrom_get_vector(&nrom, VEC_NMI)) {
                    lbl_name = "NMI";
                }
                if (instrs[i]->at == nrom_get_vector(&nrom, VEC_RESET)) {
                    lbl_name = "RST";
                }
                if (instrs[i]->at == nrom_get_vector(&nrom, VEC_IRQ)) {
                    lbl_name = "IRQ";
                }

                sprintf(s, "%3s %c %4X %s", lbl_name, i == 5 ? '>' : ' ', instrs[i]->at, instrs[i]->value);
                ui_draw_text(&uidraw, s, (SDL_Point){256, 10+i*8}, 1);
            }
        }

        SDL_RenderPresent(renderer);
        
        SDL_DestroyTexture(sdltexture);

    }

    // Close and destroy the window
    SDL_DestroyWindow(window);

    // Clean up
    SDL_Quit();
    return 0;
}