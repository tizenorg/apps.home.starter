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



#ifndef UG_MODULE_API
#define UG_MODULE_API __attribute__ ((visibility("default")))
#endif

#include <Elementary.h>
#include <ui-gadget-module.h>

#include "openlock-setting.h"
#include "openlock-setting-util.h"
#include "openlock-setting-main.h"

static Evas_Object *create_bg(Evas_Object * parent)
{
	Evas_Object *bg = elm_bg_add(parent);

	evas_object_size_hint_weight_set(bg, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	elm_object_style_set(bg, "group_list");

	elm_object_part_content_set(parent, "elm.swallow.bg", bg);
	evas_object_show(bg);

	return bg;
}

static Evas_Object *create_fullview(Evas_Object * parent,
				    openlock_ug_data * openlock_data)
{
	Evas_Object *base = NULL;
	Evas_Object *navi_bar = NULL;
	Evas_Object *bg = NULL;

	base = openlock_setting_util_create_layout(parent);

	elm_layout_theme_set(base, "layout", "application", "default");
	elm_win_resize_object_add(parent, base);
	elm_win_indicator_mode_set(parent, ELM_WIN_INDICATOR_SHOW);

	bg = create_bg(base);

	navi_bar = openlock_setting_util_create_navigation(base);
	openlock_data->navi_bar = navi_bar;

	openlock_setting_main_create_view(openlock_data);

	return base;
}

static Evas_Object *create_frameview(Evas_Object * parent,
				     openlock_ug_data * openlock_data)
{
	Evas_Object *base = NULL;

	/* Create Frame view */

	return base;
}

static void *on_create(ui_gadget_h ug, enum ug_mode mode, service_h service,
		       void *priv)
{
	Evas_Object *parent = NULL;
	Evas_Object *win_main = NULL;
	openlock_ug_data *openlock_data = NULL;

	if (!ug || !priv)
		return NULL;

	bindtextdomain("openlock-setting", "/opt/ug/res/locale");

	openlock_data = priv;
	openlock_data->ug = ug;

	parent = ug_get_parent_layout(ug);
	if (!parent)
		return NULL;

	win_main = ug_get_window();
	if (!win_main) {
		return NULL;
	}

	openlock_data->win_main = win_main;

	if (mode == UG_MODE_FULLVIEW)
		openlock_data->base = create_fullview(parent, openlock_data);
	else
		openlock_data->base = create_frameview(parent, openlock_data);

	/* Add del callback for base layout */

	return openlock_data->base;
}

static void on_start(ui_gadget_h ug, service_h service, void *priv)
{
}

static void on_pause(ui_gadget_h ug, service_h service, void *priv)
{

}

static void on_resume(ui_gadget_h ug, service_h service, void *priv)
{

}

static void on_destroy(ui_gadget_h ug, service_h service, void *priv)
{
	openlock_ug_data *openlock_data;

	if (!ug || !priv)
		return;

	openlock_data = priv;
	evas_object_del(openlock_data->base);
	openlock_data->base = NULL;
}

static void on_message(ui_gadget_h ug, service_h msg, service_h service,
		       void *priv)
{
}

static void on_event(ui_gadget_h ug, enum ug_event event, service_h service,
		     void *priv)
{
	switch (event) {
	case UG_EVENT_LOW_MEMORY:
		break;
	case UG_EVENT_LOW_BATTERY:
		break;
	case UG_EVENT_LANG_CHANGE:
		break;
	case UG_EVENT_ROTATE_PORTRAIT:
		break;
	case UG_EVENT_ROTATE_PORTRAIT_UPSIDEDOWN:
		break;
	case UG_EVENT_ROTATE_LANDSCAPE:
		break;
	case UG_EVENT_ROTATE_LANDSCAPE_UPSIDEDOWN:
		break;
	default:
		break;
	}
}

static void on_key_event(ui_gadget_h ug, enum ug_key_event event,
			 service_h service, void *priv)
{
	if (!ug)
		return;

	switch (event) {
	case UG_KEY_EVENT_END:
		ug_destroy_me(ug);
		break;
	default:
		break;
	}
}

UG_MODULE_API int UG_MODULE_INIT(struct ug_module_ops *ops)
{
	openlock_ug_data *openlock_data;

	if (!ops)
		return -1;

	openlock_data = calloc(1, sizeof(openlock_ug_data));
	if (!openlock_data)
		return -1;

	ops->create = on_create;
	ops->start = on_start;
	ops->pause = on_pause;
	ops->resume = on_resume;
	ops->destroy = on_destroy;
	ops->message = on_message;
	ops->event = on_event;
	ops->key_event = on_key_event;
	ops->priv = openlock_data;
	ops->opt = UG_OPT_INDICATOR_ENABLE;

	return 0;
}

UG_MODULE_API void UG_MODULE_EXIT(struct ug_module_ops *ops)
{
	openlock_ug_data *openlock_data;

	if (!ops)
		return;

	openlock_data = ops->priv;
	if (openlock_data)
		free(openlock_data);
}
