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



#include <appcore-efl.h>
#include <Ecore_X.h>
#include <vconf.h>
#include <vconf-keys.h>

#include "openlock-setting-pw.h"
#include "openlock-setting-debug.h"
#include "starter-vconf.h"

static Evas_Object *done_button = NULL;
static Elm_Gen_Item_Class itc_label, itc_entry;
static Evas_Object *_openlock_setting_pw_editfield_create(Evas_Object * parent,
							  void *data);
static Evas_Object *_openlock_setting_pw_editfield_entry_get(Evas_Object *
							     parent);

static Evas_Object *_openlock_setting_pw_create_conformant(Evas_Object * parent)
{
	Evas_Object *conform = NULL;
	elm_win_conformant_set(parent, 1);
	conform = elm_conformant_add(parent);
	elm_object_style_set(conform, "internal_layout");
	evas_object_show(conform);

	return conform;
}

static void _openlock_setting_pw_done_button_changed(void *data,
						     Evas_Object * obj, void *e)
{
	openlock_setting_appdata *openlock_setting_data =
	    (openlock_setting_appdata *) data;

	if (!openlock_setting_data) {
		return;
	}
	int length = strlen(elm_entry_entry_get(obj));

	if (length == 0) {
		elm_object_disabled_set(done_button, EINA_TRUE);
	} else {
		elm_object_disabled_set(done_button, EINA_FALSE);
	}

	if (elm_object_focus_get(obj)) {
		if (elm_entry_is_empty(obj)) {
			elm_object_signal_emit
			    (openlock_setting_data->editfield_layout,
			     "elm,state,eraser,hide", "elm");
		} else {
			elm_object_signal_emit
			    (openlock_setting_data->editfield_layout,
			     "elm,state,eraser,show", "elm");
		}
	}
}

static void _openlock_setting_pw_focused_cb(void *data, Evas_Object * obj,
					    void *event_info)
{
	if (!elm_entry_is_empty(obj)) {
		elm_object_signal_emit(data, "elm,state,eraser,show", "elm");
	}
	elm_object_signal_emit(data, "elm,state,guidetext,hide", "elm");
}

static void _openlock_setting_pw_unfocused_cb(void *data, Evas_Object * obj,
					      void *event_info)
{
	if (elm_entry_is_empty(obj)) {
		elm_object_signal_emit(data, "elm,state,guidetext,show", "elm");
	}
	elm_object_signal_emit(data, "elm,state,eraser,hide", "elm");
}

static void _openlock_setting_pw_eraser_clicked_cb(void *data,
						   Evas_Object * obj,
						   const char *emission,
						   const char *source)
{
	elm_entry_entry_set(data, "");
}

static Evas_Object *_openlock_setting_pw_editfield_create(Evas_Object * parent,
							  void *data)
{
	Evas_Object *layout = NULL;
	Evas_Object *entry = NULL;
	openlock_setting_appdata *openlock_setting_data =
	    (openlock_setting_appdata *) data;
	static Elm_Entry_Filter_Limit_Size limit_filter_data;

	limit_filter_data.max_char_count = 15;	/* hard code for demo */
	limit_filter_data.max_byte_count = 0;

	layout = elm_layout_add(parent);
	elm_layout_theme_set(layout, "layout", "editfield", "title");

	entry = elm_entry_add(parent);
	elm_entry_scrollable_set(entry, EINA_TRUE);
	elm_entry_single_line_set(entry, EINA_TRUE);
	elm_entry_password_set(entry, EINA_TRUE);
	elm_entry_input_panel_layout_set(entry,
					 ELM_INPUT_PANEL_LAYOUT_NUMBERONLY);
	elm_entry_markup_filter_append(entry, elm_entry_filter_limit_size,
				       &limit_filter_data);
	elm_object_focus_set(entry, EINA_TRUE);
	evas_object_show(entry);
	evas_object_smart_callback_add(entry, "changed",
				       _openlock_setting_pw_done_button_changed,
				       openlock_setting_data);
	evas_object_smart_callback_add(entry, "focused",
				       _openlock_setting_pw_focused_cb, layout);
	evas_object_smart_callback_add(entry, "unfocused",
				       _openlock_setting_pw_unfocused_cb,
				       layout);

	elm_object_part_content_set(layout, "elm.swallow.content", entry);
	elm_object_part_text_set(layout, "elm.guidetext", "");
	elm_object_signal_callback_add(layout, "elm,eraser,clicked", "elm",
				       _openlock_setting_pw_eraser_clicked_cb,
				       entry);

	Ecore_IMF_Context *imf_context =
			(Ecore_IMF_Context *) elm_entry_imf_context_get(entry);
	if (imf_context) {
		ecore_imf_context_input_panel_show(imf_context);
	}
	elm_object_focus_set(entry, EINA_TRUE);

	return layout;
}

