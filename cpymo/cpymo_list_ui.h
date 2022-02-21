#ifndef INCLUDE_CPYMO_LIST_UI
#define INCLUDE_CPYMO_LIST_UI

#include "cpymo_ui.h"
#include <stdint.h>

typedef void *(*cpymo_list_ui_get_node)(const struct cpymo_engine *, const void *ui_data, const void *cur);
typedef void (*cpymo_list_ui_draw_node)(const struct cpymo_engine *, const void *node_to_draw, float y);

typedef struct {
	cpymo_ui_deleter ui_data_deleter;
	cpymo_list_ui_draw_node draw_node;

	void *current_node;
	int selection_relative_to_cur;
	cpymo_list_ui_get_node get_next;
	cpymo_list_ui_get_node get_prev;
	bool from_bottom_to_top;
	float node_height;

	float current_y;
} cpymo_list_ui;

error_t cpymo_list_ui_enter(
	struct cpymo_engine *,
	void **out_ui_data,
	size_t ui_data_size,
	cpymo_list_ui_draw_node draw_node,
	cpymo_ui_deleter ui_data_deleter,
	void *current,
	cpymo_list_ui_get_node get_next,
	cpymo_list_ui_get_node get_prev,
	bool from_bottom_to_top,
	size_t nodes_per_screen);

static inline void cpymo_list_ui_set_current_node(struct cpymo_engine *e, void *cur)
{
	cpymo_list_ui *ui = (cpymo_list_ui *)cpymo_ui_data(e);
	ui->current_node = cur;
}

static inline void *cpymo_list_ui_data(struct cpymo_engine *e)
{ return ((uint8_t *)cpymo_ui_data(e)) + sizeof(cpymo_list_ui); }

static inline const void *cpymo_list_ui_data_const(const struct cpymo_engine *e)
{ return ((uint8_t *)cpymo_ui_data_const(e)) + sizeof(cpymo_list_ui); }

static inline void cpymo_list_ui_exit(struct cpymo_engine *e)
{ cpymo_ui_exit(e); }


#endif