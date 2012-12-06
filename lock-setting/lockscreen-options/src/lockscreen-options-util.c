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



#include "lockscreen-options-debug.h"
#include "lockscreen-options.h"
#include "lockscreen-options-util.h"

const char *sys_str_table[] = {
	"IDS_COM_SK_OK",
	"IDS_COM_SK_CANCEL",
};

const char *app_str_table[] = {
	"IDS_ST_BODY_SHORTCUTS",
	"IDS_ST_BODY_SET_SHORTCUTS_ON_LOCK_SCREEN",
#if USE_TITLE_AND_CAMERA
	"Lock screen title",
	"Show title text on the lock screen",
#endif
	//"Event notifications",
	"IDS_IDLE_MBODY_EVENT_NOTIFICATIONS",
	//"View event notification on lock screen",
	"IDS_IDLE_BODY_VIEW_EVENT_NOTIFICATIONS_ON_THE_LOCK_SCREEN",
	"Context-aware notifications",
	"View context-aware notifications on lock screen",
#if USE_TITLE_AND_CAMERA
	"IDS_ST_MBODY_CAMERA_QUICK_ACCESS",
	"IDS_ST_BODY_TAP_AND_HOLD_THE_SCREEN_WHILE_ROTATING_THE_DEVICE_TO_OPEN_CAMERA",
#endif
	"IDS_ST_BODY_CLOCK",
//	"Dual clock",
//	"Show dual clock on lock screen when roaming",
	"IDS_ST_BODY_WEATHER",
	"IDS_IM_BODY_HELP_TEXT",
	"IDS_ST_BODY_SHOW_HELP_TEXT_ON_LOCK_SCREEN",
	"IDS_ST_BODY_SET_SHORTCUTS",
	"IDS_ST_OPT_EDIT_SHORTCUTS",
	"IDS_ST_BODY_SET_SHORTCUTS_ON_LOCK_SCREEN",
	"IDS_ST_BODY_TAP_SHORTCUT_TO_ASSIGN_APPLICATION",
	"IDS_ST_BODY_DRAG_AND_DROP_TO_CHANGE_ORDER",
	"IDS_ST_BODY_SELECT_APPLICATION",
	"Life Magazine"
};

static Elm_Gen_Item_Class itc_underline;
static Elm_Gen_Item_Class itc_separator;

Evas_Object *lockscreen_options_util_create_navigation(Evas_Object * parent)
{
	Evas_Object *navi_bar = NULL;

	if (parent == NULL) {
		LOCKOPTIONS_WARN("Parent is null.");
		return NULL;
	}

	navi_bar = elm_naviframe_add(parent);
	if (navi_bar == NULL) {
		LOCKOPTIONS_ERR("Cannot add naviagtionbar.");
		return NULL;
	}

	elm_object_part_content_set(parent, "elm.swallow.content", navi_bar);

	evas_object_show(navi_bar);

	return navi_bar;
}

Evas_Object *lockscreen_options_util_create_layout(Evas_Object * parent,
						   const char *file,
						   const char *group)
{
	Evas_Object *layout = NULL;

	if (parent == NULL) {
		LOCKOPTIONS_WARN("Parent is null.");
		return NULL;
	}

	layout = elm_layout_add(parent);
	if (layout == NULL) {
		LOCKOPTIONS_ERR("Cannot add layout.");
		return NULL;
	}

	if ((file != NULL) && (group != NULL)) {
		elm_layout_file_set(layout, file, group);
	}

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);

	evas_object_show(layout);

	return layout;
}

void lockscreen_options_util_create_seperator(Evas_Object * genlist)
{
	if (NULL == genlist)
		return;

	Elm_Object_Item *item = NULL;

	itc_separator.item_style = LOCKSCREEN_GENLIST_STYLE_SEPERATOR;
	itc_separator.func.text_get = NULL;
	itc_separator.func.content_get = NULL;
	itc_separator.func.state_get = NULL;
	itc_separator.func.del = NULL;

	item =
	    elm_genlist_item_append(genlist, &(itc_separator), NULL, NULL,
				    ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item,
					 ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
}

void lockscreen_options_util_create_underline(Evas_Object * genlist)
{
	if (NULL == genlist)
		return;

	Elm_Object_Item *item = NULL;

	itc_underline.item_style = LOCKSCREEN_GENLIST_STYLE_UNDERLINE;
	itc_underline.func.text_get = NULL;
	itc_underline.func.content_get = NULL;
	itc_underline.func.state_get = NULL;
	itc_underline.func.del = NULL;

	item =
	    elm_genlist_item_append(genlist, &(itc_underline), NULL, NULL,
				    ELM_GENLIST_ITEM_NONE, NULL, NULL);
	elm_genlist_item_select_mode_set(item,
					 ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
}

char *lockscreen_optoins_get_string(int id)
{
	LOCKOPTIONS_DBG("get string id : %d\n", id);

	char *str = NULL;

	if (id < IDS_LOCKSCREEN_OPTIONS_SYSTEM_STRING_MAX) {
		/* get system string */
		str = dgettext("sys_string", sys_str_table[id]);
	} else {
		/* get application string */
		str =
		    dgettext(PKGNAME,
			     app_str_table[id -
					   IDS_LOCKSCREEN_OPTIONS_SYSTEM_STRING_MAX]);
	}

	LOCKOPTIONS_DBG("get string : %s\n", str);

	return str;
}
