#include "cpymo_select_img.h"
#include "cpymo_engine.h"
#include <memory.h>
#include <assert.h>
#include <stdlib.h>

void cpymo_select_img_reset(cpymo_select_img *img)
{
	if (img->select_img_image) {
		// select_img
		cpymo_backend_image_free(img->select_img_image);
	}
	else if(img->selections) {
		// select_imgs
		for (size_t i = 0; i < img->all_selections; ++i) {
			if (img->selections[i].image)
				cpymo_backend_image_free(img->selections[i].image); 
			if (img->selections[i].or_text)
				cpymo_backend_text_free(img->selections[i].or_text);
		}
	}

	if (img->selections)
		free(img->selections);

	for (size_t i = 0; i < 4; ++i) {
		if (img->hint[i]) cpymo_backend_image_free(img->hint[i]);
		img->hint[i] = NULL;
	}

	img->selections = NULL;
	img->select_img_image = NULL;
	img->show_option_background = false;
}

error_t cpymo_select_img_configuare_begin(
	cpymo_select_img *sel, size_t selections,
	cpymo_parser_stream_span image_name_or_empty_when_select_imgs,
	cpymo_assetloader *loader, cpymo_gameconfig *gameconfig)
{
	sel->selections =
		(cpymo_select_img_selection *)malloc(sizeof(cpymo_select_img_selection) * selections);
	if (sel->selections == NULL) return CPYMO_ERR_SUCC;

	memset(sel->selections, 0, sizeof(cpymo_select_img_selection) * selections);

	if (image_name_or_empty_when_select_imgs.len > 0) {
		error_t err = cpymo_assetloader_load_system_image(
			&sel->select_img_image,
			&sel->select_img_image_w, &sel->select_img_image_h,
			image_name_or_empty_when_select_imgs,
			"png",
			loader,
			cpymo_gameconfig_is_symbian(gameconfig));

		if (err != CPYMO_ERR_SUCC) {
			free(sel->selections);
			sel->selections = NULL;
			return err;
		}
	}

	sel->current_selection = 0;
	sel->all_selections = selections;

	return CPYMO_ERR_SUCC;
}

void cpymo_select_img_configuare_select_img_selection(cpymo_engine *e, float x, float y, bool enabled, uint64_t hash)
{
	assert(e->select_img.selections);
	assert(e->select_img.all_selections);
	assert(e->select_img.select_img_image);

	cpymo_select_img_selection *sel = &e->select_img.selections[e->select_img.current_selection++];
	sel->image = e->select_img.select_img_image;
	sel->or_text = NULL;

	sel->x = x;
	sel->y = y;
	sel->w = e->select_img.select_img_image_w / 2;
	sel->h = e->select_img.select_img_image_h / (int)e->select_img.all_selections;

	sel->enabled = enabled;

	sel->hash = hash;
	sel->has_selected = cpymo_hash_flags_check(&e->flags, hash);
}

error_t cpymo_select_img_configuare_select_imgs_selection(cpymo_engine *e, cpymo_parser_stream_span image_name, float x, float y, bool enabled, uint64_t hash)
{
	assert(e->select_img.selections);
	assert(e->select_img.all_selections);
	assert(e->select_img.select_img_image == NULL);

	cpymo_select_img_selection *sel = &e->select_img.selections[e->select_img.current_selection++];

	error_t err = cpymo_assetloader_load_system_image(
		&sel->image,
		&sel->w,
		&sel->h,
		image_name,
		"png",
		&e->assetloader,
		cpymo_gameconfig_is_symbian(&e->gameconfig)
	);

	if (err != CPYMO_ERR_SUCC) {
		return err;
	}

	sel->or_text = NULL;
	sel->x = x;
	sel->y = y;
	sel->w /= 2;
	sel->enabled = enabled;

	sel->hash = hash;
	sel->has_selected = cpymo_hash_flags_check(&e->flags, hash);

	return CPYMO_ERR_SUCC;
}

static bool cpymo_select_img_wait(struct cpymo_engine *e, float dt)
{
	if (e->select_img.hint[0] != NULL) {
		e->select_img.hint_timer += dt;
		if (e->select_img.hint_timer >= 1.0f) {
			e->select_img.hint_timer -= 1.0f;
			e->select_img.hint_tiktok = !e->select_img.hint_tiktok;
			cpymo_engine_request_redraw(e);
		}
	}

	return e->select_img.selections == NULL;
}

