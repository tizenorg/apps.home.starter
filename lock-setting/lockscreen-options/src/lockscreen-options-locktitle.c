 /*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.tizenopensource.org/license
  *
  * Unless required by applicable law or agreed to in writing, software
  * distributed under the License is distributed on an "AS IS" BASIS,
  * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  * See the License for the specific language governing permissions and
  * limitations under the License.
  */



#include <Ecore_X.h>

#include "lockscreen-options.h"
#include "lockscreen-options-debug.h"
#include "lockscreen-options-util.h"
#include "lockscreen-options-locktitle.h"

#define EDJE_DIR "/usr/ug/res/edje/ug-lockscreen-options-efl"

typedef struct {
	unsigned char red;
	unsigned char green;
	unsigned char blue;
	unsigned char reserved;
} options_locktitle_rgb_s;

typedef struct {
	lockscreen_options_ug_data *ug_data;
	Evas_Object *layout;
	Evas_Object *toolbar;
	Evas_Object *entry;
	Ecore_IMF_Context *entry_ctx;
	Evas_Object *colorselector;
	Evas_Object *fontselector;
	Evas_Object *font_radio;

	options_locktitle_rgb_s rgb;
} options_locktile_view_s;

static options_locktile_view_s *options_locktile_view_data = NULL;

static const fonttype_size = 5;
static const char *fonttype_value[] = {
	"Arial", "Courier New", "Roboto", "Tahoma", "Verdana"
};

static char *_fonttype_dropdown_text_get(void *data, Evas_Object * obj, const char *part);
static Evas_Object *_fonttype_dropdown_content_get(void *data, Evas_Object * obj, const char *part);
static Evas_Object *_editfield_content_get(void *data, Evas_Object * obj, const char *part);
static void _lockscreen_options_locktitle_font_selector_create();
static void _lockscreen_options_locktitle_color_selector_create();

static Elm_Genlist_Item_Class itc_fontlist_style = {
	.item_style = "1text.1icon.2",
	.func.text_get = _fonttype_dropdown_text_get,
	.func.content_get = _fonttype_dropdown_content_get,
};

static Elm_Genlist_Item_Class itc_editfield_style = {
	.item_style = "dialogue/1icon",
	.func.text_get = NULL,
	.func.content_get = _editfield_content_get,
};

static char *_fonttype_dropdown_text_get(void *data, Evas_Object * obj,
					     const char *part)
{
	int index = (int)data;
	if (strcmp(part, "elm.text") == 0) {
		return strdup(fonttype_value[index]);
	}
	return NULL;
}

static Evas_Object *_fonttype_dropdown_content_get(void *data,
						       Evas_Object * obj,
						       const char *part)
{
	int index = (int)data;
	if (!strcmp(part, "elm.icon")) {
		Evas_Object *radio = elm_radio_add(obj);
		elm_radio_state_value_set(radio, index);
		elm_radio_group_add(radio, options_locktile_view_data->font_radio);
		return radio;
	}
	return NULL;
}

static void *_fonttype_sel(void *data, Evas_Object * obj, void *event_info)
{
	//select font type
}

static void _lockscreen_options_locktitle_delete_cb(void *data, Evas * e,
						    Evas_Object * obj,
						    void *event_info)
{
	if (options_locktile_view_data) {
		free(options_locktile_view_data);
		options_locktile_view_data = NULL;
	}
}

static void _lockscreen_options_locktitle_back_cb(void *data, Evas_Object * obj,
						  void *event_info)
{
	lockscreen_options_ug_data *ug_data =
	    (lockscreen_options_ug_data *) data;

	if (ug_data == NULL) {
		LOCKOPTIONS_ERR("ug_data is null.");
		return;
	}

	Evas_Object *navi_bar = ug_data->navi_bar;

	if (navi_bar == NULL) {
		LOCKOPTIONS_ERR("navi_bar is null.");
		return;
	}
	elm_naviframe_item_pop(navi_bar);
}

