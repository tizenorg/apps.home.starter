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



#include <vconf.h>
#include <vconf-keys.h>
#include <ui-gadget.h>
#include <ui-gadget-module.h>
#include <ail.h>

#include "lockscreen-options.h"
#include "lockscreen-options-debug.h"
#include "lockscreen-options-main.h"
#include "lockscreen-options-util.h"

#include "starter-vconf.h"

#define LOCKSCREEN_OPTOINS_GENLIST_ITEM_CNT 5

typedef struct {
	int glStyle;
	int stringId;
	Evas_Object *check;
	void (*func) (void *data, Evas_Object * obj, void *event_info);
} lockscreen_menu_item_info;

static lockscreen_menu_item_info lockscreen_options_menu_item[] = {
	{ENUM_LOCKSCREEN_GENLIST_STYLE_1TEXT1ICON, IDS_LOCKSCREEN_OPTIONS_EVENT_NOTIFICATIONS, NULL, NULL},
	{ENUM_LOCKSCREEN_GENLIST_STYLE_HELP, IDS_LOCKSCREEN_OPTIONS_EVENT_NOTIFICATIONS_HELP, NULL, NULL},
	{ENUM_LOCKSCREEN_GENLIST_STYLE_1TEXT1ICON, IDS_LOCKSCREEN_OPTIONS_CLOCK, NULL, NULL},
	{ENUM_LOCKSCREEN_GENLIST_STYLE_1TEXT1ICON, IDS_LOCKSCREEN_OPTIONS_HELPTEXT, NULL, NULL},
	{ENUM_LOCKSCREEN_GENLIST_STYLE_HELP, IDS_LOCKSCREEN_OPTIONS_HELPTEXT_HELP, NULL, NULL}
};

static Elm_Gen_Item_Class itc_menu_1text1icon;
static Elm_Gen_Item_Class itc_menu_2text1icon;
static Elm_Gen_Item_Class itc_help_1text;

static void _lockscreen_options_main_back_cb(void *data, Evas_Object * obj,
					     void *event_info)
{
	lockscreen_options_ug_data *ug_data =
	    (lockscreen_options_ug_data *) data;

	if (ug_data == NULL)
		return;

	ug_destroy_me(ug_data->ug);
}

static char *_lockscreen_options_main_gl_label_get(void *data,
						   Evas_Object * obj,
						   const char *part)
{
	if (data == NULL || part == NULL)
		return NULL;

	lockscreen_menu_item_info *lockoption_data =
	    (lockscreen_menu_item_info *) data;

	if ((strcmp(part, "elm.text") == 0) ||
	    (strcmp(part, "elm.text.1") == 0)) {
		return
		    strdup(lockscreen_optoins_get_string
			   (lockoption_data->stringId));
	}

	return NULL;

}

static void _lockscreen_options_set_menu_status(int stringId, int value)
{
	int ret = 0;
	switch (stringId) {
	case IDS_LOCKSCREEN_OPTIONS_EVENT_NOTIFICATIONS:
		ret = vconf_set_bool(VCONFKEY_LOCKSCREEN_EVENT_NOTIFICATION_DISPLAY, value);
		break;
	case IDS_LOCKSCREEN_OPTIONS_CLOCK:
		ret = vconf_set_bool(VCONFKEY_LOCKSCREEN_CLOCK_DISPLAY, value);
		break;
	case IDS_LOCKSCREEN_OPTIONS_HELPTEXT:
		ret = vconf_set_bool(VCONFKEY_LOCKSCREEN_HELP_TEXT_DISPLAY, value);
		break;
	default:
		LOCKOPTIONS_DBG("NO VALID STRINGID %d", stringId);
		break;
	}
}

static bool _lockscreen_options_get_menu_status(int stringId)
{
	int status = 0;
	int ret = 0;

	switch (stringId) {
	case IDS_LOCKSCREEN_OPTIONS_EVENT_NOTIFICATIONS:
		ret = vconf_get_bool(VCONFKEY_LOCKSCREEN_EVENT_NOTIFICATION_DISPLAY, &status);
		break;
	case IDS_LOCKSCREEN_OPTIONS_CLOCK:
		ret = vconf_get_bool(VCONFKEY_LOCKSCREEN_CLOCK_DISPLAY, &status);
		break;
	case IDS_LOCKSCREEN_OPTIONS_HELPTEXT:
		ret = vconf_get_bool(VCONFKEY_LOCKSCREEN_HELP_TEXT_DISPLAY, &status);
		break;
	default:
		LOCKOPTIONS_DBG("NO VALID INDEX %d", stringId);
		break;
	}

	if (ret == -1) {
		LOCKOPTIONS_ERR("Failed to get vconfkey %d!", stringId);
		return 0;
	}

	LOCKOPTIONS_DBG
	    ("_lockscreen_options_get_menu_status index %d Status %d", stringId,
	     status);

	return status;
}

