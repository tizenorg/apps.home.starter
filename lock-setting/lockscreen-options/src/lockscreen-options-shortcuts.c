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
	char *app_icon;
	char *app_name;
	char *pkg_name;
} app_list_item_s;

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
} options_shortcuts_view_s;

static Elm_Gengrid_Item_Class gic;
static Elm_Genlist_Item_Class ltc;
static options_shortcuts_view_s *options_shortcuts_view_data = NULL;
static Evas_Object *edit_item = NULL;

static Eina_Bool _lockscreen_options_package_name_compare(const char *pkg_name)
{
	char *temp_name = NULL;

	temp_name = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT1);
	if ((temp_name != NULL) && (strcmp(pkg_name, temp_name) == 0)) {
		LOCKOPTIONS_DBG("pkg_name is %s", pkg_name);
		free(temp_name);
		temp_name = NULL;
		return EINA_TRUE;
	}

	temp_name = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT2);
	if ((temp_name != NULL) && (strcmp(pkg_name, temp_name) == 0)) {
		LOCKOPTIONS_DBG("pkg_name is %s", pkg_name);
		free(temp_name);
		temp_name = NULL;
		return EINA_TRUE;
	}

	temp_name = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT3);
	if ((temp_name != NULL) && (strcmp(pkg_name, temp_name) == 0)) {
		LOCKOPTIONS_DBG("pkg_name is %s", pkg_name);
		free(temp_name);
		temp_name = NULL;
		return EINA_TRUE;
	}

	temp_name = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT4);
	if ((temp_name != NULL) && (strcmp(pkg_name, temp_name) == 0)) {
		LOCKOPTIONS_DBG("pkg_name is %s", pkg_name);
		free(temp_name);
		temp_name = NULL;
		return EINA_TRUE;
	}

	return EINA_FALSE;
}