static void _send_int_message_to_edc(Evas_Object * obj, int msgID, int param)
{
	Edje_Message_Int msg;
	msg.val = param;
	edje_object_message_send(elm_layout_edje_get(obj),
				 EDJE_MESSAGE_INT, msgID, &msg);
}

static void _send_float_message_to_edc(Evas_Object * obj, int msgID, float param)
{
	Edje_Message_Float msg;
	msg.val = param;
	edje_object_message_send(elm_layout_edje_get(obj),
				 EDJE_MESSAGE_FLOAT, msgID, &msg);
}

static void _lockscreen_options_locktitle_entry_imf_state_cb(void *data,
							     Ecore_IMF_Context *
							     ctx, int value)
{
	static float val = 1.0;
	if (value == ECORE_IMF_INPUT_PANEL_STATE_SHOW) {
		Evas_Coord keypad_x = 0;
		Evas_Coord keypad_y = 0;
		Evas_Coord keypad_w = 0;
		Evas_Coord keypad_h = 0;
		Evas_Coord window_width = 0;
		Evas_Coord window_height = 0;

		ecore_x_window_size_get(ecore_x_window_root_first_get(),
					&window_width, &window_height);
		ecore_imf_context_input_panel_geometry_get(ctx, &keypad_x,
							   &keypad_y, &keypad_w,
							   &keypad_h);
		val = (float)(keypad_y) / (float)window_height;
		/*set the toolbar's location*/
		_send_float_message_to_edc(options_locktile_view_data->layout, 1, val);
		/*set toolbar visible*/
		_send_float_message_to_edc(options_locktile_view_data->layout, 2, 0.0);
	} else if (value == ECORE_IMF_INPUT_PANEL_STATE_HIDE
					&&options_locktile_view_data->colorselector == NULL
					&&options_locktile_view_data->fontselector == NULL) {
		_send_float_message_to_edc(options_locktile_view_data->layout, 3, 0.0);
	}else {
		/*set the toolbar's location*/
		_send_float_message_to_edc(options_locktile_view_data->layout, 1, val);
	}
}

static void _lockscreen_options_locktitle_entry_imf_resize_cb(void *data,
							      Ecore_IMF_Context
							      * ctx, int value)
{
	Evas_Coord keypad_x = 0;
	Evas_Coord keypad_y = 0;
	Evas_Coord keypad_w = 0;
	Evas_Coord keypad_h = 0;
	Evas_Coord window_width = 0;
	Evas_Coord window_height = 0;

	ecore_x_window_size_get(ecore_x_window_root_first_get(), &window_width,
				&window_height);
	ecore_imf_context_input_panel_geometry_get(ctx, &keypad_x, &keypad_y,
					   &keypad_w, &keypad_h);
}

/* This callback is for showing(hiding) X marked button.*/
static void _changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (elm_object_focus_get(data)) {
		if (elm_entry_is_empty(obj)) {
			elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
		}else {
			elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
		}
	}
}

/* Focused callback will show X marked button and hide guidetext.*/
static void _focused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (!elm_entry_is_empty(obj)) {
		elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}
	elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
}

/*Unfocused callback will show guidetext and hide X marked button.*/
static void _unfocused_cb(void *data, Evas_Object *obj, void *event_info)
{
	if (elm_entry_is_empty(obj)) {
		elm_object_signal_emit(data, "elm,state,guidetext,show", "elm");
	}
	elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
}

static void _eraser_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source) // When X marked button clicked, make string as empty.
{
	elm_entry_entry_set(data, "");
}

