#include <stdio.h>
#include <time.h>
#include "SDL3/SDL_audio.h"
#include "SDL3/SDL_dialog.h"
#include "SDL3/SDL_error.h"
#include "SDL3/SDL_events.h"
#include "SDL3/SDL_gamepad.h"
#include "SDL3/SDL_image.h"
#include "SDL3/SDL_init.h"
#include "SDL3/SDL_keyboard.h"
#include "SDL3/SDL_keycode.h"
#include "SDL3/SDL_log.h"
#include "SDL3/SDL_misc.h"
#include "SDL3/SDL_mouse.h"
#include "SDL3/SDL_mutex.h"
#include "SDL3/SDL_rect.h"
#include "SDL3/SDL_render.h"
#include "SDL3/SDL_surface.h"
#include <SDL3/SDL.h>
#include "SDL3/SDL_video.h"
#include "SDL3/SDL_main.h"
#include "neske.h"

uint32_t pallete[] = {
    0x626262ff, 0x001fb2ff, 0x2404c8ff, 0x5200b2ff,
    0x730076ff, 0x800024ff, 0x730b00ff, 0x522800ff,
    0x244400ff, 0x005700ff, 0x005c00ff, 0x005324ff,
    0x003c76ff, 0x000000ff, 0x000000ff, 0x000000ff,
    0xabababff, 0x0d57ffff, 0x4b30ffff, 0x8a13ffff,
    0xbc08d6ff, 0xd21269ff, 0xc72e00ff, 0x9d5400ff,
    0x607b00ff, 0x209800ff, 0x00a300ff, 0x009942ff,
    0x007db4ff, 0x000000ff, 0x000000ff, 0x000000ff,
    0xffffffff, 0x53aeffff, 0x9085ffff, 0xd365ffff,
    0xff57ffff, 0xff5dcfff, 0xff7757ff, 0xfa9e00ff,
    0xbdc700ff, 0x7ae700ff, 0x43f611ff, 0x26ef7eff,
    0x2cd5f6ff, 0x4e4e4eff, 0x000000ff, 0x000000ff,
    0xffffffff, 0xb6e1ffff, 0xced1ffff, 0xe9c3ffff,
    0xffbcffff, 0xffbdf4ff, 0xffc6c3ff, 0xffd59aff,
    0xe9e681ff, 0xcef481ff, 0xb6fb9aff, 0xa9fac3ff,
    0xa9f0f4ff, 0xb8b8b8ff, 0x000000ff, 0x000000ff,
};

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
    int w, h;
    SDL_GetWindowSize(win, &w, &h);

    int ui_scale = w/258;

    bool hit = pos->y < 12*ui_scale && pos->x > 110*ui_scale && pos->x < 233*ui_scale;

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

struct player load_rom_from_file(const char *path, struct mux_api apu_mux)
{
    FILE *fp = fopen(path, "rb");
    if (!fp)
    {
        show_error("Error loading ROM:", "Can't open the file", false);
        return (struct player){ 0 };
    }
    fseek(fp, 0, SEEK_END);
    size_t fsize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    uint8_t *rom = malloc(fsize);
    fread(rom, 1, fsize, fp);
    fclose(fp);

    if (fsize < 16)
    {
        return (struct player){ 0 };
    }

    struct player player = player_init(rom, apu_mux);

    if (!player.is_valid)
    {
        show_error("Error loading ROM:", "Invalid ROM or I don't support that mapper oops", false);
        return (struct player){ 0 };
    }

    return player;
}

struct controls_spec
{
    SDL_Keycode keys[8];
};

enum ui_window
{
    WIN_NONE,
    WIN_CONFIG,
    WIN_ABOUT,
    WIN_FUN,
};

struct rtc_state
{
    int level;
    int frames_since_last_pulse;
};  

struct neske_ui
{
    int scale;
    struct player player;
    struct mux_api apu_mux;
    SDL_Renderer *renderer;
    SDL_Window *window;
    SDL_Mutex *mutex;

    struct rtc_state rtc_state;

    bool emulating;
    bool error;
    bool crash;
    bool mouse_released;
    bool ctx_file;
    bool changing_control;

    enum ui_window show_window;
    struct controller_state controller;
    struct controls_spec controls;
    enum controller_btn btn_selected;
    SDL_Texture *tex_backbuffer;

