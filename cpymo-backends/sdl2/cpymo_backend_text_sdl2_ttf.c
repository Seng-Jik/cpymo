#ifdef ENABLE_SDL2_TTF
#include <cpymo_engine.h>
#include <cpymo_utils.h>
#include <SDL2/SDL_ttf.h>
#include "../include/cpymo_backend_text.h"

static TTF_Font *font = NULL;
static int prev_ptsize = 16;
extern SDL_Renderer * const renderer;
extern const cpymo_engine engine;

void cpymo_backend_font_free()
{
    if (font) {
        TTF_CloseFont(font);
        font = NULL;
    }
    
    TTF_Quit();
}

error_t cpymo_backend_font_init(const char *gamedir)
{
    if (TTF_Init() == -1) return CPYMO_ERR_UNKNOWN;

    const char *font_suffixes[] = {
        "/system/default.ttf",
        "/system/default.otf",
        "/system/default.fnt",
    };

    if (gamedir) {
        CPYMO_FOREACH_ARR(i, font_suffixes) {
            char *path = alloca(strlen(gamedir) + 2 + strlen(font_suffixes[i]));
            sprintf(path, "%s/%s", gamedir, font_suffixes[i]);
            font = TTF_OpenFont(path, prev_ptsize);
            if (font) break;
        }
    }

#ifdef GAME_SELECTOR_DIR
    if (font == NULL) {
        const char *font_paths[] = {
            GAME_SELECTOR_DIR "/default.ttf",
            GAME_SELECTOR_DIR "/default.otf",
            GAME_SELECTOR_DIR "/default.fnt",
        };

        CPYMO_FOREACH_ARR(i, font_paths) {
            font = TTF_OpenFont(font_paths[i], prev_ptsize);
            if (font) break;
        }
    }
#endif

    if (font == NULL) {
        TTF_Quit();
        return CPYMO_ERR_CAN_NOT_OPEN_FILE;
    }

    return CPYMO_ERR_SUCC;
}

static void cpymo_backend_font_update_size(float s)
{
    int ptsize = s;
    if (ptsize != prev_ptsize) {
        //TTF_SetFontSize(font, ptsize);
        //TTF_SetFontKerning(font, 1);
        prev_ptsize = ptsize;
        cpymo_backend_font_free();
        cpymo_backend_font_init(engine.assetloader.gamedir);
    }
}

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_parser_stream_span utf8_string, 
    float single_character_size_in_logical_screen)
{
    char *str = alloca(utf8_string.len + 1);
    cpymo_parser_stream_span_copy(str, utf8_string.len + 1, utf8_string);

    cpymo_backend_font_update_size(single_character_size_in_logical_screen);

    SDL_Color c;
    c.r = 255;
    c.g = 255;
    c.b = 255;
    c.a = 255;

    SDL_Surface *text = TTF_RenderUTF8_Blended(font, str, c);
    if (text == NULL) return CPYMO_ERR_UNKNOWN;

    SDL_Texture *text_tex = SDL_CreateTextureFromSurface(renderer, text);
    int w = text->w;

    SDL_FreeSurface(text);
    if (text_tex == NULL) return CPYMO_ERR_UNKNOWN;

    SDL_SetTextureBlendMode(text_tex, SDL_BLENDMODE_BLEND);

    *out = text_tex;
    *out_width = w;

    return CPYMO_ERR_SUCC;
}

void cpymo_backend_text_free(cpymo_backend_text text)
{ cpymo_backend_image_free(text); }

void cpymo_backend_text_draw(
    cpymo_backend_text t,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type)
{
    int w, h;
    if (SDL_QueryTexture(t, NULL, NULL, &w, &h)) return;

    const float magic_offset = 2.0f;
    y_baseline += magic_offset;
    
    SDL_SetTextureColorMod(t, 255 - col.r, 255 - col.g, 255 - col.b);
    cpymo_backend_image_draw(
        x + 1, y_baseline - h + 1, w, h, t, 0, 0, w, h, alpha, draw_type);

    SDL_SetTextureColorMod(t, col.r, col.g, col.b);
    cpymo_backend_image_draw(
        x, y_baseline - h, w, h, t, 0, 0, w, h, alpha, draw_type);
}

float cpymo_backend_text_width(
    cpymo_parser_stream_span s,
    float single_character_size_in_logical_screen)
{
    cpymo_backend_font_update_size(single_character_size_in_logical_screen);

    char *str = alloca(s.len + 1);
    cpymo_parser_stream_span_copy(str, s.len + 1, s);

    int w;

    if (TTF_SizeUTF8(font, str, &w, NULL) != 0) {
        return 
            cpymo_parser_stream_span_utf8_len(s) 
            * single_character_size_in_logical_screen;
    }

    return (float)w;
}
#endif