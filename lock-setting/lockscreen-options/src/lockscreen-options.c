 /*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.1 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://floralicense.org/license/
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

#include "lockscreen-options.h"
#include "lockscreen-options-util.h"
#include "lockscreen-options-main.h"

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
				    lockscreen_options_ug_data * ug_data)
{
	Evas_Object *base = NULL;
	Evas_Object *navi_bar = NULL;

	base = lockscreen_options_util_create_layout(parent, NULL, NULL);

	elm_layout_theme_set(base, "layout", "application", "default");
	elm_win_resize_object_add(parent, base);

	elm_win_indicator_mode_set(parent, ELM_WIN_INDICATOR_SHOW);

	create_bg(base);

	navi_bar = lockscreen_options_util_create_navigation(base);
	ug_data->navi_bar = navi_bar;

	lockscreen_options_main_create_view(ug_data);

	return base;
}

static Evas_Object *create_frameview(Evas_Object * parent,
				     lockscreen_options_ug_data * ug_data)
{
	Evas_Object *base = NULL;

	return base;
}

static void *on_create(ui_gadget_h ug, enum ug_mode mode, service_h service,
		       void *priv)
{
	Evas_Object *parent = NULL;
	Evas_Object *win_main = NULL;
	lockscreen_options_ug_data *ug_data = NULL;

	if (!ug || !priv)
		return NULL;

	bindtextdomain(PKGNAME, "/usr/ug/res/locale");

	ug_data = priv;
	ug_data->ug = ug;

	parent = ug_get_parent_layout(ug);
	if (!parent)
		return NULL;

	win_main = ug_get_window();
	if (!win_main) {
		return NULL;
	}

	ug_data->win_main = win_main;

	if (mode == UG_MODE_FULLVIEW)
		ug_data->base = create_fullview(parent, ug_data);
	else
		ug_data->base = create_frameview(parent, ug_data);

	return ug_data->base;
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
	lockscreen_options_ug_data *ug_data;

	if (!ug || !priv)
		return;

	ug_data = priv;
	evas_object_del(ug_data->base);
	ug_data->base = NULL;
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
	lockscreen_options_ug_data *ug_data;

	if (!ops)
		return -1;

	ug_data = calloc(1, sizeof(lockscreen_options_ug_data));
	if (!ug_data)
		return -1;

	ops->create = on_create;
	ops->start = on_start;
	ops->pause = on_pause;
	ops->resume = on_resume;
	ops->destroy = on_destroy;
	ops->message = on_message;
	ops->event = on_event;
	ops->key_event = on_key_event;
	ops->priv = ug_data;
	ops->opt = UG_OPT_INDICATOR_ENABLE;

	return 0;
}

UG_MODULE_API void UG_MODULE_EXIT(struct ug_module_ops *ops)
{
	lockscreen_options_ug_data *ug_data;

	if (!ops)
		return;

	ug_data = ops->priv;
	if (ug_data)
		free(ug_data);
}