    SDL_Cursor *cursor;
    SDL_Texture *tex_menu;
    SDL_Texture *tex_select;
    SDL_Texture *tex_error;
    SDL_Texture *tex_crash;
    SDL_Texture *tex_ctx_file;
    SDL_Texture *tex_config;
    SDL_Texture *tex_about;
    SDL_Texture *tex_fun;
    SDL_Texture *tex_userfont;
};

static void draw_user_text(struct neske_ui *ui, const char *text, int x, int y)
{
    for (int i = 0; text[i]; i++)
    {
        char ch = text[i];

        if (ch >= 'a' && ch <= 'z')
        {
            ch -= 'a'-'A';
        }
        
        int index = -1;

        if (ch >= '0' && ch <= '9')
        {
            index = ch-'0';
        }

        if (ch >= 'A' && ch <= 'Z')
        {
            index = ch-'A'+10;
        }

        if (index != -1)
        {
            SDL_RenderTexture(
                ui->renderer,
                ui->tex_userfont,
                &(SDL_FRect){index*3, 0, 3, 7},
                &(SDL_FRect){x+i*4, y, 3, 7}
            );
        }
    }
}

static struct SDL_Texture *load_ui_texture(SDL_Renderer *renderer, const char *path)
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

static void set_default_controls(struct controls_spec *controls)
{
    controls->keys[BTN_A] = SDLK_X;
    controls->keys[BTN_B] = SDLK_Z;
    controls->keys[BTN_START] = SDLK_RETURN;
    controls->keys[BTN_SELECT] = SDLK_TAB;
    controls->keys[BTN_UP] = SDLK_UP;
    controls->keys[BTN_DOWN] = SDLK_DOWN;
    controls->keys[BTN_LEFT] = SDLK_LEFT;
    controls->keys[BTN_RIGHT] = SDLK_RIGHT;
}

void rtc_iter(struct rtc_state *rtc, struct system *sys)
{
    if (rtc->level == 0) return;
    rtc->frames_since_last_pulse++;
    int frames = 20;
    if (rtc->level == 1) frames = 10;
    if (rtc->frames_since_last_pulse < frames) return;
    rtc->frames_since_last_pulse = 0;

    uint16_t addr = rand()%0x2000;
    while (addr >= 0x100 && addr < 0x200)
    {
        addr = rand()%0x2000;
    }

    uint8_t val = sys->memory[addr];
    if (rtc->level == 2)
    {
        val = rand()%0x100;
    }
    else
    {
        int delta = rand()%2 ? 1 : -1;
        if (val == 0) delta = 1;
        if (val == 0xFF) delta = -1;
        val += delta;
    }

    sys->memory[addr] = val;
}

struct neske_ui neske_ui_init(SDL_Renderer *renderer, SDL_Window *window, int ui_scale)
{
    struct neske_ui ui = { 0 };

    int count = 1;
    SDL_JoystickID *ids = SDL_GetJoysticks(&count);

    if (count > 0)
        if (!SDL_OpenGamepad(ids[0]))
        {
            printf("Error opening gamepad: %s\n", SDL_GetError());
        }


    ui.apu_mux = sdl_mux_make();
    ui.btn_selected = -1;
    ui.mutex = SDL_CreateMutex();
    ui.emulating = false;
    ui.renderer = renderer;
    ui.window = window;
    ui.scale = ui_scale;

    set_default_controls(&ui.controls);

    ui.tex_menu = load_ui_texture(renderer, "img/menu.png");
    ui.tex_select = load_ui_texture(renderer, "img/select.png");
    ui.tex_error = load_ui_texture(renderer, "img/error.png");
    ui.tex_crash = load_ui_texture(renderer, "img/crash.png");
    ui.tex_ctx_file = load_ui_texture(renderer, "img/ctx_file.png");
    ui.tex_config = load_ui_texture(renderer, "img/config.png");
    ui.tex_about = load_ui_texture(renderer, "img/about.png");
    ui.tex_fun = load_ui_texture(renderer, "img/fun.png");
    ui.tex_userfont = load_ui_texture(renderer, "img/userfont.png");
    ui.tex_backbuffer = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGBA8888, SDL_TEXTUREACCESS_TARGET, 256, 240);
    SDL_SetTextureScaleMode(ui.tex_backbuffer, SDL_SCALEMODE_NEAREST);

    SDL_Surface *cursor_surface = IMG_Load("img/cursor.png");
    if (!cursor_surface)
    {
        SDL_LogError(0, "Can't open image: %s %s", "img/cursor.png", SDL_GetError());
        exit(1);
    }

    SDL_Surface *cursor_surface_2 = SDL_ScaleSurface(cursor_surface, cursor_surface->w*ui.scale, cursor_surface->h*ui.scale, SDL_SCALEMODE_NEAREST);

    ui.cursor = SDL_CreateColorCursor(cursor_surface_2, 2*ui.scale, 1*ui.scale);

    SDL_SetCursor(ui.cursor);
    
    SDL_DestroySurface(cursor_surface);
    SDL_DestroySurface(cursor_surface_2);

    return ui;
}

