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



#include <Elementary.h>
#include <Ecore_X.h>
#include <ail.h>
#include <vconf.h>
#include <vconf-keys.h>

#include "lockscreen-options.h"
#include "lockscreen-options-debug.h"
#include "lockscreen-options-util.h"
#include "lockscreen-options-shortcuts.h"
#include "lockscreen-options-shortcuts-edit.h"

#define EDJE_DIR                    "/usr/ug/res/edje/ug-lockscreen-options-efl"
#define IMAGES_DIR                  "/usr/ug/res/images/ug-lockscreen-options-efl"
#define GENGRID_ITEM_HEIGHT         188
#define WINDOW_HEIGHT               1280

typedef enum {
	SET_SHORTCUTS_APP1 = 0,
	SET_SHORTCUTS_APP2,
	SET_SHORTCUTS_APP3,
	SET_SHORTCUTS_APP4
} shortcuts_type_t;

typedef struct {
	int index;
	char *pkg_name;
	Elm_Object_Item *item;
} shortcuts_item_s;

typedef struct {
	lockscreen_options_ug_data *ug_data;
	Evas_Object *gengrid;
	Eina_List *app_list;
	shortcuts_type_t shortcuts_type;
	Elm_Object_Item *selected_gengrid_item;
	Eina_Bool is_moved;
} options_shortcuts_edit_view_s;

static Elm_Gengrid_Item_Class gic;
static options_shortcuts_edit_view_s *options_shortcuts_edit_view_data = NULL;
static bool delete_icons[4];

static Evas_Object *_lockscreen_options_grid_content_get_e(void *data, Evas_Object * obj, const char *part);

static char *_lockscreen_options_icon_path_get_e(shortcuts_item_s *
					       shortcuts_item)
{
	ail_appinfo_h handle;
	ail_error_e ret;
	char *pkg_name = NULL;
	char *temp = NULL;
	char *icon_path = NULL;
	shortcuts_type_t shortcuts_type = SET_SHORTCUTS_APP1;

	if (shortcuts_item == NULL) {
		return NULL;
	}

	shortcuts_type = shortcuts_item->index;

	switch (shortcuts_type) {
	case SET_SHORTCUTS_APP1:
		pkg_name = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT1);
		LOCKOPTIONS_ERR("pkg_name is %s", pkg_name);
		break;
	case SET_SHORTCUTS_APP2:
		pkg_name = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT2);
		LOCKOPTIONS_ERR("pkg_name is %s", pkg_name);
		break;
	case SET_SHORTCUTS_APP3:
		pkg_name = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT3);
		LOCKOPTIONS_ERR("pkg_name is %s", pkg_name);
		break;
	case SET_SHORTCUTS_APP4:
		pkg_name = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT4);
		LOCKOPTIONS_ERR("pkg_name is %s", pkg_name);
		break;
	default:
		break;
	}

	if (pkg_name == NULL || strlen(pkg_name) == 0) {
		LOCKOPTIONS_ERR("pkg_name is NULL");
		return NULL;
	}
	shortcuts_item->pkg_name = strdup(pkg_name);

	ret = ail_get_appinfo(pkg_name, &handle);
	if (ret != AIL_ERROR_OK) {
		LOCKOPTIONS_ERR("Fail to ail_get_appinfo");
		return NULL;
	}

	ret = ail_appinfo_get_str(handle, AIL_PROP_ICON_STR, &temp);
	if (ret != AIL_ERROR_OK) {
		LOCKOPTIONS_ERR("Fail to ail_appinfo_get_str");
		ail_destroy_appinfo(handle);
		return NULL;
	}

	if (temp) {
		icon_path = strdup(temp);
	}

	ret = ail_destroy_appinfo(handle);
	if (ret != AIL_ERROR_OK) {
		LOCKOPTIONS_ERR("Fail to ail_destroy_appinfo");
	}

	if (pkg_name) {
		free(pkg_name);
		pkg_name = NULL;
	}
	return icon_path;
}