static Evas_Object *_lockscreen_options_editfield_create(Evas_Object *parent)
{
	LOCKOPTIONS_DBG("[ == %s == ]", __func__);
	Evas_Object *layout = NULL;
	Evas_Object *entry = NULL;

	layout = elm_layout_add(parent);
	if(layout == NULL)
		return NULL;
	elm_layout_theme_set(layout, "layout", "editfield", "default");
	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);

	entry = elm_entry_add(parent);
	if(entry == NULL)
		return NULL;
	evas_object_smart_callback_add(entry, "changed", _changed_cb, layout);
	evas_object_smart_callback_add(entry, "focused", _focused_cb, layout);
	evas_object_smart_callback_add(entry, "unfocused", _unfocused_cb, layout);

	elm_object_part_content_set(layout, "elm.swallow.content", entry);
	elm_object_part_text_set(layout, "elm.guidetext", lockscreen_optoins_get_string(IDS_LOCKSCREEN_OPTIONS_LOCK_SCREEN_TITLE_GUIDE_TEXT));
	elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm", _eraser_clicked_cb, entry);

	options_locktile_view_data->entry = entry;

	Ecore_IMF_Context *entry_ctx = elm_entry_imf_context_get(entry);
	if(entry_ctx != NULL) {
		ecore_imf_context_input_panel_event_callback_add((Ecore_IMF_Context *)
							entry_ctx,
							ECORE_IMF_INPUT_PANEL_STATE_EVENT,
							_lockscreen_options_locktitle_entry_imf_state_cb,
							options_locktile_view_data);
		ecore_imf_context_input_panel_event_callback_add((Ecore_IMF_Context *)
							entry_ctx,
							ECORE_IMF_INPUT_PANEL_GEOMETRY_EVENT,
							_lockscreen_options_locktitle_entry_imf_resize_cb,
							options_locktile_view_data);
		options_locktile_view_data->entry_ctx = entry_ctx;
	}

	return layout;
}

static Evas_Object *_editfield_content_get(void *data,
						       Evas_Object * obj,
						       const char *part)
{
	if (!strcmp(part, "elm.icon")) {
		LOCKOPTIONS_DBG("[ == %s == ]", __func__);
		Evas_Object *editfield = NULL;
		editfield = _lockscreen_options_editfield_create(obj);
		return editfield;
	}
	return NULL;
}

static void *_editfield_sel(void *data, Evas_Object * obj, void *event_info)
{
	elm_genlist_item_selected_set((Elm_Object_Item *)event_info, EINA_FALSE);
}

static void _on_text_fonttype_btn_clicked_cb(void *data, Evas_Object * obj,
					       const char *emission,
					       const char *source)
{
	if (options_locktile_view_data != NULL) {
		if(options_locktile_view_data->fontselector == NULL) {
			if(options_locktile_view_data->colorselector != NULL) {
				evas_object_del(options_locktile_view_data->colorselector);
				options_locktile_view_data->colorselector = NULL;
			}
			_lockscreen_options_locktitle_font_selector_create();
			/*if create fontselector successfully, hide IME*/
			if(options_locktile_view_data->fontselector != NULL) {
				elm_object_focus_set(options_locktile_view_data->entry, EINA_FALSE);
			}
		}else {
			evas_object_del(options_locktile_view_data->fontselector);
			options_locktile_view_data->fontselector = NULL;
			elm_object_focus_set(options_locktile_view_data->entry, EINA_TRUE);
		}
	}
}

static void _on_text_draw_btn_clicked_cb(void *data, Evas_Object * obj,
					       const char *emission,
					       const char *source)
{
	if (options_locktile_view_data != NULL) {
		if(options_locktile_view_data->colorselector == NULL) {
			if(options_locktile_view_data->fontselector != NULL) {
				evas_object_del(options_locktile_view_data->fontselector);
				options_locktile_view_data->fontselector = NULL;
			}
			_lockscreen_options_locktitle_color_selector_create();
			/*if create colorselector successfully, hide IME*/
			if(options_locktile_view_data->colorselector != NULL) {
				elm_object_focus_set(options_locktile_view_data->entry, EINA_FALSE);
			}
		}else {
			evas_object_del(options_locktile_view_data->colorselector);
			options_locktile_view_data->colorselector = NULL;
			elm_object_focus_set(options_locktile_view_data->entry, EINA_TRUE);
		}
	}
}