static Evas_Object *_openlock_setting_pw_editfield_entry_get(Evas_Object *
							     parent)
{
	Evas_Object *entry = NULL;

	entry = elm_object_part_content_get(parent, "elm.swallow.content");

	return entry;
}

static char *_openlock_setting_pw_gl_label_get_title(void *data,
						     Evas_Object * obj,
						     const char *part)
{
	openlock_setting_appdata *openlock_setting_data =
	    (openlock_setting_appdata *) data;
	char buf[50] = { 0, };

	if (!openlock_setting_data || !part) {
		return NULL;
	}

	if (!strcmp(part, "elm.text")) {
		snprintf(buf, sizeof(buf), "%s", "Enter Password");	/* hard code for demo */
		return strdup(buf);
	}
	return NULL;
}

static Evas_Object *_openlock_setting_pw_gl_icon_get(void *data,
						     Evas_Object * obj,
						     const char *part)
{
	Evas_Object *layout = NULL;
	openlock_setting_appdata *openlock_setting_data =
	    (openlock_setting_appdata *) data;

	if (!openlock_setting_data || !part) {
		return NULL;
	}

	if (!strcmp(part, "elm.icon")) {
		layout =
		    _openlock_setting_pw_editfield_create(obj,
							  openlock_setting_data);
		openlock_setting_data->editfield_layout = layout;

		return layout;

	}
	return NULL;
}

static void _openlock_setting_pw_list_set_styles()
{
	itc_label.item_style = "dialogue/title";
	itc_label.func.text_get = _openlock_setting_pw_gl_label_get_title;
	itc_label.func.content_get = NULL;
	itc_label.func.state_get = NULL;
	itc_label.func.del = NULL;

	itc_entry.item_style = "dialogue/1icon";
	itc_entry.func.text_get = NULL;
	itc_entry.func.content_get = _openlock_setting_pw_gl_icon_get;
	itc_entry.func.state_get = NULL;
	itc_entry.func.del = NULL;
}

static void _openlock_setting_pw_back_cb(void *data, Evas_Object * obj,
					 void *event_info)
{
	openlock_setting_appdata *openlock_setting_data =
	    (openlock_setting_appdata *) data;
	openlock_ug_data *openlock_data = NULL;

	if (!openlock_setting_data) {
		return;
	}

	openlock_data = openlock_setting_data->openlock_data;
	if (!openlock_data) {
		return;
	}
	OPENLOCKS_DBG("_openlock_setting_pw_back_cb\n");
	openlock_setting_data->count = 0;

	elm_naviframe_item_pop(openlock_data->navi_bar);
	if (openlock_setting_data->editfield_layout) {
		evas_object_del(openlock_setting_data->editfield_layout);
		openlock_setting_data->editfield_layout = NULL;
	}
	if (openlock_setting_data->genlist) {
		evas_object_del(openlock_setting_data->genlist);
		openlock_setting_data->genlist = NULL;
	}
	if (openlock_setting_data->ly) {
		evas_object_del(openlock_setting_data->ly);
		openlock_setting_data->ly = NULL;
	}
}

static Eina_Bool _openlock_setting_pw_input_panel_show_idler(void *data)
{
	Evas_Object *entry = (Evas_Object *) data;

	OPENLOCKS_DBG("_openlock_setting_pw_input_panel_show_idler");

	elm_object_focus_set(entry, EINA_TRUE);

	return ECORE_CALLBACK_CANCEL;
}

static void _openlock_setting_pw_imf_context_input_panel_show(void *data)
{
	openlock_setting_appdata *openlock_setting_data =
			(openlock_setting_appdata *) data;
	Evas_Object *entry = NULL;

	if (!openlock_setting_data) {
		return;
	}

	OPENLOCKS_DBG("_openlock_setting_pw_imf_context_input_panel_show\n");

	entry = _openlock_setting_pw_editfield_entry_get(
			openlock_setting_data->editfield_layout);
	evas_object_show(entry);

	ecore_idler_add(_openlock_setting_pw_input_panel_show_idler, entry);
}

