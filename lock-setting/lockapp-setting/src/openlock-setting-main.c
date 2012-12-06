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
#include <aul.h>
#include <ail.h>

#include "openlock-setting.h"
#include "openlock-setting-pw.h"
#include "openlock-setting-debug.h"
#include "openlock-setting-main.h"
#include "starter-vconf.h"

#define OPENLOCK_DESKTOP_VALUE_LOCKSCREEN "lock-screen"

static Elm_Gen_Item_Class itc;
static Evas_Object *radio_group = NULL;

static void _openlock_setting_main_back_cb(void *data, Evas_Object * obj,
					   void *event_info)
{
	openlock_ug_data *openlock_data = (openlock_ug_data *) data;

	if (openlock_data == NULL)
		return;

	ug_destroy_me(openlock_data->ug);
}

static void
_openlock_setting_main_button_clicked_cb(void *data, Evas_Object * obj,
					 void *event_info)
{
	if (data == NULL)
		return;

	openlock_setting_appdata *lockapp_data =
	    (openlock_setting_appdata *) data;

	OPENLOCKS_DBG("edit button pressed : %s", lockapp_data->pkg_name);

	bundle *b = NULL;

	b = bundle_create();

	bundle_add(b, "mode", "edit");
	aul_launch_app(lockapp_data->pkg_name, b);

	if (b)
		bundle_free(b);
}

static Evas_Object *_openlock_setting_main_gl_icon_get(void *data,
						       Evas_Object * obj,
						       const char *part)
{
	if (data == NULL || part == NULL)
		return NULL;

	openlock_setting_appdata *lockapp_data =
	    (openlock_setting_appdata *) data;
	Evas_Object *radio = NULL;
	Evas_Object *button = NULL;
	char *cur_pkg_name = NULL;
	char editable_key[512] = { 0, };
	int val = 0;

	OPENLOCKS_DBG("part : %s", part);

	if (!strcmp(part, "elm.icon.1")) {
		radio = elm_radio_add(obj);
		elm_radio_state_value_set(radio, lockapp_data->index);

		if (lockapp_data->index != 0) {
			elm_radio_group_add(radio, radio_group);
		} else {
			radio_group = radio;
		}

		lockapp_data->radio = radio;

		cur_pkg_name = vconf_get_str(VCONF_PRIVATE_LOCKSCREEN_PKGNAME);
		OPENLOCKS_DBG("cur pkg : %s, this pkg : %s", cur_pkg_name,
			      lockapp_data->pkg_name);
		if (cur_pkg_name == NULL) {
			return NULL;
		}
		if (!strcmp(cur_pkg_name, lockapp_data->pkg_name)) {
			elm_radio_value_set(radio_group, lockapp_data->index);
		}

		return radio;
	} else if (!strcmp(part, "elm.icon.2")) {
		snprintf(editable_key, sizeof(editable_key),
			 "db/openlockscreen/%s/editable",
			 lockapp_data->app_name);
		int ret = vconf_get_bool(editable_key, &val);
		if (ret == 0 && val == 1) {
			OPENLOCKS_DBG("app %s have a editable mode!",
				      lockapp_data->app_name);
			button = elm_button_add(obj);
			elm_object_style_set(button, "text_only/sweep");
			elm_object_text_set(button, "edit");
			evas_object_smart_callback_add(button, "clicked",
						       _openlock_setting_main_button_clicked_cb,
						       lockapp_data);

			return button;
		}
	}

	return NULL;
}

static char *_openlock_setting_main_gl_label_get(void *data, Evas_Object * obj,
						 const char *part)
{
	if (data == NULL || part == NULL)
		return NULL;

	openlock_setting_appdata *lockapp_data =
	    (openlock_setting_appdata *) data;

	if (lockapp_data->app_name && !strcmp(part, "elm.text")) {
		return strdup(lockapp_data->app_name);
	}

	return NULL;
}

static void _openlock_setting_main_gl_del(void *data, Evas_Object * obj)
{
	if (data == NULL)
		return;

	openlock_setting_appdata *lockapp_data =
	    (openlock_setting_appdata *) data;

	if (lockapp_data->pkg_name)
		free(lockapp_data->pkg_name);

	if (lockapp_data->app_name)
		free(lockapp_data->app_name);

	free(lockapp_data);
}

static void
_openlock_setting_main_gl_sel(void *data, Evas_Object * obj, void *event_info)
{
	char *cur_pkg_name = NULL;
	elm_genlist_item_selected_set((Elm_Object_Item *) event_info,
				      EINA_FALSE);

	if (data == NULL)
		return;

	openlock_setting_appdata *lockapp_data =
	    (openlock_setting_appdata *) data;

	(lockapp_data->count)++;
	OPENLOCKS_DBG("lockapp_data->count = %d\n", lockapp_data->count);

	if ((lockapp_data->count) > 1) {
		lockapp_data->count = 0;
		return;
	}

	OPENLOCKS_DBG("lockapp_data->pkg_name = %s\n", lockapp_data->pkg_name);

	if (lockapp_data != NULL && lockapp_data->pkg_name != NULL) {
		int ret = 0;
		elm_radio_value_set(radio_group, lockapp_data->index);
		ret = vconf_set_str(VCONF_PRIVATE_LOCKSCREEN_PKGNAME,
			      lockapp_data->pkg_name);
		if(ret != 0)
		{
			OPENLOCKS_ERR("Failed to get vconf : VCONF_PRIVATE_LOCKSCREEN_PKGNAME\n");
		}
		lockapp_data->count = 0;	/* reset the count */
	}
}