static bool cpymo_select_img_mouse_in_selection(cpymo_select_img *o, int sel, const cpymo_engine *e) {
	assert(sel >= 0 && sel < (int)o->all_selections);

	if (!e->input.mouse_position_useable) return false;
	cpymo_select_img_selection *s = &o->selections[sel];
	
	float x = e->input.mouse_x;
	float y = e->input.mouse_y;

	if (s->image) {
		float left = s->x - (float)s->w / 2.0f;
		float top = s->y - (float)s->h / 2.0f;
		float right = s->x + (float)s->w / 2.0f;
		float bottom = s->y + (float)s->h / 2.0f;

		return x >= left && x <= right && y >= top && y <= bottom;
	}

	if (s->or_text) {
		float left = (float)s->w / 2.0f - (float)o->sel_highlight_w / 2.0f + s->x;
		float w = (float)o->sel_highlight_w;
		float right = left + w;
		float top = s->y - (float)s->h;
		float bottom = s->y;

		return x >= left && x <= right && y >= top && y <= bottom;
	}

	return false;
}


void cpymo_select_img_configuare_end(struct cpymo_engine *e, int init_position)
{
	assert(e->select_img.selections);
	assert(e->select_img.all_selections);
	assert(e->select_img.current_selection == e->select_img.all_selections);

	e->select_img.current_selection = init_position >= 0 ? init_position : 0;
	e->select_img.save_enabled = init_position == -1;

	// In pymo, if all options are disabled, it will enable every option.
	bool all_is_disabled = true;
	for (size_t i = 0; i < e->select_img.all_selections; ++i) {
		if (e->select_img.selections[i].enabled) {
			all_is_disabled = false;
			break;
		}
	}

	if (all_is_disabled)
		for (size_t i = 0; i < e->select_img.all_selections; ++i)
			e->select_img.selections[i].enabled = true;

	if (e->select_img.selections[e->select_img.current_selection].enabled == false) {
		e->select_img.current_selection = 0;
		while (e->select_img.selections[e->select_img.current_selection].enabled == false)
			e->select_img.current_selection++;
	}

	// Trim disabled images.
	for (size_t i = 0; i < e->select_img.all_selections; ++i) {
		if (!e->select_img.selections[i].enabled) {
			if (e->select_img.selections[i].image) {
				if (e->select_img.select_img_image == NULL)
					cpymo_backend_image_free(e->select_img.selections[i].image);
				e->select_img.selections[i].image = NULL;
			}

			if (e->select_img.selections[i].or_text) {
				if (e->select_img.selections[i].or_text)
					cpymo_backend_text_free(e->select_img.selections[i].or_text);
				e->select_img.selections[i].or_text = NULL;
			}
		}
	}

	for (int i = 0; i < (int)e->select_img.all_selections; ++i) {
		if (cpymo_select_img_mouse_in_selection(&e->select_img, i, e)) {
			if (i != e->select_img.current_selection) {
				e->select_img.current_selection = i;
				break;
			}
		}
	}

	cpymo_wait_register(&e->wait, &cpymo_select_img_wait);
	cpymo_engine_request_redraw(e);
}

static void cpymo_select_img_move(cpymo_select_img *o, int move) {
	assert(move == 1 || move == -1);
	
	o->current_selection += move;
	while (o->current_selection < 0) o->current_selection += (int)o->all_selections;
	while (o->current_selection >= (int)o->all_selections) o->current_selection -= (int)o->all_selections;

	if (o->selections[o->current_selection].image == NULL && o->selections[o->current_selection].or_text == NULL)
		cpymo_select_img_move(o, move);
}

static error_t cpymo_select_img_ok(cpymo_engine *e, int sel, uint64_t hash)
{
	const char *out_var = cpymo_gameconfig_is_mo1(&e->gameconfig) ? "F91" : "FSEL";

	if(e->select_img.save_enabled)
		cpymo_hash_flags_add(&e->flags, hash);

	error_t err = cpymo_vars_set(
		&e->vars,
		cpymo_parser_stream_span_pure(out_var),
		sel);
	if (err != CPYMO_ERR_SUCC) return err;

	cpymo_select_img_reset(&e->select_img);
	cpymo_engine_request_redraw(e);

	return err;
}