static void _lockscreen_options_check_changed_cb(void *data, Evas_Object * obj,
						 void *event_info)
{
	if (data == NULL || obj == NULL)
		return;

	lockscreen_menu_item_info *lockoption_data =
	    (lockscreen_menu_item_info *) data;

	Eina_Bool ret;
	int value = 0;

	ret = elm_check_state_get(obj);

	LOCKOPTIONS_DBG("_lockscreen_options_check_changed_cb : %s",
			ret == EINA_TRUE ? "ON" : "OFF");

	if (ret == EINA_TRUE) {
		value = 1;
	} else {
		value = 0;
	}

	_lockscreen_options_set_menu_status(lockoption_data->stringId, value);
}

static Evas_Object *_lockscreen_options_main_gl_icon_get(void *data,
							 Evas_Object * obj,
							 const char *part)
{
	if (data == NULL || obj == NULL)
		return NULL;

	lockscreen_menu_item_info *lockoption_data =
	    (lockscreen_menu_item_info *) data;

	LOCKOPTIONS_DBG("icon get stringId : %d", lockoption_data->stringId);

	Evas_Object *check;
	int value = 0;

	check = elm_check_add(obj);
	elm_object_style_set(check, "on&off");
	evas_object_show(check);

	value = _lockscreen_options_get_menu_status(lockoption_data->stringId);
	elm_check_state_set(check, value);

	evas_object_pass_events_set(check, 1);
	evas_object_propagate_events_set(check, 0);

	evas_object_smart_callback_add(check, "changed",
				       _lockscreen_options_check_changed_cb,
				       lockoption_data);

	lockoption_data->check = check;

	return check;
}

static void _lockscreen_options_main_gl_del(void *data, Evas_Object * obj)
{
	LOCKOPTIONS_DBG("_lockscreen_options_main_gl_del");
}

static void _lockscreen_options_main_gl_sel(void *data, Evas_Object * obj,
					    void *event_info)
{
	if (data == NULL)
		return;

	lockscreen_menu_item_info *lockoption_data = NULL;

	elm_genlist_item_selected_set((Elm_Object_Item *) event_info,
				      EINA_FALSE);

	Elm_Object_Item *item = (Elm_Object_Item *) event_info;
	lockoption_data =
	    (lockscreen_menu_item_info *) elm_object_item_data_get(item);
	if (lockoption_data == NULL) {
		return;
	}

	if (lockoption_data->stringId ==
	    IDS_LOCKSCREEN_OPTIONS_EVENT_NOTIFICATIONS
	    || lockoption_data->stringId == IDS_LOCKSCREEN_OPTIONS_CLOCK
	    || lockoption_data->stringId == IDS_LOCKSCREEN_OPTIONS_HELPTEXT) {
		Eina_Bool check_state =
		    elm_check_state_get(lockoption_data->check);
		_lockscreen_options_set_menu_status(lockoption_data->stringId,
						    !check_state);
		elm_genlist_item_update(item);
	}

	if (lockoption_data->func != NULL) {
		lockoption_data->func(data, obj, event_info);
	}
}