static void _lockscreen_options_shortcuts_set_e(shortcuts_type_t shortcuts_type,
					      const char *pkg_name)
{
	int ret = 0;

	switch (shortcuts_type) {
	case SET_SHORTCUTS_APP1:
		ret = vconf_set_str(VCONFKEY_LOCKSCREEN_SHORTCUT1, pkg_name);
		if (ret != 0) {
			LOCKOPTIONS_ERR("set failed");
		}
		break;
	case SET_SHORTCUTS_APP2:
		ret = vconf_set_str(VCONFKEY_LOCKSCREEN_SHORTCUT2, pkg_name);
		if (ret != 0) {
			LOCKOPTIONS_ERR("set failed");
		}
		break;
	case SET_SHORTCUTS_APP3:
		ret = vconf_set_str(VCONFKEY_LOCKSCREEN_SHORTCUT3, pkg_name);
		if (ret != 0) {
			LOCKOPTIONS_ERR("set failed");
		}
		break;
	case SET_SHORTCUTS_APP4:
		ret = vconf_set_str(VCONFKEY_LOCKSCREEN_SHORTCUT4, pkg_name);
		if (ret != 0) {
			LOCKOPTIONS_ERR("set failed");
		}
		break;
	default:
		break;
	}
}

static void _lockscreen_options_shortcuts_delete_cb_e(void *data, Evas * e,
						    Evas_Object * obj,
						    void *event_info)
{
	void *list_item_data = NULL;

	if (options_shortcuts_edit_view_data == NULL) {
		return;
	}

	EINA_LIST_FREE(options_shortcuts_edit_view_data->app_list, list_item_data) {
		if (list_item_data) {
			free(list_item_data);
			list_item_data = NULL;
		}
	}
	if (options_shortcuts_edit_view_data->gengrid) {
		evas_object_del(options_shortcuts_edit_view_data->gengrid);
		options_shortcuts_edit_view_data->gengrid = NULL;
	}
	if (options_shortcuts_edit_view_data) {
		free(options_shortcuts_edit_view_data);
		options_shortcuts_edit_view_data = NULL;
	}
}

static void _lockscreen_options_shortcuts_back_cb_e(void *data, Evas_Object * obj,
						  void *event_info)
{
	lockscreen_options_ug_data *ug_data =
	    (lockscreen_options_ug_data *) data;
	if (ug_data == NULL) {
		return;
	}
	Evas_Object *navi_bar = ug_data->navi_bar;

	if (navi_bar == NULL) {
		LOCKOPTIONS_ERR("navi_bar is null.");
		return;
	}

	elm_naviframe_item_pop(navi_bar);
}

static void _lockscreen_options_grid_moved_cb_e(void *data, Evas_Object * obj,
					      void *event_info)
{
	options_shortcuts_edit_view_data->is_moved = EINA_TRUE;
}

static void _lockscreen_options_shortcuts_done_cb_e(void *data, Evas_Object * obj,
						  void *event_info)
{
	lockscreen_options_ug_data *ug_data =
	    (lockscreen_options_ug_data *) data;
	if (ug_data == NULL) {
		return;
	}

    int index = 0;
	Elm_Object_Item *object_item = NULL;
	shortcuts_item_s *shortcuts_item = NULL;

	object_item = elm_gengrid_first_item_get(options_shortcuts_edit_view_data->gengrid);
	while (object_item) {
		shortcuts_item = elm_object_item_data_get(object_item);
		if (shortcuts_item) {
			shortcuts_item->index = index;
			if ((shortcuts_item->pkg_name == NULL)
			    || (strlen(shortcuts_item->pkg_name) == 0)
			    || (strcmp(shortcuts_item->pkg_name, "(null)") ==
				0)) {
				_lockscreen_options_shortcuts_set_e(index, "");
			} else {
				_lockscreen_options_shortcuts_set_e(index,
								  shortcuts_item->pkg_name);
			}
		}
		object_item = elm_gengrid_item_next_get(object_item);
		index++;
	}
	while(index<4)
	{
		_lockscreen_options_shortcuts_set_e(index, "");
		index++;
	}
	Evas_Object *navi_bar = ug_data->navi_bar;

	if (navi_bar == NULL) {
		LOCKOPTIONS_ERR("navi_bar is null.");
		return;
	}
	elm_naviframe_item_pop(navi_bar);
	lockscreen_options_shortcuts_update_view();
}

