#ifndef INCLUDE_CPYMO_PARSER
#define INCLUDE_CPYMO_PARSER

#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "cpymo_color.h"

typedef struct {
	const char *begin;
	size_t len;
} cpymo_parser_stream_span;

typedef struct {
	cpymo_parser_stream_span stream;

	size_t cur_pos;
	size_t cur_line;
	bool is_line_end;
} cpymo_parser;

void cpymo_parser_init(cpymo_parser *parser, const char *stream, size_t len);
void cpymo_parser_reset(cpymo_parser *parser);
bool cpymo_parser_next_line(cpymo_parser *parser);

char cpymo_parser_curline_readchar(cpymo_parser *parser);
char cpymo_parser_curline_peek(cpymo_parser *parser);
cpymo_parser_stream_span cpymo_parser_curline_readuntil(cpymo_parser *parser, char until);
cpymo_parser_stream_span cpymo_parser_curline_readuntil_or(cpymo_parser *parser, char until1, char until2);
cpymo_parser_stream_span cpymo_parser_curline_readuntil_or3(cpymo_parser *parser, char until1, char until2, char until3);
cpymo_parser_stream_span cpymo_parser_curline_pop_commacell(cpymo_parser *parser);
cpymo_parser_stream_span cpymo_parser_curline_pop_command(cpymo_parser *parser);
cpymo_parser_stream_span cpymo_parser_stream_span_pure(const char *);

void cpymo_parser_stream_span_trim_start(cpymo_parser_stream_span *span);
void cpymo_parser_stream_span_trim_end(cpymo_parser_stream_span *span);
void cpymo_parser_stream_span_trim(cpymo_parser_stream_span *span);
void cpymo_parser_stream_span_copy(char *dst, size_t buffer_size, cpymo_parser_stream_span span);
int cpymo_parser_stream_span_atoi(cpymo_parser_stream_span span);
float cpymo_parser_stream_span_atof(cpymo_parser_stream_span span);
cpymo_color cpymo_parser_stream_span_as_color(cpymo_parser_stream_span span);

bool cpymo_parser_stream_span_equals_str(cpymo_parser_stream_span span, const char *str);
bool cpymo_parser_stream_span_equals(cpymo_parser_stream_span a, cpymo_parser_stream_span b);
bool cpymo_parser_stream_span_equals_ignore_case(cpymo_parser_stream_span a, cpymo_parser_stream_span b);
bool cpymo_parser_stream_span_equals_str_ignore_case(cpymo_parser_stream_span a, const char *b);
bool cpymo_parser_stream_span_starts_with_str_ignore_case(cpymo_parser_stream_span span, const char *prefix);

cpymo_parser_stream_span cpymo_parser_stream_span_utf8_try_head(cpymo_parser_stream_span *tail);
uint32_t cpymo_parser_stream_span_utf8_try_head_to_utf32(cpymo_parser_stream_span *tail);
size_t cpymo_parser_stream_span_utf8_len(cpymo_parser_stream_span span);
cpymo_parser_stream_span cpymo_parser_stream_span_split(cpymo_parser_stream_span *tail, size_t skip);

static inline void cpymo_parser_stream_span_hash_init(uint64_t *hash) 
{ *hash = 0; }

void cpymo_parser_stream_span_hash_step(uint64_t *hash, char ch);
void cpymo_parser_stream_span_hash_append_cstr(uint64_t *hash, const char *s);
void cpymo_parser_stream_span_hash_append(
	uint64_t *hash, cpymo_parser_stream_span span);

#endif