static void _on_text_bold_btn_clicked_cb(void *data, Evas_Object * obj,
					  const char *emission,
					  const char *source)
{
	if (options_locktile_view_data != NULL) {
		_send_int_message_to_edc(options_locktile_view_data->toolbar, 1, 1);
	}
}

static void _on_text_italic_btn_clicked_cb(void *data, Evas_Object * obj,
					    const char *emission,
					    const char *source)
{
	if (options_locktile_view_data != NULL) {
		_send_int_message_to_edc(options_locktile_view_data->toolbar, 1, 2);
	}

}

static void _on_text_underline_btn_clicked_cb(void *data, Evas_Object * obj,
					       const char *emission,
					       const char *source)
{
	if (options_locktile_view_data != NULL) {
		_send_int_message_to_edc(options_locktile_view_data->toolbar, 1, 3);
	}
}

static void _lockscreen_options_locktitle_colorselector_changed_cb(void *data, Evas_Object *obj, void *event_info)
{
	Evas_Object *rect = (Evas_Object *)data;
	int r = 0, g = 0, b = 0, a = 0;
	elm_colorselector_color_get(obj, &r, &g, &b, &a);
	evas_object_color_set(rect, r, g, b, a);
}

static void _lockscreen_options_locktitle_color_selector_create()
{
	if(options_locktile_view_data == NULL)
		return;
	Evas_Object *colorselector = NULL;
	Evas_Object *rect = NULL;
	Evas_Object *color = NULL;

	colorselector =
			lockscreen_options_util_create_layout(options_locktile_view_data->layout,
							  EDJE_DIR
							  "/lockscreen-options.edj",
							  "lockscreen.options.locktitle.colorselector");
	if(colorselector == NULL)
		return;

	rect = evas_object_rectangle_add(evas_object_evas_get(options_locktile_view_data->layout));
	elm_object_part_content_set(colorselector, "rect", rect);
	evas_object_color_set(rect, options_locktile_view_data->rgb.red, options_locktile_view_data->rgb.green, options_locktile_view_data->rgb.blue, options_locktile_view_data->rgb.reserved);

	color = elm_colorselector_add(options_locktile_view_data->layout);
	elm_colorselector_mode_set(color, ELM_COLORSELECTOR_COMPONENTS);
	elm_object_part_content_set(colorselector, "color", color);
	evas_object_smart_callback_add(color, "changed", _lockscreen_options_locktitle_colorselector_changed_cb, rect);
	evas_object_color_set(color, options_locktile_view_data->rgb.red, options_locktile_view_data->rgb.green, options_locktile_view_data->rgb.blue, options_locktile_view_data->rgb.reserved);

	options_locktile_view_data->colorselector = colorselector;
	elm_object_part_content_set(options_locktile_view_data->layout, "elm.swallow.selector", colorselector);
	evas_object_show(colorselector);
}

static void _lockscreen_options_locktitle_font_selector_create()
{
	if(options_locktile_view_data == NULL)
		return;
	Evas_Object *fontselector = NULL;
	Evas_Object *font_radio = NULL;

	fontselector = elm_genlist_add(options_locktile_view_data->layout);
	if(fontselector == NULL)
		return;

	font_radio= elm_radio_add(fontselector);
	if(font_radio == NULL)
		return;

	int index = 0;
	for (index = 0; index < fonttype_size; index++) {
		elm_genlist_item_append(fontselector, &itc_fontlist_style,
				(void *)index, NULL,
				ELM_GENLIST_ITEM_NONE, _fonttype_sel, NULL);
	}

	options_locktile_view_data->fontselector = fontselector;
	options_locktile_view_data->font_radio = font_radio;
	elm_object_part_content_set(options_locktile_view_data->layout, "elm.swallow.selector", fontselector);
	evas_object_show(fontselector);
}