static void _lockscreen_options_grid_del_e(void *data, Evas_Object * obj)
{
	shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) data;

	if (shortcuts_item == NULL) {
		return;
	}
	if (shortcuts_item->pkg_name) {
		free(shortcuts_item->pkg_name);
		shortcuts_item->pkg_name = NULL;
	}
	free(shortcuts_item);
	shortcuts_item = NULL;
}

static void _lockscreen_options_shortcuts_thumbnail_delete_cb_e(void *data,
							      Evas_Object * obj,
							      void *event_info)
{
	LOCKOPTIONS_DBG("_lockscreen_options_shortcuts_thumbnail_delete_cb");
	shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) data;

	if (shortcuts_item == NULL || options_shortcuts_edit_view_data->is_moved == EINA_TRUE) {
		options_shortcuts_edit_view_data->is_moved = EINA_FALSE;
		return;
	}

	delete_icons[shortcuts_item->index] = true;

	elm_gengrid_clear(options_shortcuts_edit_view_data->gengrid);

	Evas_Object *gengrid = options_shortcuts_edit_view_data->gengrid;

	evas_object_smart_callback_add(gengrid, "moved",
				       _lockscreen_options_grid_moved_cb_e, NULL);

	gic.item_style = "default_grid";
	gic.func.text_get = NULL;
	gic.func.content_get = _lockscreen_options_grid_content_get_e;
	gic.func.state_get = NULL;
	gic.func.del = _lockscreen_options_grid_del_e;

	char *pkg_name1 = NULL;
	char *pkg_name2 = NULL;
	char *pkg_name3 = NULL;
	char *pkg_name4 = NULL;
	pkg_name1 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT1);
	pkg_name2 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT2);
	pkg_name3 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT3);
	pkg_name4 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT4);

	if((delete_icons[0] == false) && (pkg_name1 != NULL && strlen(pkg_name1) != 0))
	{
		shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) calloc(1, sizeof(shortcuts_item_s));
		shortcuts_item->index = 0;
		shortcuts_item->item = elm_gengrid_item_append(gengrid, &gic,
			(void *)shortcuts_item, NULL, (void *)shortcuts_item);
    }
	if((delete_icons[1] == false) && (pkg_name2 != NULL && strlen(pkg_name2) != 0))
	{
		shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) calloc(1, sizeof(shortcuts_item_s));
		shortcuts_item->index = 1;
		shortcuts_item->item = elm_gengrid_item_append(gengrid, &gic,
			(void *)shortcuts_item, NULL, (void *)shortcuts_item);
	}
	if((delete_icons[2] == false) && (pkg_name3 != NULL && strlen(pkg_name3) != 0))
	{
		shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) calloc(1, sizeof(shortcuts_item_s));
		shortcuts_item->index = 2;
		shortcuts_item->item = elm_gengrid_item_append(gengrid, &gic,
			(void *)shortcuts_item, NULL, (void *)shortcuts_item);
	}
	if((delete_icons[3] == false) && (pkg_name4 != NULL && strlen(pkg_name4) != 0))
	{
		shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) calloc(1, sizeof(shortcuts_item_s));
		shortcuts_item->index = 3;
		shortcuts_item->item = elm_gengrid_item_append(gengrid, &gic,
			(void *)shortcuts_item, NULL, (void *)shortcuts_item);
	}

	if(pkg_name1 != NULL)
	{
		free(pkg_name1);
		pkg_name1 = NULL;
	}
	if(pkg_name2 != NULL)
	{
		free(pkg_name2);
		pkg_name2 = NULL;
	}
	if(pkg_name3 != NULL)
	{
		free(pkg_name3);
		pkg_name3 = NULL;
	}
	if(pkg_name4 != NULL)
	{
		free(pkg_name4);
		pkg_name4 = NULL;
	}
}