void draw_nes_emu(SDL_Renderer *renderer, SDL_Texture *sdltexture, struct system_frame_result result)
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

    if (SDL_PointInRect(&(SDL_Point){mx/ui->scale, my/ui->scale}, &(SDL_Rect){x, y, w, h}))
    {
        if (name[0] == '#') 
        {
            SDL_SetRenderDrawColor(ui->renderer, 0x00, 0x00, 0x77, 0x22);
        }
        else
        {
            SDL_SetRenderDrawColor(ui->renderer, 0xFF, 0xFF, 0xFF, 0x22);
        }
        SDL_RenderFillRect(ui->renderer, &(SDL_FRect){x, y, w, h});

        intersect = true;
    }

    return intersect && ui->mouse_released;
}

bool neske_ui_event(struct neske_ui *ui, SDL_Event *event)
{
    SDL_LockMutex(ui->mutex);

    switch (event->type)
    {
        case SDL_EVENT_MOUSE_BUTTON_UP:
            if (event->button.button == SDL_BUTTON_LEFT) ui->mouse_released = true;
            break;
        case SDL_EVENT_GAMEPAD_BUTTON_DOWN:
        case SDL_EVENT_GAMEPAD_BUTTON_UP:
            if (ui->show_window == WIN_NONE)
            {
                switch (event->gbutton.button)
                {
                case SDL_GAMEPAD_BUTTON_SOUTH:
                    ui->controller.btns[BTN_A] = event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
                    break;
                case SDL_GAMEPAD_BUTTON_WEST:
                    ui->controller.btns[BTN_B] = event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
                    break;
                case SDL_GAMEPAD_BUTTON_START:
                    ui->controller.btns[BTN_START] = event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
                    break;
                case SDL_GAMEPAD_BUTTON_BACK:
                    ui->controller.btns[BTN_SELECT] = event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_UP:
                    ui->controller.btns[BTN_UP] = event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_DOWN:
                    ui->controller.btns[BTN_DOWN] = event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_LEFT:
                    ui->controller.btns[BTN_LEFT] = event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
                    break;
                case SDL_GAMEPAD_BUTTON_DPAD_RIGHT:
                    ui->controller.btns[BTN_RIGHT] = event->type == SDL_EVENT_GAMEPAD_BUTTON_DOWN;
                    break;
                }

                if (ui->emulating)
                {
                    player_set_controller(&ui->player, ui->controller);
                }
            }
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            if (ui->show_window == WIN_NONE)
            {
                const enum controller_btn buttons[] = { BTN_A, BTN_B, BTN_START, BTN_SELECT, BTN_UP, BTN_DOWN, BTN_LEFT, BTN_RIGHT };

                for (int i = 0; i < 8; i++)
                {
                    if (event->key.key == ui->controls.keys[buttons[i]])
                    {
                        ui->controller.btns[buttons[i]] = event->type == SDL_EVENT_KEY_DOWN;
                    }
                }

                if (ui->emulating)
                {
                    player_set_controller(&ui->player, ui->controller);
                }
            }
            else if (ui->show_window == WIN_CONFIG)
            {
                if (ui->changing_control && event->type == SDL_EVENT_KEY_DOWN)
                {
                    ui->controls.keys[ui->btn_selected] = event->key.key;
                    ui->changing_control = false;
                }
            }
            break;
    }

    SDL_UnlockMutex(ui->mutex);

    return true;
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
    if (ui->player.is_valid)
    {
        player_free(&ui->player);
    }
    ui->player = load_rom_from_file(*filelist, ui->apu_mux);
    if (!ui->player.is_valid)
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
    SDL_LockMutex(ui->mutex);

    if (ui->player.is_valid)
    {
        struct system *sys = player_get_system(&ui->player);
        rtc_iter(&ui->rtc_state, sys);
    }

    if (ui->crash)
    {
        SDL_RenderTexture(ui->renderer, ui->tex_crash, NULL, &(SDL_FRect){1, 13, 256, 240});
    }
    else if (ui->error)
    {
        SDL_RenderTexture(ui->renderer, ui->tex_error, NULL, &(SDL_FRect){1, 13, 256, 240});
        if (draw_widget(ui, "List", 56, 196+13, 143, 12))
        {
            SDL_OpenURL("https://github.com/ske2004/neske?tab=readme-ov-file#game-support");
        }
    }
    else if (ui->emulating)
    {
        draw_nes_emu(ui->renderer, ui->tex_backbuffer, player_frame(&ui->player));
        if (player_crash(&ui->player))
        {
            ui->crash = true;
        }
    }
    else
    {
        SDL_RenderTexture(ui->renderer, ui->tex_select, NULL, &(SDL_FRect){1, 13, 256, 240});
    }
    
    SDL_RenderTexture(ui->renderer, ui->tex_menu, NULL, NULL);

    if (ui->show_window != WIN_NONE)
    {
        SDL_SetRenderDrawColor(ui->renderer, 0, 0, 0, 0x44);
        SDL_RenderFillRect(ui->renderer, &(SDL_FRect){1, 13, 256, 240});

        switch (ui->show_window)
        {
        case WIN_CONFIG:
            {
                SDL_RenderTexture(ui->renderer, ui->tex_config, NULL, NULL);

                int btn = -1;

                if (draw_widget(ui, "CFG_X", 197, 95, 11, 11))
                {
                    ui->show_window = WIN_NONE;
                }

                if (draw_widget(ui, "CFG_OK", 155, 140, 45, 13))
                {
                    ui->show_window = WIN_NONE;
                }

                if (draw_widget(ui, "CFG_RESET", 155, 124, 45, 13))
                {
                    ui->changing_control = false;
                    set_default_controls(&ui->controls);
                }

                if (draw_widget(ui, "#CFG_LEFT", 59, 134, 7, 7))
                {
                    btn = BTN_LEFT;
                }

                if (draw_widget(ui, "#CFG_RIGHT", 71, 134, 7, 7))
                {
                    btn = BTN_RIGHT;
                }

                if (draw_widget(ui, "#CFG_UP", 65, 128, 7, 7))
                {
                    btn = BTN_UP;
                }

                if (draw_widget(ui, "#CFG_DOWN", 65, 140, 7, 7))
                {
                    btn = BTN_DOWN;
                }

                if (draw_widget(ui, "#CFG_SELECT", 82, 137, 12, 7))
                {
                    btn = BTN_SELECT;
                }

                if (draw_widget(ui, "#CFG_START", 95, 137, 12, 7))
                {
                    btn = BTN_START;
                }
                
                if (draw_widget(ui, "#CFG_B", 111, 135, 11, 11))
                {
                    btn = BTN_B;
                }

                if (draw_widget(ui, "#CFG_A", 124, 135, 11, 11))
                {
                    btn = BTN_A;
                }

                if (btn != -1)
                {
                    ui->btn_selected = btn;
                    ui->changing_control = true;
                }

                if (ui->btn_selected != -1)
                {
                    draw_user_text(ui, SDL_GetKeyName(ui->controls.keys[ui->btn_selected]), 157, 112);
                }

                if (ui->changing_control)
                {
                    SDL_SetRenderDrawColor(ui->renderer, 0xFF, 0xFF, 0, 0xFF);
                    SDL_RenderRect(ui->renderer, &(SDL_FRect){155, 109, 46, 13});
                }
            }
            break;
        case WIN_ABOUT:
            SDL_RenderTexture(ui->renderer, ui->tex_about, NULL, NULL);
            if (draw_widget(ui, "ABOUT_X", 197, 95, 11, 11))
            {
                ui->show_window = WIN_NONE;
            }

            if (draw_widget(ui, "ABOUT_CLOSE", 152, 140, 51, 13))
            {
                ui->show_window = WIN_NONE;
            }
            
            if (draw_widget(ui, "ABOUT_URL", 102, 123, 48, 11))
            {
                SDL_OpenURL("https://ske.land");
            }
            break;
        case WIN_FUN:
            SDL_RenderTexture(ui->renderer, ui->tex_fun, NULL, NULL);
            if (draw_widget(ui, "FUN_X", 165, 95, 11, 11))
            {
                ui->show_window = WIN_NONE;
            }

            if (draw_widget(ui, "FUN_OK", 160, 110, 13, 44))
            {
                ui->show_window = WIN_NONE;
            }

            {
                SDL_FRect fun_btn_off = {84, 125, 24, 28};
                SDL_FRect fun_btn_low = {108, 125, 24, 28};
                SDL_FRect fun_btn_hi = {132, 125, 23, 28};

                if (draw_widget(ui, "FUN_BTN_OFF", fun_btn_off.x, fun_btn_off.y, fun_btn_off.w, fun_btn_off.h))
                {
                    ui->rtc_state.level = 0;
                }

                if (draw_widget(ui, "FUN_BTN_LOW", fun_btn_low.x, fun_btn_low.y, fun_btn_low.w, fun_btn_low.h))
                {
                    ui->rtc_state.level = 1;
                }

                if (draw_widget(ui, "FUN_BTN_HI", fun_btn_hi.x, fun_btn_hi.y, fun_btn_hi.w, fun_btn_hi.h))
                {
                    ui->rtc_state.level = 2;
                }

                SDL_SetRenderDrawColor(ui->renderer, 0xFF, 0xFF, 0x0, 0xFF); 
                switch (ui->rtc_state.level)
                {
                    case 0: SDL_RenderRect(ui->renderer, &fun_btn_off); break;
                    case 1: SDL_RenderRect(ui->renderer, &fun_btn_low); break;
                    case 2: SDL_RenderRect(ui->renderer, &fun_btn_hi); break;
                }
            }
            break;
        case WIN_NONE:
            break;
        }
    }

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
            player_reset(&ui->player);
            ui->emulating = false;
        }

        if (draw_widget(ui, "Reset", 1, 37, 47, 11))
        {
            printf("Reset\n");
            ui->crash = false;
            player_reset(&ui->player);
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

    if (draw_widget(ui, "Config", 26, 1, 32, 11))
    {
        ui->changing_control = false;
        ui->btn_selected = -1;
        ui->show_window = WIN_CONFIG;
    }
    
    if (draw_widget(ui, "About", 59, 1, 29, 11))
    {
        ui->show_window = WIN_ABOUT;
    }

    if (draw_widget(ui, "Fun", 89, 1, 21, 11))
    {
        ui->show_window = WIN_FUN;
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

    SDL_UnlockMutex(ui->mutex);
}

void audio_callback(void *userdata, SDL_AudioStream *stream, int additional_amount, int total_amount)
{
    struct neske_ui *ui = userdata;

    uint16_t buf[16384] = { 0 };

    if (ui->player.is_valid)
    {
        // TODO: Maybe i should lock the apu mux here?
        player_generate_samples(&ui->player, buf, additional_amount/2);
    }

    SDL_PutAudioStreamData(stream, buf, additional_amount);
}

int _get_ui_scale()
{
    const SDL_DisplayMode *dm = SDL_GetDesktopDisplayMode(1);
    if (dm == NULL) return 3;
    int mw = dm->w/1.2, mh = dm->h/1.2;
    int v = mh/254;
    if (mw < mh) v = mw/258;
    if (v < 1) v = 1;
    return v;
}

int main(int argc, char* argv[])
{
    srand(time(NULL));

    SDL_Window *window;
    SDL_Renderer *renderer;
    bool done = false;

    SDL_Init(SDL_INIT_VIDEO | SDL_INIT_AUDIO | SDL_INIT_GAMEPAD);
    
    int ui_scale = _get_ui_scale();

    window = SDL_CreateWindow(
        "neske",
        258*ui_scale,
        254*ui_scale,
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
    
    SDL_AudioSpec audio_in = { 0 };

    audio_in.channels = 1;
    audio_in.format = SDL_AUDIO_S16;
    audio_in.freq = 44100;

    SDL_AudioDeviceID audio_device = SDL_OpenAudioDevice(SDL_AUDIO_DEVICE_DEFAULT_PLAYBACK, NULL);

    SDL_SetRenderScale(renderer, ui_scale, ui_scale);

    struct neske_ui neske_ui = neske_ui_init(renderer, window, ui_scale);
    SDL_AudioStream *audio_device_stream = SDL_OpenAudioDeviceStream(audio_device, &audio_in, audio_callback, &neske_ui);
    SDL_ResumeAudioStreamDevice(audio_device_stream);
    while (!done) {
        SDL_Event event;

        while (SDL_PollEvent(&event)) {
            
            if (event.type == SDL_EVENT_QUIT)
            {
                done = true;
                continue;
            }

            if (neske_ui_event(&neske_ui, &event))
            {
                continue;
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