static void _lockscreen_options_locktitle_create_toolbar()
{
	if(options_locktile_view_data == NULL)
		return;

	Evas_Object *toolbar = NULL;

	toolbar =
			lockscreen_options_util_create_layout(options_locktile_view_data->layout,
							  EDJE_DIR
							  "/lockscreen-options.edj",
							  "lockscreen.options.locktitle.toolbar");
	if(toolbar == NULL)
		return;

	elm_object_part_content_set(options_locktile_view_data->layout, "elm.swallow.toolbar", toolbar);
	evas_object_show(toolbar);
	options_locktile_view_data->toolbar = toolbar;

	edje_object_signal_callback_add(elm_layout_edje_get(toolbar),
				"dropdown.fonttype.clicked", "*",
				_on_text_fonttype_btn_clicked_cb,
				NULL);
	edje_object_signal_callback_add(elm_layout_edje_get(toolbar),
				"draw.button.clicked", "*",
				_on_text_draw_btn_clicked_cb, NULL);
	edje_object_signal_callback_add(elm_layout_edje_get(toolbar),
				"text.bold.clicked", "*",
				_on_text_bold_btn_clicked_cb, NULL);
	edje_object_signal_callback_add(elm_layout_edje_get(toolbar),
				"text.italic.clicked", "*",
				_on_text_italic_btn_clicked_cb, NULL);
	edje_object_signal_callback_add(elm_layout_edje_get(toolbar),
				"text.underline.clicked", "*",
				_on_text_underline_btn_clicked_cb, NULL);
}

void lockscreen_options_locktitle_create_view(lockscreen_options_ug_data *
					      ug_data)
{
	if (ug_data == NULL) {
		LOCKOPTIONS_ERR("ug is NULL");
		return;
	}
	LOCKOPTIONS_DBG("lockscreen_options_locktitle_create_view begin\n");
	Evas_Object *navi_bar = ug_data->navi_bar;
	if (navi_bar == NULL) {
		LOCKOPTIONS_ERR("navi_bar is null.");
		return;
	}

	/*initialization*/
	options_locktile_view_data =
	    (options_locktile_view_s *) calloc(1,
					       sizeof(options_locktile_view_s));
	options_locktile_view_data->ug_data = ug_data;
	options_locktile_view_data->rgb.red = 255;
	options_locktile_view_data->rgb.green = 255;
	options_locktile_view_data->rgb.blue = 255;
	options_locktile_view_data->rgb.reserved = 255;

	Evas_Object *layout = NULL;
	Evas_Object *back_button = NULL;
	Evas_Object *genlist = NULL;

	layout =
	    lockscreen_options_util_create_layout(navi_bar,
						  EDJE_DIR
						  "/lockscreen-options.edj",
						  "lockscreen.options.locktitle.main");

	if (layout == NULL) {
		LOCKOPTIONS_ERR("can't create locktitle layout.");
		return;
	}

	evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL,
				       _lockscreen_options_locktitle_delete_cb,
				       NULL);
	options_locktile_view_data->layout = layout;

	genlist = elm_genlist_add(layout);
	if(genlist == NULL) {
		return;
	}
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_genlist_item_append(genlist, &itc_editfield_style, NULL, NULL, ELM_GENLIST_ITEM_NONE, _editfield_sel, NULL);
	elm_object_part_content_set(layout, "elm.swallow.content", genlist);
	evas_object_show(genlist);

	_lockscreen_options_locktitle_create_toolbar();

	back_button = elm_button_add(navi_bar);
	elm_object_style_set(back_button, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_button, "clicked",
				       _lockscreen_options_locktitle_back_cb,
				       ug_data);

	elm_naviframe_item_push(navi_bar, "Lock screen title", back_button,
				NULL, layout, NULL);
}