static void _openlock_setting_pw_imf_context_input_panel_hide(void *data)
{
	openlock_setting_appdata *openlock_setting_data =
			(openlock_setting_appdata *) data;
	Evas_Object *entry = NULL;

	if (!openlock_setting_data) {
		return;
	}

	OPENLOCKS_DBG("_openlock_setting_pw_imf_context_input_panel_hide\n");

	entry = _openlock_setting_pw_editfield_entry_get(
			openlock_setting_data->editfield_layout);
	evas_object_hide(entry);

	elm_object_focus_set(entry, EINA_FALSE);
}

static void _openlock_setting_pw_destroy_popup_cb(void *data, Evas_Object * obj,
						  void *e)
{
	openlock_setting_appdata *openlock_setting_data =
	    (openlock_setting_appdata *) data;
	Evas_Object *popup = NULL;

	OPENLOCKS_DBG("_openlock_setting_pw_destroy_popup_cb\n");
	_openlock_setting_pw_imf_context_input_panel_show
	    (openlock_setting_data);

	if (!openlock_setting_data) {
		return;
	}

	popup = openlock_setting_data->popup;
	if (popup) {
		evas_object_del(popup);
		popup = NULL;
	}
}

static void _openlock_setting_pw_show_popup(void *data, const char *mesg)
{
	openlock_setting_appdata *openlock_setting_data =
	    (openlock_setting_appdata *) data;
	openlock_ug_data *openlock_data = NULL;
	Evas_Object *popup = NULL;

	if (!openlock_setting_data) {
		return;
	}
	openlock_data = openlock_setting_data->openlock_data;
	if (!openlock_data) {
		return;
	}

	OPENLOCKS_DBG("_openlock_setting_pw_show_popup\n");

	_openlock_setting_pw_imf_context_input_panel_hide
	    (openlock_setting_data);
	popup = elm_popup_add(openlock_data->win_main);
	openlock_setting_data->popup = popup;
	evas_object_size_hint_weight_set(popup, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	elm_object_text_set(popup, mesg);
	elm_popup_timeout_set(popup, 3);
	evas_object_smart_callback_add(popup, "timeout",
				       _openlock_setting_pw_destroy_popup_cb,
				       openlock_setting_data);
	evas_object_show(popup);
}

static void _openlock_setting_pw_set_str(char **s, const char *str)
{
	if (s == NULL)
		return;

	if (*s)
		free(*s);

	if (str && str[0] != '\0')
		*s = strdup(str);
	else
		*s = NULL;
}

static void _openlock_setting_pw_done_cb(void *data, Evas_Object * obj, void *e)
{
	openlock_setting_appdata *openlock_setting_data =
	    (openlock_setting_appdata *) data;
	openlock_ug_data *openlock_data = NULL;
	Evas_Object *entry = NULL;
	Evas_Object *editfield_layout = NULL;
	char *str = NULL;

	if (!openlock_setting_data) {
		return;
	}

	OPENLOCKS_DBG("_openlock_setting_pw_done_cb\n");
	editfield_layout = openlock_setting_data->editfield_layout;
	entry =
	    _openlock_setting_pw_editfield_entry_get
	    (openlock_setting_data->editfield_layout);

	_openlock_setting_pw_set_str(&str, elm_entry_entry_get(entry));
	OPENLOCKS_DBG("str = %s\n", str);
	if (!str) {
		_openlock_setting_pw_show_popup(openlock_setting_data, "Wrong Password!");	/* hard code for demo */
		if (entry) {
			elm_object_part_text_set(editfield_layout,
						 "elm.guidetext", "");
			elm_entry_entry_set(entry, "");
		}
	} else {
		if (strcmp(str, "16777216") == 0) {	/* hard code for demo */
			OPENLOCKS_DBG("right pw\n");
			OPENLOCKS_DBG("openlock_setting_data->index: %d",
				      openlock_setting_data->index);
			if (openlock_setting_data != NULL
			    && openlock_setting_data->pkg_name != NULL) {
				vconf_set_str(VCONF_PRIVATE_LOCKSCREEN_PKGNAME,
					      openlock_setting_data->pkg_name);
				OPENLOCKS_DBG("vconf pkgname set : %s",
					      openlock_setting_data->pkg_name);
			}
			openlock_data = openlock_setting_data->openlock_data;
			if (!openlock_data) {
				return;
			}
			OPENLOCKS_DBG("_openlock_setting_pw_done_cb\n");
			openlock_setting_data->count = 0;	/* reset the count */

			elm_naviframe_item_pop(openlock_data->navi_bar);
			if (openlock_setting_data->editfield_layout) {
				evas_object_del
				    (openlock_setting_data->editfield_layout);
				openlock_setting_data->editfield_layout = NULL;
			}
			if (openlock_setting_data->genlist) {
				evas_object_del(openlock_setting_data->genlist);
				openlock_setting_data->genlist = NULL;
			}
			if (openlock_setting_data->ly) {
				evas_object_del(openlock_setting_data->ly);
				openlock_setting_data->ly = NULL;
			}
		} else {
			_openlock_setting_pw_show_popup(openlock_setting_data, "Wrong Password!");	/* hard code for demo */
			if (entry) {
				elm_object_part_text_set(editfield_layout,
							 "elm.guidetext", "");
				elm_entry_entry_set(entry, "");
			}
		}
	}
}

void
openlock_setting_pw_create_view(openlock_setting_appdata *
				openlock_setting_data)
{
	Evas_Object *navi_bar = NULL;
	Evas_Object *win_main = NULL;
	Evas_Object *genlist = NULL;
	Evas_Object *ly = NULL;
	Evas_Object *cancel_button = NULL;
	Evas_Object *back_button = NULL;
	Elm_Object_Item *genlist_item = NULL;
	Elm_Object_Item *navi_it = NULL;
	openlock_ug_data *openlock_data = NULL;

	if (!openlock_setting_data) {
		return;
	}

	openlock_data = openlock_setting_data->openlock_data;
	if (!openlock_data) {
		return;
	}

	OPENLOCKS_DBG("openlock_setting_pw_create_view begin\n");

	win_main = openlock_data->win_main;
	navi_bar = openlock_data->navi_bar;

	ly = _openlock_setting_pw_create_conformant(win_main);
	openlock_setting_data->ly = ly;

	/* genlist */
	genlist = elm_genlist_add(navi_bar);
	_openlock_setting_pw_list_set_styles();
	genlist_item =
	    elm_genlist_item_append(genlist, &itc_label,
				    (void *)openlock_setting_data, NULL,
				    ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(genlist_item, EINA_TRUE);

	genlist_item =
	    elm_genlist_item_append(genlist, &itc_entry,
				    (void *)openlock_setting_data, NULL,
				    ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(genlist_item, EINA_TRUE);

	evas_object_show(genlist);
	elm_object_content_set(ly, genlist);
	openlock_setting_data->genlist = genlist;

	/* Done button */
	done_button = elm_button_add(navi_bar);
	elm_object_style_set(done_button, "naviframe/title/default");
	elm_object_text_set(done_button, "Done");	/* hard code for demo */
	elm_object_disabled_set(done_button, EINA_TRUE);
	evas_object_smart_callback_add(done_button, "clicked",
				       _openlock_setting_pw_done_cb,
				       openlock_setting_data);

	/* Cancel button */
	cancel_button = elm_button_add(navi_bar);
	elm_object_style_set(cancel_button, "naviframe/title/default");
	elm_object_text_set(cancel_button, "Cancel"); /* hard code for demo */
	elm_object_disabled_set(cancel_button, EINA_FALSE);
	evas_object_smart_callback_add(cancel_button, "clicked",
			_openlock_setting_pw_back_cb, openlock_setting_data);

	/* Set navigation objects and push */
	navi_it = elm_naviframe_item_push(navi_bar, "Enter Password", NULL, NULL,
			ly, NULL); /* hard code for demo */
	elm_object_item_part_content_set(navi_it, "title_right_btn", done_button);
	elm_object_item_part_content_set(navi_it, "title_left_btn", cancel_button);

	/* Remove <- button */
	back_button = elm_object_item_part_content_get(navi_it, "prev_btn");
	if (back_button != NULL) {
		elm_object_item_part_content_set(navi_it, "prev_btn", NULL);
		if (back_button != NULL) {
			evas_object_del(back_button);
		}
	}
}
