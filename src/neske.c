#include <stdio.h>
#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_image.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_messagebox.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_mutex.h"
#include "SDL3/SDL_oldnames.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include <SDL3/SDL.h>
#include "SDL3/SDL_video.h"
#include "font8x8.h"
#include "neske.h"

uint32_t pallete[] = {
    0x626262ff, 0x001fb2ff, 0x2404c8ff, 0x5200b2ff, 0x730076ff, 
    0x800024ff, 0x730b00ff, 0x522800ff, 0x244400ff, 0x005700ff, 
    0x005c00ff, 0x005324ff, 0x003c76ff, 0x000000ff, 0x000000ff, 
    0x000000ff, 0xabababff, 0x0d57ffff, 0x4b30ffff, 0x8a13ffff,
    0xbc08d6ff, 0xd21269ff, 0xc72e00ff, 0x9d5400ff, 0x607b00ff,
    0x209800ff, 0x00a300ff, 0x009942ff, 0x007db4ff, 0x000000ff,
    0x000000ff, 0x000000ff, 0xffffffff, 0x53aeffff, 0x9085ffff,
    0xd365ffff, 0xff57ffff, 0xff5dcfff, 0xff7757ff, 0xfa9e00ff,
    0xbdc700ff, 0x7ae700ff, 0x43f611ff, 0x26ef7eff, 0x2cd5f6ff,
    0x4e4e4eff, 0x000000ff, 0x000000ff, 0xffffffff, 0xb6e1ffff,
    0xced1ffff, 0xe9c3ffff, 0xffbcffff, 0xffbdf4ff, 0xffc6c3ff,
    0xffd59aff, 0xe9e681ff, 0xcef481ff, 0xb6fb9aff, 0xa9fac3ff,
    0xa9f0f4ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff
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

void sdl_mux_lock( void *mux )
{
    SDL_LockMutex(mux);
}

void sdl_mux_unlock( void *mux )
{
    SDL_UnlockMutex(mux);
}

struct mux_api sdl_mux_make()
{
    return (struct mux_api) {
        SDL_CreateMutex(),
        sdl_mux_lock,
        sdl_mux_unlock,
    };
}

SDL_HitTestResult hit_test(SDL_Window* win, const SDL_Point* pos, void *userdata)
{
    bool hit = pos->y < 12*3 && pos->x > 110*3 && pos->x < 233*3;

    return hit ? SDL_HITTEST_DRAGGABLE : SDL_HITTEST_NORMAL;
}

void show_error(const char *title, const char *mesg, bool fatal)
{
    printf("%s: %s\n", title, mesg);
    // SDL_ShowSimpleMessageBox(SDL_MESSAGEBOX_ERROR, title, mesg, NULL);
    if (fatal)
    {
        exit(1);
    }
}

bool load_rom_from_file(const char *path, struct nrom *nrom)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        show_error("Error loading ROM:", "Can't open the file", false);
        return false;
    }
    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t *rom = malloc(fsize);
    fread(rom, 1, fsize, fp);
    fclose(fp);

    if (!nrom->apu_mux.mux)
    {
        nrom->apu_mux = sdl_mux_make();
    }
    struct mux_api mux = nrom->apu_mux;

    if (nrom_load(rom, nrom))
    {
        nrom->apu_mux = mux;
        show_error("Error loading ROM:", "Invalid ROM or I don't support that mapper oops", false);
        return false;
    }

    nrom->apu_mux = mux;

    return true;
}

struct neske_ui
{
    struct nrom *nrom;
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Mutex *mutex;
    
    bool emulating;
    bool error;
    bool crash;
    bool mouse_released;
    bool ctx_file;

    SDL_Texture *tex_backbuffer;

    SDL_Texture *tex_menu;
    SDL_Texture *tex_select;
    SDL_Texture *tex_error;
    SDL_Texture *tex_crash;
    SDL_Texture *tex_ctx_file;
};

struct SDL_Texture *load_ui_texture(SDL_Renderer *renderer, const char *path)
{
    SDL_Texture *t = IMG_LoadTexture(renderer, path);
    SDL_SetTextureScaleMode(t, SDL_SCALEMODE_NEAREST);
    if (t == NULL)
    {
        SDL_LogError(0, "Can't open image: %s %s", path, SDL_GetError());
        exit(1);
    }
    return t;
}

struct neske_ui neske_ui_init(SDL_Renderer *renderer, SDL_Window *window, struct nrom *nrom)
{
    struct neske_ui ui = { 0 };