error_t cpymo_select_img_update(cpymo_engine *e)
{
	cpymo_select_img *o = &e->select_img;

	if (o->selections) {
		if (CPYMO_INPUT_JUST_PRESSED(e, down)) {
			cpymo_select_img_move(o, 1);
			cpymo_engine_request_redraw(e);
		}

		if (CPYMO_INPUT_JUST_PRESSED(e, up)) {
			cpymo_select_img_move(o, -1);
			cpymo_engine_request_redraw(e);
		}

		if (cpymo_input_mouse_moved(e) && e->input.mouse_position_useable) {
			for (int i = 0; i < (int)o->all_selections; ++i) {
				if (cpymo_select_img_mouse_in_selection(o, i, e)) {
					if (i != o->current_selection) {
						o->current_selection = i;
						cpymo_engine_request_redraw(e);
					}
				}
			}
		}

		if (CPYMO_INPUT_JUST_PRESSED(e, ok)) {
			return cpymo_select_img_ok(e, o->current_selection, o->selections[o->current_selection].hash);
		}

		if (CPYMO_INPUT_JUST_PRESSED(e, cancel) && !e->select_img.save_enabled) {
			o->current_selection = (int)o->all_selections - 1;
			while (o->selections[o->current_selection].enabled == false 
					|| (o->selections[o->current_selection].image == NULL
						&& o->selections[o->current_selection].or_text == NULL))
				o->current_selection--;
			return cpymo_select_img_ok(e, o->current_selection, o->selections[o->current_selection].hash);
		}

		if (CPYMO_INPUT_JUST_PRESSED(e, mouse_button)) {
			for (int i = 0; i < (int)o->all_selections; ++i) {
				if (cpymo_select_img_mouse_in_selection(o, i, e)) {
					o->current_selection = i;
					return cpymo_select_img_ok(e, i, o->selections[o->current_selection].hash);
				}
			}
		}
	}

	return CPYMO_ERR_SUCC;
}

void cpymo_select_img_draw(const cpymo_select_img *o, int logical_screen_w, int logical_screen_h, bool gray_seleted)
{
	if (o->selections) {

		if (o->show_option_background && o->option_background) {
			cpymo_backend_image_draw(
				(float)logical_screen_w / 2.0f - (float)o->option_background_w / 2.0f,
				(float)logical_screen_h / 4.0f - (float)o->option_background_h / 2.0f,
				(float)o->option_background_w, (float)o->option_background_h,
				o->option_background,
				0, 0, o->option_background_w, o->option_background_h, 1.0f,
				cpymo_backend_image_draw_type_sel_img);
		}

		for (int i = 0; i < (int)o->all_selections; ++i) {
			const cpymo_select_img_selection *sel = &o->selections[i];
			bool gray = sel->has_selected;
			if (!o->save_enabled || !gray_seleted) gray = false;
			
			if (sel->image) {
				const bool selected = o->current_selection == i;
				cpymo_backend_image_draw(
					sel->x - (float)sel->w / 2.0f,
					sel->y - (float)sel->h / 2.0f,
					(float)sel->w,
					(float)sel->h,
					sel->image,
					selected ? sel->w : 0,
					o->select_img_image ? i * sel->h : 0,
					sel->w,
					sel->h,
					gray ? 0.75f : 1.0f,
					cpymo_backend_image_draw_type_sel_img
				);
			}

			if (sel->or_text) {
				const bool selected = o->current_selection == i;

				if (selected && o->sel_highlight) {
					cpymo_backend_image_draw(
						(float)sel->w / 2.0f - (float)o->sel_highlight_w / 2.0f + sel->x,
						(float)sel->h / 2.0f - (float)o->sel_highlight_h / 2.0f + sel->y - sel->h,
						(float)o->sel_highlight_w,
						(float)o->sel_highlight_h,
						o->sel_highlight,
						0, 0, o->sel_highlight_w, o->sel_highlight_h,
						1.0f,
						cpymo_backend_image_draw_type_sel_img);
				}

				cpymo_backend_text_draw(
					sel->or_text,
					sel->x,
					sel->y,
					(selected && o->sel_highlight == NULL) ? 
						cpymo_color_inv(sel->text_color) : sel->text_color,
					gray ? 0.75f : 1.0f,
					cpymo_backend_image_draw_type_sel_img
				);

			}
		}

		if (o->hint[0] != NULL) {
			int hint_state = o->selections[o->current_selection].hint_state;

			if (hint_state != cpymo_select_img_selection_nohint) {
				int cur_img = 0;
				if (hint_state == cpymo_select_img_selection_hint23) cur_img = 2;
				if (o->hint_tiktok) cur_img++;

				float hint_y = 0.0f;
				if(o->show_option_background && o->option_background)
					hint_y = (float)logical_screen_h / 4.0f - (float)o->option_background_h / 2.0f;

				cpymo_backend_image_draw(
					0,
					hint_y,
					(float)o->hint_w[cur_img],
					(float)o->hint_h[cur_img],
					o->hint[cur_img],
					0,
					0,
					o->hint_w[cur_img],
					o->hint_h[cur_img],
					1.0f,
					cpymo_backend_image_draw_type_sel_img);
			}
		}
	}
}