static void _lockscreen_options_create_gl_item(Elm_Gen_Item_Class * item,
					       int glStyle)
{
	if (glStyle == ENUM_LOCKSCREEN_GENLIST_STYLE_1TEXT1ICON) {
		item->item_style = LOCKSCREEN_GENLIST_STYLE_1TEXT1ICON;
		item->func.text_get = _lockscreen_options_main_gl_label_get;
		item->func.content_get = _lockscreen_options_main_gl_icon_get;
		item->func.state_get = NULL;
		item->func.del = _lockscreen_options_main_gl_del;
	} else if (glStyle == ENUM_LOCKSCREEN_GENLIST_STYLE_2TEXT1ICON) {
		item->item_style = LOCKSCREEN_GENLIST_STYLE_2TEXT1ICON;
		item->func.text_get = _lockscreen_options_main_gl_label_get;
		item->func.content_get = _lockscreen_options_main_gl_icon_get;
		item->func.state_get = NULL;
		item->func.del = NULL;
	} else if (glStyle == ENUM_LOCKSCREEN_GENLIST_STYLE_HELP) {
		item->item_style = LOCKSCREEN_GENLIST_STYLE_HELP;
		item->func.text_get = _lockscreen_options_main_gl_label_get;
		item->func.content_get = NULL;
		item->func.state_get = NULL;
		item->func.del = _lockscreen_options_main_gl_del;
	} else {
		LOCKOPTIONS_DBG("_lockscreen_options_create_gl_item FAIL\n");
	}
}

void lockscreen_options_main_create_view(lockscreen_options_ug_data * ug_data)
{
	LOCKOPTIONS_DBG("lockscreen_options_main_create_view begin\n");

	Evas_Object *navi_bar = ug_data->navi_bar;
	Evas_Object *back_button = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *item = NULL;
	int i = 0;

	if (navi_bar == NULL) {
		LOCKOPTIONS_WARN("navi_bar is null.");
		return;
	}

	_lockscreen_options_create_gl_item(&(itc_menu_1text1icon),
					   ENUM_LOCKSCREEN_GENLIST_STYLE_1TEXT1ICON);
	_lockscreen_options_create_gl_item(&(itc_menu_2text1icon),
					   ENUM_LOCKSCREEN_GENLIST_STYLE_2TEXT1ICON);
	_lockscreen_options_create_gl_item(&(itc_help_1text),
					   ENUM_LOCKSCREEN_GENLIST_STYLE_HELP);

	genlist = elm_genlist_add(navi_bar);

	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_object_style_set(genlist, "dialogue");

	lockscreen_options_util_create_seperator(genlist);

	for (i = 0; i < LOCKSCREEN_OPTOINS_GENLIST_ITEM_CNT; i++) {
		Elm_Gen_Item_Class *itc = NULL;
		if (lockscreen_options_menu_item[i].glStyle ==
		    ENUM_LOCKSCREEN_GENLIST_STYLE_1TEXT1ICON) {
			itc = &(itc_menu_1text1icon);
			elm_genlist_item_append(genlist,
						       itc,
						       &
						       (lockscreen_options_menu_item
							[i]), NULL,
						       ELM_GENLIST_ITEM_NONE,
						       _lockscreen_options_main_gl_sel,
						       ug_data);
		} else if(lockscreen_options_menu_item[i].glStyle ==
		    ENUM_LOCKSCREEN_GENLIST_STYLE_2TEXT1ICON) {
		    itc = &(itc_menu_2text1icon);
			elm_genlist_item_append(genlist,
						       itc,
						       &
						       (lockscreen_options_menu_item
							[i]), NULL,
						       ELM_GENLIST_ITEM_NONE,
						       _lockscreen_options_main_gl_sel,
						       ug_data);
		} else if (lockscreen_options_menu_item[i].glStyle ==
			   ENUM_LOCKSCREEN_GENLIST_STYLE_HELP) {
			itc = &(itc_help_1text);
			item = elm_genlist_item_append(genlist,
						       itc,
						       &
						       (lockscreen_options_menu_item
							[i]), NULL,
						       ELM_GENLIST_ITEM_NONE,
						       NULL, NULL);
			elm_genlist_item_select_mode_set(item,
							 ELM_OBJECT_SELECT_MODE_DISPLAY_ONLY);
			if(i != (LOCKSCREEN_OPTOINS_GENLIST_ITEM_CNT - 1)){
				lockscreen_options_util_create_underline(genlist);
			}
		} else {
			LOCKOPTIONS_WARN("lockscreen option has no such type.");
			return;
		}
	}

	back_button = elm_button_add(navi_bar);
	elm_object_style_set(back_button, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_button, "clicked",
				       _lockscreen_options_main_back_cb,
				       ug_data);

	elm_naviframe_item_push(navi_bar, _("IDS_ST_BODY_LOCK_SCREEN") , back_button, NULL, genlist, NULL);
}