    ui.mutex = SDL_CreateMutex();
    ui.emulating = false;
    ui.renderer = renderer;
    ui.window = window;
    ui.nrom = nrom;

    ui.tex_menu = load_ui_texture(renderer, "menu.png");
    ui.tex_select = load_ui_texture(renderer, "select.png");
    ui.tex_error = load_ui_texture(renderer, "error.png");
    ui.tex_crash = load_ui_texture(renderer, "crash.png");
    ui.tex_ctx_file = load_ui_texture(renderer, "ctx_file.png");
    ui.tex_backbuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 256, 240);
    SDL_SetTextureScaleMode(ui.tex_backbuffer, SDL_SCALEMODE_NEAREST);

    return ui;
}

void draw_nes_emu(SDL_Renderer *renderer, SDL_Texture *sdltexture, struct nrom_frame_result result)
{
    uint32_t texture[240*256];

    for (int i = 0; i < 240*256; i++)
    {
        texture[i] = pallete[result.screen[i]];
    }

    SDL_UpdateTexture(sdltexture, NULL, texture, 256*4);

    SDL_FRect srcf = {0, 0, 128*8, 8};
    SDL_FRect src = {0, 0, 256, 240};
    SDL_FRect dst = {1, 13, 256, 240};
    SDL_RenderTexture(renderer, sdltexture, &src, &dst);
}

bool draw_widget(struct neske_ui *ui, const char *name, int x, int y, int w, int h)
{
    float mx, my;
    bool pressed = SDL_GetMouseState(&mx, &my) & SDL_BUTTON_LMASK;

    bool intersect = false;

    if (SDL_PointInRect(&(SDL_Point){mx/3, my/3}, &(SDL_Rect){x, y, w, h}))
    {
        SDL_SetRenderDrawColor(ui->renderer, 0xFF, 0xFF, 0xFF, 0x22);
        SDL_RenderFillRect(ui->renderer, &(SDL_FRect){x, y, w, h});

        intersect = true;
    }

    return intersect && ui->mouse_released;
}

bool neske_ui_event(struct neske_ui *ui, SDL_Event *event)
{
    switch (event->type)
    {
    case SDL_EVENT_MOUSE_BUTTON_UP:
        if (event->button.button == SDL_BUTTON_LEFT) {
            ui->mouse_released = true;
            return true;
        }
    }

    return false;
}

static void SDLCALL on_rom_open(void *userdata, const char *const *filelist, int filter)
{
    if (!filelist)
    {
        SDL_LogError(0, "An error occured: %s", SDL_GetError());
        return;
    }
    else if (!*filelist)
    {
        return;
    }

    struct neske_ui *ui = userdata;
    
    SDL_LockMutex(ui->mutex);
    ui->emulating = false;
    ui->error = false;
    ui->crash = false;
    if (!load_rom_from_file(*filelist, ui->nrom))
    {
        ui->error = true;
    }
    else
    {
        ui->emulating = true;
    }
    SDL_UnlockMutex(ui->mutex);
}

void neske_ui_update(struct neske_ui *ui)
{
    if (ui->crash)
    {
        SDL_RenderTexture(ui->renderer, ui->tex_crash, NULL, &(SDL_FRect){1, 13, 256, 240});
    }
    else if (ui->error)
    {
        SDL_RenderTexture(ui->renderer, ui->tex_error, NULL, &(SDL_FRect){1, 13, 256, 240});
    }
    else if (ui->emulating)
    {
        SDL_LockMutex(ui->mutex);
        draw_nes_emu(ui->renderer, ui->tex_backbuffer, nrom_frame(ui->nrom));
        if (ui->nrom->cpu.crash)
        {
            ui->crash = true;
        }
        SDL_UnlockMutex(ui->mutex);
    }
    else
    {
        SDL_RenderTexture(ui->renderer, ui->tex_select, NULL, &(SDL_FRect){1, 13, 256, 240});
    }
    
    SDL_RenderTexture(ui->renderer, ui->tex_menu, NULL, NULL);

    if (ui->ctx_file)
    {
        SDL_RenderTexture(ui->renderer, ui->tex_ctx_file, NULL, NULL);

        if (draw_widget(ui, "Load ROM", 1, 13, 47, 11))
        {
            const SDL_DialogFileFilter filters[] = {
                { "iNES (.nes)",  "nes" },
                { "All files",   "*" }
            };

            SDL_ShowOpenFileDialog(on_rom_open, ui, ui->window, filters, 2, NULL, false);
        }

        if (draw_widget(ui, "Unload ROM", 1, 25, 47, 11))
        {
            ui->emulating = false;
        }

        if (draw_widget(ui, "Reset", 1, 37, 47, 11))
        {
            ui->crash = false;
            nrom_reset(ui->nrom);
        }
    }

    if (draw_widget(ui, "File", 1, 1, 24, 11))
    {
        ui->ctx_file = !ui->ctx_file;
    }
    else if (ui->mouse_released)
    {
        ui->ctx_file = false;
    }
    
    if (draw_widget(ui, "X", 246, 1, 11, 11))
    {
        exit(0);
    }

    if (draw_widget(ui, "-", 234, 1, 11, 11))
    {
        SDL_MinimizeWindow(ui->window);
    }


    ui->mouse_released = false;
}

void audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    struct neske_ui *neske_ui = userdata;
    // TODO? can have race condition? idk
    if (!neske_ui->emulating)
    {
        return;
    }

    struct nrom *nrom = neske_ui->nrom;
    struct apu *apu = &nrom->apu;

    uint16_t buf[16384];

    nrom->apu_mux.lock(nrom->apu_mux.mux);
    apu->samples_read += additional_amount/2;
    apu_catchup_samples(apu, additional_amount/2);
    apu_ring_read(apu, buf, additional_amount/2);
    nrom->apu_mux.unlock(nrom->apu_mux.mux);
    SDL_PutAudioStreamData(stream, buf, additional_amount);
}


int main(int argc, char* argv[]) {
    if (argc != 2)
    {
        fprintf(stderr, "Usage: %s <NES ROM>\n", argv[0]);
        return 1;
    }

    struct nrom nrom = { 0 };
    SDL_Window *window;                    // Declare a pointer
    SDL_Renderer *renderer;
    bool done = false;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO);              // Initialize SDL3

    // Create an application window with the following settings:
    window = SDL_CreateWindow(
        "neske",                           // window title
        258*3,                               // width, in pixels
        254*3,                               // height, in pixels
        SDL_WINDOW_OPENGL | SDL_WINDOW_BORDERLESS
    );
    SDL_SetWindowHitTest(window, hit_test, NULL);

    if (window == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create window: %s\n", SDL_GetError());
        return 1;
    }

    renderer = SDL_CreateRenderer(window, NULL);
    SDL_SetRenderVSync(renderer, 1);
    SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

    if (renderer == NULL)
    {
        SDL_LogError(SDL_LOG_CATEGORY_ERROR, "Could not create renderer: %s\n", SDL_GetError());
        return 1;
    }
    
    struct ui_draw uidraw = ui_draw_mk(renderer);

    bool show = true;
    struct controller_state controller = { 0 };

    SDL_AudioSpec audio_in = { 0 };

    audio_in.channels = 1;
    audio_in.format = SDL_AUDIO_S16;
    audio_in.freq = 44100;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);

    SDL_SetRenderScale(renderer, 3, 3);

    struct neske_ui neske_ui = neske_ui_init(renderer, window, &nrom);
    SDL_AudioStream *audio_device_stream = SDL_OpenAudioDeviceStream(audio_device, &audio_in, audio_callback, &neske_ui);
    SDL_ResumeAudioStreamDevice(audio_device_stream);

    while (!done) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            if (neske_ui_event(&neske_ui, &event))
            {
                continue;
            }

            switch (event.type)
            {
            case SDL_EVENT_QUIT:
                done = true;
                break;
            case SDL_EVENT_KEY_DOWN:
            case SDL_EVENT_KEY_UP:
                show = false;
                if (event.key.key == SDLK_X) controller.btns[BTN_A] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_Z) controller.btns[BTN_B] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_RETURN) controller.btns[BTN_START] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_TAB) controller.btns[BTN_SELECT] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_UP) controller.btns[BTN_UP] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_DOWN) controller.btns[BTN_DOWN] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_LEFT) controller.btns[BTN_LEFT] = event.type == SDL_EVENT_KEY_DOWN;
                if (event.key.key == SDLK_RIGHT) controller.btns[BTN_RIGHT] = event.type == SDL_EVENT_KEY_DOWN;
                if (neske_ui.emulating)
                {
                    nrom_update_controller(&nrom, controller);
                }
                break;
            }
        }

        neske_ui_update(&neske_ui);

        SDL_RenderPresent(renderer);
    }

    // Close and destroy the window
    SDL_DestroyWindow(window);

    // Clean up
    SDL_Quit();
    return 0;
}