error_t cpymo_select_img_configuare_select_text(cpymo_engine *e, cpymo_parser_stream_span text, bool enabled, enum cpymo_select_img_selection_hint_state hint_mode, uint64_t hash)
{
	if (e->select_img.sel_highlight == NULL) {
		error_t err = cpymo_assetloader_load_system_image(
			&e->select_img.sel_highlight,
			&e->select_img.sel_highlight_w,
			&e->select_img.sel_highlight_h,
			cpymo_parser_stream_span_pure("sel_highlight"),
			"png",
			&e->assetloader,
			cpymo_gameconfig_is_symbian(&e->gameconfig));

		if (err != CPYMO_ERR_SUCC) e->select_img.sel_highlight = NULL;
	}

	assert(e->select_img.selections);
	assert(e->select_img.all_selections);
	assert(e->select_img.select_img_image == NULL);

	const float fontsize = cpymo_gameconfig_font_size(&e->gameconfig);

	cpymo_select_img_selection *sel = &e->select_img.selections[e->select_img.current_selection++];
	float text_width;
	error_t err =
		cpymo_backend_text_create(
			&sel->or_text,
			&text_width,
			text,
			fontsize);
	
	CPYMO_THROW(err);


	sel->image = NULL;
	sel->enabled = enabled;
	sel->h = (int)fontsize + 1;
	sel->w = (int)text_width + 1;
	sel->hint_state = hint_mode;
	sel->hash = hash;
	sel->has_selected = cpymo_hash_flags_check(&e->flags, hash);

	return CPYMO_ERR_SUCC;
}

void cpymo_select_img_configuare_select_text_hint_pic(cpymo_engine * engine, cpymo_parser_stream_span hint)
{
	if (engine->gameconfig.hint == 0) return;

	assert(engine->select_img.hint[0] == NULL);
	assert(engine->select_img.hint[1] == NULL);
	assert(engine->select_img.hint[2] == NULL);
	assert(engine->select_img.hint[3] == NULL);

	engine->select_img.hint_timer = 0;
	engine->select_img.hint_tiktok = false;

	char *hint_pic_name = (char *)malloc(hint.len + 2);
	if (hint_pic_name == NULL) return;

	bool is_all_succ = true;
	for (size_t i = 0; i < 4; ++i) {
		cpymo_parser_stream_span_copy(hint_pic_name, hint.len + 2, hint);
		hint_pic_name[hint.len] = (char)('0' + i);
		hint_pic_name[hint.len + 1] = '\0';

		error_t err = cpymo_assetloader_load_system_image(
			&engine->select_img.hint[i],
			&engine->select_img.hint_w[i],
			&engine->select_img.hint_h[i],
			cpymo_parser_stream_span_pure(hint_pic_name),
			"png",
			&engine->assetloader,
			cpymo_gameconfig_is_symbian(&engine->gameconfig)
		);
		
		if (err != CPYMO_ERR_SUCC) {
			is_all_succ = false;
			engine->select_img.hint[i] = NULL;
			break;
		}
	}

	free(hint_pic_name);

	if (is_all_succ) return;
	else {
		for (size_t i = 0; i < 4; ++i) {
			if (engine->select_img.hint[i] != NULL) {
				cpymo_backend_image_free(engine->select_img.hint[i]);
				engine->select_img.hint[i] = NULL;
			}
		}
	}
}

void cpymo_select_img_configuare_end_select_text(cpymo_engine * e, float x1, float y1, float x2, float y2, cpymo_color col, int init_pos, bool show_option_background)
{
	float box_w = x2 - x1;
	float y = y1;
	for (size_t i = 0; i < e->select_img.all_selections; ++i) {
		cpymo_select_img_selection *sel = &e->select_img.selections[i];
		sel->text_color = col;
		sel->x = (box_w - sel->w) / 2 + x1;
		sel->y = y + sel->h;	// y is baseline.
		if(sel->enabled) y += sel->h;
	}

	float y_offset = (y2 - y) / 2;

	for (size_t i = 0; i < e->select_img.all_selections; ++i)
		e->select_img.selections[i].y += y_offset;

	e->select_img.show_option_background = show_option_background;

	cpymo_select_img_configuare_end(e, init_pos);

	// load option background if not load.
	if (e->select_img.option_background == NULL && show_option_background) {
		error_t err = cpymo_assetloader_load_system_image(
			&e->select_img.option_background,
			&e->select_img.option_background_w,
			&e->select_img.option_background_h,
			cpymo_parser_stream_span_pure("option"),
			"png",
			&e->assetloader,
			cpymo_gameconfig_is_symbian(&e->gameconfig));

		if (err != CPYMO_ERR_SUCC) {
			e->select_img.option_background = NULL;
			e->select_img.show_option_background = false;
		}
	}
}
