#ifndef INCLUDE_CPYMO_BACKEND_TEXT
#define INCLUDE_CPYMO_BACKEND_TEXT

#include "../../cpymo/cpymo_error.h"
#include "../../cpymo/cpymo_color.h"
#include "cpymo_backend_image.h"
#include "../../cpymo/cpymo_parser.h"

typedef void *cpymo_backend_text;

error_t cpymo_backend_text_create(
    cpymo_backend_text *out, 
    float *out_width,
    cpymo_parser_stream_span utf8_string, 
    float single_character_size_in_logical_screen);

void cpymo_backend_text_free(cpymo_backend_text);

void cpymo_backend_text_draw(
    cpymo_backend_text,
    float x, float y_baseline,
    cpymo_color col, float alpha,
    enum cpymo_backend_image_draw_type draw_type);

float cpymo_backend_text_width(
    cpymo_parser_stream_span,
    float single_character_size_in_logical_screen);

#ifndef NON_VISUALLY_IMPAIRED_HELP
void cpymo_backend_text_visually_impaired_help(const char *text);
#endif

#endif