static char *_lockscreen_options_icon_path_get(shortcuts_item_s *
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

static void _lockscreen_options_shortcuts_set(shortcuts_type_t shortcuts_type,
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

ail_cb_ret_e _lockscreen_options_appinfo_func(const ail_appinfo_h appinfo,
					      void *user_data)
{
	char *pkg_name = NULL;
	char *app_name = NULL;
	char *app_icon = NULL;
	int index = 0;
	Eina_Bool is_same = EINA_FALSE;

	if (AIL_ERROR_OK !=
	    ail_appinfo_get_str(appinfo, AIL_PROP_PACKAGE_STR, &pkg_name)) {
		LOCKOPTIONS_ERR("cannot get package name");
	}

	is_same = _lockscreen_options_package_name_compare(pkg_name);

	LOCKOPTIONS_DBG("is_same = %d", is_same);
	if (is_same == EINA_FALSE) {
		app_list_item_s *list_item =
		    (app_list_item_s *) calloc(1, sizeof(app_list_item_s));

		if (pkg_name) {
			list_item->pkg_name = strdup(pkg_name);
		}

		if (AIL_ERROR_OK !=
		    ail_appinfo_get_str(appinfo, AIL_PROP_NAME_STR,
					&app_name)) {
			LOCKOPTIONS_ERR("cannot get App name");
		}
		if (AIL_ERROR_OK !=
		    ail_appinfo_get_str(appinfo, AIL_PROP_ICON_STR,
					&app_icon)) {
			LOCKOPTIONS_ERR("cannot get App icon");
		}
		list_item->index = index;
		if (app_name) {
			list_item->app_name = strdup(app_name);
		}
		if (app_icon && (strcmp(app_icon, "(null)") != 0)) {
			list_item->app_icon = strdup(app_icon);
		} else {
			list_item->app_icon =
			    strdup(IMAGES_DIR "/mainmenu_icon.png");
		}
		options_shortcuts_view_data->app_list =
		    eina_list_append(options_shortcuts_view_data->app_list,
				     list_item);
		index++;
	}

	return AIL_CB_RET_CONTINUE;
}

void _lockscreen_options_list_all_packages()
{
	ail_filter_h filter;
	ail_error_e ret;

	ret = ail_filter_new(&filter);
	if (ret != AIL_ERROR_OK) {
		return;
	}

	ret = ail_filter_add_bool(filter, AIL_PROP_X_SLP_TASKMANAGE_BOOL, true);
	if (ret != AIL_ERROR_OK) {
		return;
	}

	ret = ail_filter_add_bool(filter, AIL_PROP_NODISPLAY_BOOL, false);
	if (ret != AIL_ERROR_OK) {
		return;
	}

	ret = ail_filter_add_str(filter, AIL_PROP_TYPE_STR, "Application");
	if (ret != AIL_ERROR_OK) {
		return;
	}

	ail_filter_list_appinfo_foreach(filter,
					_lockscreen_options_appinfo_func, NULL);
	ail_filter_destroy(filter);
}

static void _lockscreen_options_select_apps_delete_cb(void *data, Evas * e,
						      Evas_Object * obj,
						      void *event_info)
{
	void *list_item_data = NULL;

	EINA_LIST_FREE(options_shortcuts_view_data->app_list, list_item_data) {
		if (list_item_data) {
			free(list_item_data);
			list_item_data = NULL;
		}
	}
}

static void _lockscreen_options_shortcuts_delete_cb(void *data, Evas * e,
						    Evas_Object * obj,
						    void *event_info)
{
	void *list_item_data = NULL;

	if (options_shortcuts_view_data == NULL) {
		return;
	}

	EINA_LIST_FREE(options_shortcuts_view_data->app_list, list_item_data) {
		if (list_item_data) {
			free(list_item_data);
			list_item_data = NULL;
		}
	}
	if (options_shortcuts_view_data->gengrid) {
		evas_object_del(options_shortcuts_view_data->gengrid);
		options_shortcuts_view_data->gengrid = NULL;
	}
	if (options_shortcuts_view_data) {
		free(options_shortcuts_view_data);
		options_shortcuts_view_data = NULL;
	}
}

static void _lockscreen_options_shortcuts_back_cb(void *data, Evas_Object * obj,
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

//not used now
static void _lockscreen_options_shortcuts_thumbnail_delete_cb(void *data,
							      Evas_Object * obj,
							      void *event_info)
{
	LOCKOPTIONS_DBG("_lockscreen_options_shortcuts_thumbnail_delete_cb");
	Elm_Object_Item *object_item = NULL;
	shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) data;

	if (shortcuts_item == NULL) {
		return;
	}

	_lockscreen_options_shortcuts_set(shortcuts_item->index, "");

	object_item =
	    elm_gengrid_first_item_get(options_shortcuts_view_data->gengrid);
	while (object_item) {
		shortcuts_item_s *shortcuts_item_info = NULL;
		shortcuts_item_info = elm_object_item_data_get(object_item);
		if (shortcuts_item->index == shortcuts_item_info->index) {
			elm_gengrid_item_update(object_item);
		}
		object_item = elm_gengrid_item_next_get(object_item);
	}
}

//not used now
static void _lockscreen_options_grid_moved_cb(void *data, Evas_Object * obj,
					      void *event_info)
{
	int index = 0;
	Elm_Object_Item *object_item = NULL;
	shortcuts_item_s *shortcuts_item = NULL;

	options_shortcuts_view_data->is_moved = EINA_TRUE;

	object_item =
	    elm_gengrid_first_item_get(options_shortcuts_view_data->gengrid);
	while (object_item) {
		shortcuts_item = elm_object_item_data_get(object_item);
		if (shortcuts_item) {
			LOCKOPTIONS_DBG
			    ("index = %d, shortcuts_item->pkg_name = %s.",
			     index, shortcuts_item->pkg_name);
			shortcuts_item->index = index;
			if ((shortcuts_item->pkg_name == NULL)
			    || (strlen(shortcuts_item->pkg_name) == 0)
			    || (strcmp(shortcuts_item->pkg_name, "(null)") ==
				0)) {
				_lockscreen_options_shortcuts_set(index, "");
			} else {
				_lockscreen_options_shortcuts_set(index,
								  shortcuts_item->pkg_name);
			}
		}
		object_item = elm_gengrid_item_next_get(object_item);
		index++;
	}
}

static void _lockscreen_options_gengrid_select_cb(void *data, Evas_Object * obj,
						  const char *emission,
						  const char *source)
{
	shortcuts_item_s *shortcuts_item = (shortcuts_item_s *) data;

	if (shortcuts_item == NULL
	    || options_shortcuts_view_data->is_moved == EINA_TRUE) {
		options_shortcuts_view_data->is_moved = EINA_FALSE;
		return;
	}
	LOCKOPTIONS_DBG("_lockscreen_options_gengrid_select_cb.");
	if (shortcuts_item->item != NULL) {
		options_shortcuts_view_data->selected_gengrid_item =
		    shortcuts_item->item;
		elm_gengrid_item_selected_set(shortcuts_item->item, EINA_FALSE);
		_lockscreen_options_select_applications_create
		    (shortcuts_item->index);
	}
}

static Evas_Object *_lockscreen_options_grid_content_get(void *data,
							 Evas_Object * obj,
							 const char *part)
{
	char *icon_path = NULL;
	Evas_Object *icon = NULL;
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
		icon_path = _lockscreen_options_icon_path_get(shortcuts_item);
		icon = elm_icon_add(layout);
		if (icon_path) {
			elm_icon_file_set(icon, icon_path, NULL);
			elm_object_part_content_set(layout,
						    "elm.swallow.contents",
						    icon);
			elm_icon_aspect_fixed_set(icon, EINA_FALSE);
			/*button = elm_button_add(layout);
			elm_object_style_set(button, "minus/extended");
			evas_object_pass_events_set(button, 1);
			evas_object_repeat_events_set(button, 0);
			evas_object_propagate_events_set(button, 0);
			elm_object_part_content_set(layout,
						    "elm.swallow.button",
						    button);
			evas_object_smart_callback_add(button, "clicked",
						       _lockscreen_options_shortcuts_thumbnail_delete_cb,
						       shortcuts_item);*/
			free(icon_path);
			icon_path = NULL;
		} else {
			elm_icon_file_set(icon, IMAGES_DIR "/icon_add.png",
					  NULL);
			elm_object_part_content_set(layout,
						    "elm.swallow.contents",
						    icon);
		}
		edje_object_signal_callback_add(elm_layout_edje_get(layout),
						"mouse,clicked,*", "background",
						_lockscreen_options_gengrid_select_cb,
						shortcuts_item);
		return layout;
	}
	return NULL;
}

static void _lockscreen_options_grid_del(void *data, Evas_Object * obj)
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

static void _lockscreen_options_shortcuts_help_text_create(Evas_Object * parent)
{
	Evas_Object *title_label = NULL;
	Evas_Object *contents_label = NULL;
	char buffer[1024] = { 0 };

	title_label = elm_label_add(parent);
	snprintf(buffer, sizeof(buffer),
		 "<font_size=40><align=center>%s</align></font_size>",
		 lockscreen_optoins_get_string
		 (IDS_LOCKSCREEN_OPTIONS_SET_SHORTCUTS_ON_LOCKSCREEN));
	elm_object_text_set(title_label, buffer);
	evas_object_show(title_label);
	elm_object_part_content_set(parent, "shortcuts.help.title",
				    title_label);

	contents_label = elm_label_add(parent);
	snprintf(buffer, sizeof(buffer),
		 "<font_size=32><align=center><color=#7C7C7C>%s</color></align></font_size>",
		 lockscreen_optoins_get_string
		 (IDS_LOCKSCREEN_OPTIONS_TAP_SHORTCUTS));
	elm_object_text_set(contents_label, buffer);
	evas_object_show(contents_label);
	elm_object_part_content_set(parent, "shortcuts.help.contents",
				    contents_label);
}

static void _lockscreen_options_shortcuts_gengrid_create(Evas_Object * parent)
{
	Evas_Coord window_width = 0;
	Evas_Coord window_height = 0;
	Evas_Object *gengrid = NULL;
	int index = 0;

	gengrid = elm_gengrid_add(parent);
	ecore_x_window_size_get(ecore_x_window_root_first_get(), &window_width,
				&window_height);
	elm_gengrid_item_size_set(gengrid, window_width / 4,
				  GENGRID_ITEM_HEIGHT * window_height /
				  WINDOW_HEIGHT);
	elm_gengrid_align_set(gengrid, 0.5, 0.0);
	//elm_gengrid_reorder_mode_set(gengrid, EINA_TRUE),
	    evas_object_show(gengrid);
	elm_object_part_content_set(parent, "shortcuts.gengrid", gengrid);

	/*evas_object_smart_callback_add(gengrid, "moved",
				       _lockscreen_options_grid_moved_cb, NULL);*/
	options_shortcuts_view_data->gengrid = gengrid;

	gic.item_style = "default_grid";
	gic.func.text_get = NULL;
	gic.func.content_get = _lockscreen_options_grid_content_get;
	gic.func.state_get = NULL;
	gic.func.del = _lockscreen_options_grid_del;

	for (index = 0; index <= SET_SHORTCUTS_APP4; index++) {
		shortcuts_item_s *shortcuts_item =
		    (shortcuts_item_s *) calloc(1, sizeof(shortcuts_item_s));
		shortcuts_item->index = index;
		shortcuts_item->item =
		    elm_gengrid_item_append(gengrid, &gic,
					    (void *)shortcuts_item, NULL,
					    (void *)shortcuts_item);
	}
}

static char *_lockscreen_options_gl_text_get(void *data, Evas_Object * obj,
					     const char *part)
{
	int index = (int)data;
	app_list_item_s *list_item = (app_list_item_s *)
	    eina_list_nth(options_shortcuts_view_data->app_list, index);

	if (list_item == NULL || list_item->app_name == NULL) {
		return NULL;
	}

	if (!strcmp(part, "elm.text")) {
		return strdup(list_item->app_name);
	}
	return NULL;
}

static Evas_Object *_lockscreen_options_gl_content_get(void *data,
						       Evas_Object * obj,
						       const char *part)
{
	int index = (int)data;
	Evas_Object *icon = NULL;
	app_list_item_s *list_item = (app_list_item_s *)
	    eina_list_nth(options_shortcuts_view_data->app_list, index);

	if (list_item == NULL || list_item->app_icon == NULL) {
		return NULL;
	}

	if (!strcmp(part, "elm.icon") || !strcmp(part, "elm.swallow.icon")) {
		icon = elm_icon_add(obj);
		elm_icon_file_set(icon, list_item->app_icon, NULL);
		return icon;
	}

	return NULL;
}

static void _lockscreen_options_gl_options_sel(void *data, Evas_Object * obj,
					       void *event_info)
{
	Elm_Object_Item *item = (Elm_Object_Item *) event_info;
	int index = (int)data;
	app_list_item_s *list_item = (app_list_item_s *)
	    eina_list_nth(options_shortcuts_view_data->app_list, index);

	if (list_item == NULL || list_item->pkg_name == NULL) {
		return;
	}
	LOCKOPTIONS_DBG("list_item->pkg_name = %s", list_item->pkg_name);
	if (item != NULL) {
		elm_genlist_item_selected_set(item, EINA_FALSE);
		_lockscreen_options_shortcuts_set
		    (options_shortcuts_view_data->shortcuts_type,
		     list_item->pkg_name);
		elm_gengrid_item_update
		    (options_shortcuts_view_data->selected_gengrid_item);
	}
	elm_naviframe_item_pop(options_shortcuts_view_data->ug_data->navi_bar);
	Eina_Bool is_have = EINA_FALSE;
	is_have = lockscreen_options_shortcuts_check_items();
	if(is_have == EINA_TRUE){
		elm_object_disabled_set(edit_item, EINA_FALSE);
	}else{
		elm_object_disabled_set(edit_item, EINA_TRUE);
	}
}

void _lockscreen_options_select_applications_create(shortcuts_type_t
						    shortcuts_type)
{
	Evas_Object *genlist = NULL;
	int index = 0;

	_lockscreen_options_list_all_packages();

	options_shortcuts_view_data->shortcuts_type = shortcuts_type;
	genlist =
	    elm_genlist_add(options_shortcuts_view_data->ug_data->navi_bar);

	ltc.item_style = "1text.1icon.2";
	ltc.func.text_get = _lockscreen_options_gl_text_get;
	ltc.func.content_get = _lockscreen_options_gl_content_get;
	ltc.func.state_get = NULL;
	ltc.func.del = NULL;

	for (index = 0;
	     index < eina_list_count(options_shortcuts_view_data->app_list);
	     index++) {
		elm_genlist_item_append(genlist, &ltc, (void *)index, NULL,
					ELM_GENLIST_ITEM_NONE,
					_lockscreen_options_gl_options_sel,
					(void *)index);
	}

	evas_object_event_callback_add(genlist, EVAS_CALLBACK_DEL,
				       _lockscreen_options_select_apps_delete_cb,
				       NULL);
	elm_naviframe_item_push(options_shortcuts_view_data->ug_data->navi_bar, lockscreen_optoins_get_string(IDS_LOCKSCREEN_OPTIONS_SELECT_APPLICATIONS), NULL, NULL, genlist, NULL);	/* the same tile */
}

static void _lockscreen_options_shortcuts_edit_cb(void *data, Evas_Object * obj, void *event_info)
{
	lockscreen_options_shortcuts_create_edit_view(data);
}

void lockscreen_options_shortcuts_create_view(void *data)
{
	lockscreen_options_ug_data *ug_data =
	    (lockscreen_options_ug_data *) data;
	if (ug_data == NULL) {
		return;
	}
	Evas_Object *navi_bar = ug_data->navi_bar;
	Evas_Object *layout = NULL;

	if (navi_bar == NULL) {
		LOCKOPTIONS_ERR("navi_bar is null.");
		return;
	}

	options_shortcuts_view_data =
	    (options_shortcuts_view_s *) calloc(1,
						sizeof
						(options_shortcuts_view_s));
	options_shortcuts_view_data->ug_data = ug_data;

	layout =
	    lockscreen_options_util_create_layout(navi_bar,
						  EDJE_DIR
						  "/lockscreen-options.edj",
						  "lockscreen.options.shortcuts.main");
	_lockscreen_options_shortcuts_help_text_create(layout);
	_lockscreen_options_shortcuts_gengrid_create(layout);

	edit_item= elm_button_add(navi_bar);
	elm_object_style_set(edit_item, "naviframe/toolbar/default");
	elm_object_text_set(edit_item, _S("IDS_COM_BODY_EDIT"));
	evas_object_smart_callback_add(edit_item, "clicked", _lockscreen_options_shortcuts_edit_cb, ug_data);

	Evas_Object *back_button = elm_button_add(navi_bar);
	elm_object_style_set(back_button, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_button, "clicked", _lockscreen_options_shortcuts_back_cb, ug_data);
	evas_object_event_callback_add(layout, EVAS_CALLBACK_DEL,
					_lockscreen_options_shortcuts_delete_cb, NULL);


	Elm_Object_Item *navi_item = elm_naviframe_item_push(navi_bar, lockscreen_optoins_get_string(IDS_LOCKSCREEN_OPTIONS_SET_SHORTCUTS), back_button, NULL, layout, NULL);	/* the same tile */
	elm_object_item_part_content_set(navi_item , "toolbar_button1", edit_item);

	Eina_Bool is_have = EINA_FALSE;
	is_have = lockscreen_options_shortcuts_check_items();
	if(is_have == EINA_TRUE){
		elm_object_disabled_set(edit_item, EINA_FALSE);
	}else{
		elm_object_disabled_set(edit_item, EINA_TRUE);
	}
}

void lockscreen_options_shortcuts_update_view()
{
	Elm_Object_Item *object_item = NULL;

	object_item = elm_gengrid_first_item_get(options_shortcuts_view_data->gengrid);
	while (object_item) {
		elm_gengrid_item_update(object_item);
		object_item = elm_gengrid_item_next_get(object_item);
	}

	Eina_Bool is_have = EINA_FALSE;
	is_have = lockscreen_options_shortcuts_check_items();
	if(is_have == EINA_TRUE){
		elm_object_disabled_set(edit_item, EINA_FALSE);
	}else{
		elm_object_disabled_set(edit_item, EINA_TRUE);
	}
}

Eina_Bool lockscreen_options_shortcuts_check_items()
{
	char *pkg_name1 = NULL;
	char *pkg_name2 = NULL;
	char *pkg_name3 = NULL;
	char *pkg_name4 = NULL;
	Eina_Bool is_have = EINA_FALSE;

	pkg_name1 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT1);
	pkg_name2 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT2);
	pkg_name3 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT3);
	pkg_name4 = vconf_get_str(VCONFKEY_LOCKSCREEN_SHORTCUT4);

	if((pkg_name1 == NULL || strlen(pkg_name1) == 0) && (pkg_name2 == NULL || strlen(pkg_name2) == 0)
		&& (pkg_name3 == NULL || strlen(pkg_name3) == 0) && (pkg_name4 == NULL || strlen(pkg_name4) == 0))
	{
		is_have = EINA_FALSE;
		//elm_object_disabled_set(edit_item, EINA_TRUE);
	}
	else
	{
		is_have = EINA_TRUE;
		//elm_object_disabled_set(edit_item, EINA_FALSE);
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
	return is_have;
}