ail_cb_ret_e _openlock_setting_main_appinfo_cb(const ail_appinfo_h appinfo,
					       void *user_data)
{
	openlock_setting_appdata *lockapp_data =
	    (openlock_setting_appdata *) user_data;
	openlock_setting_appdata *new_lockapp_data = NULL;
	char *pkgname = NULL;
	char *appname = NULL;

	ail_appinfo_get_str(appinfo, AIL_PROP_PACKAGE_STR, &pkgname);
	ail_appinfo_get_str(appinfo, AIL_PROP_NAME_STR, &appname);

	/* Get tail of the list */
	while (lockapp_data->next != NULL) {
		lockapp_data = lockapp_data->next;
	}

	if (lockapp_data->index == 0 && lockapp_data->pkg_name == NULL) {
		/* first data */
		lockapp_data->pkg_name = strdup(pkgname);
		lockapp_data->app_name = strdup(appname);
		OPENLOCKS_DBG("appinfo %d) %s, %s", lockapp_data->index,
			      lockapp_data->pkg_name, lockapp_data->app_name);
	} else {
		/* create new data */
		new_lockapp_data = (openlock_setting_appdata *)
		    malloc(sizeof(openlock_setting_appdata));
		memset(new_lockapp_data, 0, sizeof(openlock_setting_appdata));
		new_lockapp_data->pkg_name = strdup(pkgname);
		new_lockapp_data->app_name = strdup(appname);
		new_lockapp_data->index = lockapp_data->index + 1;
		new_lockapp_data->openlock_data = lockapp_data->openlock_data;

		new_lockapp_data->next = NULL;
		new_lockapp_data->prev = lockapp_data;

		lockapp_data->next = new_lockapp_data;

		OPENLOCKS_DBG("appinfo %d) %s, %s", new_lockapp_data->index,
			      new_lockapp_data->pkg_name,
			      new_lockapp_data->app_name);
	}

	return AIL_CB_RET_CONTINUE;
}

void openlock_setting_main_create_view(openlock_ug_data * openlock_data)
{
	OPENLOCKS_DBG("openlock_setting_main_create_view begin\n");
	Evas_Object *navi_bar = openlock_data->navi_bar;
	Evas_Object *back_button = NULL;
	Evas_Object *genlist = NULL;
	Elm_Object_Item *item = NULL;
	int i = 0, count = 0;
	openlock_setting_appdata *lockapp_data = NULL;
	openlock_setting_appdata *head_lockapp_data = NULL;

	if (navi_bar == NULL) {
		OPENLOCKS_WARN("navi_bar is null.");
		return;
	}

	/* Get app info */
	ail_filter_h filter;
	ail_error_e ret;

	ret = ail_filter_new(&filter);
	if (ret != AIL_ERROR_OK) {
		OPENLOCKS_WARN("Fail ail_filter_new : %d", ret);
		return;
	}

	ret =
	    ail_filter_add_str(filter, AIL_PROP_CATEGORIES_STR,
			       OPENLOCK_DESKTOP_VALUE_LOCKSCREEN);
	ret = ail_filter_count_appinfo(filter, &count);

	lockapp_data = (openlock_setting_appdata *)
	    malloc(sizeof(openlock_setting_appdata));
	memset(lockapp_data, 0, sizeof(openlock_setting_appdata));
	lockapp_data->prev = NULL;
	lockapp_data->next = NULL;
	lockapp_data->pkg_name = NULL;
	lockapp_data->app_name = NULL;
	lockapp_data->index = 0;
	lockapp_data->openlock_data = openlock_data;
	ail_filter_list_appinfo_foreach(filter,
					_openlock_setting_main_appinfo_cb,
					lockapp_data);

	ail_filter_destroy(filter);

	OPENLOCKS_DBG("lock app count : %d", count);

	if (count > 0) {
		/* Create genlist */
		genlist = elm_genlist_add(navi_bar);

		itc.item_style = "dialogue/1text.2icon";
		itc.func.text_get = _openlock_setting_main_gl_label_get;
		itc.func.content_get = _openlock_setting_main_gl_icon_get;
		itc.func.state_get = NULL;
		itc.func.del = _openlock_setting_main_gl_del;

		head_lockapp_data = lockapp_data;

		for (i = 0; i < count; i++) {
			/* Find index */
			lockapp_data = head_lockapp_data;

			while (lockapp_data != NULL) {
				if (lockapp_data->index == i) {
					/* find */
					OPENLOCKS_DBG("%d) [%s][%s]",
						      lockapp_data->index,
						      lockapp_data->pkg_name,
						      lockapp_data->app_name);
					    elm_genlist_item_append(genlist,
								    &itc,
								    lockapp_data,
								    NULL,
								    ELM_GENLIST_ITEM_NONE,
								    _openlock_setting_main_gl_sel,
								    lockapp_data);
					break;
				}
				lockapp_data = lockapp_data->next;
			}
		}
	}

	/* Set navigation objects and push */
	back_button = elm_button_add(navi_bar);
	elm_object_style_set(back_button, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_button, "clicked",
				       _openlock_setting_main_back_cb,
				       openlock_data);

	elm_naviframe_item_push(navi_bar, "Downloaded lock screens", back_button, NULL, genlist, NULL);	/* the same tile */
}