static Evas_Object *_lockscreen_options_grid_content_get_e(void *data,
							 Evas_Object * obj,
							 const char *part)
{
	char *icon_path = NULL;
	Evas_Object *icon = NULL;
	Evas_Object *button = NULL;
	Evas_Object *layout = NULL;
	shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) data;
	LOCKOPTIONS_DBG("_lockscreen_options_grid_content_get.");

	if (shortcuts_item == NULL) {
		return NULL;
	}

	if (!strcmp(part, "elm.swallow.icon")) {
		layout =
		    lockscreen_options_util_create_layout(obj,
							  EDJE_DIR
							  "/lockscreen-options.edj",
							  "lockscreen.options.shortcuts.thumbnail.main");
		icon_path = _lockscreen_options_icon_path_get_e(shortcuts_item);
		icon = elm_icon_add(layout);
		if (icon_path) {
			elm_icon_file_set(icon, icon_path, NULL);
			elm_object_part_content_set(layout,
						    "elm.swallow.contents",
						    icon);
			elm_icon_aspect_fixed_set(icon, EINA_FALSE);
			button = elm_button_add(layout);
			elm_object_style_set(button, "icon_minus");
			evas_object_pass_events_set(button, 1);
			evas_object_repeat_events_set(button, 0);
			evas_object_propagate_events_set(button, 0);
			elm_object_part_content_set(layout,
						    "elm.swallow.button",
						    button);
			evas_object_smart_callback_add(button, "clicked",
						       _lockscreen_options_shortcuts_thumbnail_delete_cb_e,
						       shortcuts_item);
			free(icon_path);
			icon_path = NULL;
		} else {
			return NULL;
		}
		return layout;
	}
	return NULL;
}

static void _lockscreen_options_shortcuts_help_text_create_e(Evas_Object * parent)
{
	Evas_Object *title_label = NULL;
	char buffer[1024] = { 0 };

	title_label = elm_label_add(parent);
	snprintf(buffer, sizeof(buffer),
		 "<font_size=40><align=center>%s</align></font_size>",
		 lockscreen_optoins_get_string
		 (IDS_LOCKSCREEN_OPTIONS_DRAG_DROP_SHORTCUTS));
	elm_object_text_set(title_label, buffer);
	evas_object_show(title_label);
	elm_object_part_content_set(parent, "shortcuts.edit.help.title",
				    title_label);
}

static void _lockscreen_options_shortcuts_gengrid_create_e(Evas_Object * parent)
{
	char *pkg_name1 = NULL;
	char *pkg_name2 = NULL;
	char *pkg_name3 = NULL;
	char *pkg_name4 = NULL;
	pkg_name1 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT1);
	pkg_name2 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT2);
	pkg_name3 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT3);
	pkg_name4 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT4);

	Evas_Coord window_width = 0;
	Evas_Coord window_height = 0;
	Evas_Object *gengrid = NULL;

	gengrid = elm_gengrid_add(parent);
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &window_width, &window_height);
	elm_gengrid_item_size_set(gengrid, window_width / 4,
					GENGRID_ITEM_HEIGHT * window_height /
					WINDOW_HEIGHT);
	elm_gengrid_align_set(gengrid, 0.5, 0.0);
	elm_gengrid_reorder_mode_set(gengrid, EINA_TRUE),
	evas_object_show(gengrid);
	elm_object_part_content_set(parent, "shortcuts.gengrid", gengrid);

	evas_object_smart_callback_add(gengrid, "moved",
				       _lockscreen_options_grid_moved_cb_e, NULL);
	options_shortcuts_edit_view_data->gengrid = gengrid;

	gic.item_style = "default_grid";
	gic.func.text_get = NULL;
	gic.func.content_get = _lockscreen_options_grid_content_get_e;
	gic.func.state_get = NULL;
	gic.func.del = _lockscreen_options_grid_del_e;

	if(pkg_name1 != NULL && strlen(pkg_name1) != 0)
	{
		shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) calloc(1, sizeof(shortcuts_item_s));
		shortcuts_item->index = 0;
		shortcuts_item->item = elm_gengrid_item_append(gengrid, &gic,
			(void *)shortcuts_item, NULL, (void *)shortcuts_item);
	}

	if(pkg_name2 != NULL && strlen(pkg_name2) != 0)
	{
		shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) calloc(1, sizeof(shortcuts_item_s));
		shortcuts_item->index = 1;
		shortcuts_item->item = elm_gengrid_item_append(gengrid, &gic,
			(void *)shortcuts_item, NULL, (void *)shortcuts_item);
	}

	if(pkg_name3 != NULL && strlen(pkg_name3) != 0)
	{
		shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) calloc(1, sizeof(shortcuts_item_s));
		shortcuts_item->index = 2;
		shortcuts_item->item = elm_gengrid_item_append(gengrid, &gic,
			(void *)shortcuts_item, NULL, (void *)shortcuts_item);
	}

	if(pkg_name4 != NULL && strlen(pkg_name4) != 0)
	{
		shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) calloc(1, sizeof(shortcuts_item_s));
		shortcuts_item->index = 3;
		shortcuts_item->item = elm_gengrid_item_append(gengrid, &gic,
			(void *)shortcuts_item, NULL, (void *)shortcuts_item);
	}

	if(pkg_name1 != NULL)
	{
		free(pkg_name1);
		pkg_name1 = NULL;
	}
	if(pkg_name2 != NULL)
	{
		free(pkg_name2);
		pkg_name2 = NULL;
	}
	if(pkg_name3 != NULL)
	{
		free(pkg_name3);
		pkg_name3 = NULL;
	}
	if(pkg_name4 != NULL)
	{
		free(pkg_name4);
		pkg_name4 = NULL;
	}
}

void lockscreen_options_shortcuts_create_edit_view(void *data)
{
	lockscreen_options_ug_data *ug_data =
	    (lockscreen_options_ug_data *) data;
	if (ug_data == NULL) {
        LOCKOPTIONS_ERR("ug is NULL");
		return;
	}
	Evas_Object *navi_bar = ug_data->navi_bar;
	Evas_Object *layout = NULL;

	if (navi_bar == NULL) {
		LOCKOPTIONS_ERR("navi_bar is null.");
		return;
	}

	options_shortcuts_edit_view_data =
	    (options_shortcuts_edit_view_s *) calloc(1,
						sizeof
						(options_shortcuts_edit_view_s));
	options_shortcuts_edit_view_data->ug_data = ug_data;

	layout =
	    lockscreen_options_util_create_layout(navi_bar,
						  EDJE_DIR
						  "/lockscreen-options.edj",
						  "lockscreen.options.shortcuts.main");
	_lockscreen_options_shortcuts_help_text_create_e(layout);
	_lockscreen_options_shortcuts_gengrid_create_e(layout);

	Evas_Object *done_button = elm_button_add(navi_bar);
	Evas_Object *cancel_button = elm_button_add(navi_bar);
	elm_object_style_set(done_button, "naviframe/toolbar/default");
	elm_object_text_set(done_button, _S("IDS_COM_SK_DONE"));
	evas_object_smart_callback_add(done_button, "clicked", _lockscreen_options_shortcuts_done_cb_e, ug_data);
	elm_object_style_set(cancel_button, "naviframe/toolbar/default");
	elm_object_text_set(cancel_button, _S("IDS_COM_POP_CANCEL"));
	evas_object_smart_callback_add(cancel_button, "clicked", _lockscreen_options_shortcuts_back_cb_e, ug_data);

	evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL,
				_lockscreen_options_shortcuts_delete_cb_e, NULL);

	Elm_Object_Item *navi_item = elm_naviframe_item_push(navi_bar, lockscreen_optoins_get_string(IDS_LOCKSCREEN_OPTIONS_EDIT_SHORTCUTS), NULL, NULL, layout, NULL);
	elm_object_item_part_content_set(navi_item, "prev_btn", NULL);
	elm_object_item_part_content_set(navi_item , "toolbar_button1", done_button);
	elm_object_item_part_content_set(navi_item , "toolbar_button2", cancel_button);

	int i;
	for(i = 0;i < 4;i++)
	{
		delete_icons[i] = false;
	}
}
