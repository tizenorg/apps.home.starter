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
#include "lockscreen-options-dualclock.h"

#define CITY_BUF_SIZE 		128
#define GMT_BUF_SIZE        10

static char time_zone_str[CITY_BUF_SIZE + GMT_BUF_SIZE + 3];
static Elm_Object_Item *genlist_item;


static Elm_Gen_Item_Class itc_menu_2text;

static char *_lockscreen_options_dualclock_gl_label_get(void *data,
						   Evas_Object * obj,
						   const char *part)
{
	if (part == NULL)
		return NULL;

	if ((strcmp(part, "elm.text.1") == 0)){
		return strdup("Set home city");
	}
	if((strcmp(part, "elm.text.2") == 0)) {
		return strdup(time_zone_str);
	}

	return NULL;
}


static void _lockscreen_options_dualclock_create_gl_item(Elm_Gen_Item_Class * item)
{
	item->item_style = "dialogue/2text.3";
	item->func.text_get = _lockscreen_options_dualclock_gl_label_get;
	item->func.content_get = NULL;
	item->func.state_get = NULL;
	item->func.del = NULL;
}

static void _lockscreen_options_update_timezone()
{
	elm_genlist_item_update(genlist_item);
}

static void _launch_worldclock_layout_ug_cb(ui_gadget_h ug,
						      enum ug_mode mode,
						      void *priv)
{
	LOCKOPTIONS_DBG("_launch_worldclock_layout_ug_cb begin.\n");
	lockscreen_options_ug_data *ug_data = (lockscreen_options_ug_data *) priv;
	Evas_Object *base;

	if (!priv)
		return;

	base = (Evas_Object *) ug_get_layout(ug);
	if (!base)
		return;

	switch (mode) {
	case UG_MODE_FULLVIEW:
		evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND,
						 EVAS_HINT_EXPAND);
		elm_win_resize_object_add(ug_data->win_main, base);
		evas_object_show(base);
		break;
	default:
		break;
	}
	LOCKOPTIONS_DBG("_launch_worldclock_layout_ug_cb end.\n");
}

static void _launch_worldclock_result_ug_cb(ui_gadget_h ug,
						 service_h result, void *priv)
{
	LOCKOPTIONS_DBG("_launch_worldclock_result_ug_cb begin.\n");
	if (!priv)
		return;

	char *city = NULL;
	char *timezone = NULL;
	service_get_extra_data(result, "city", &city);
	if(city == NULL) return;
	service_get_extra_data(result, "timezone", &timezone);
	if(timezone == NULL) return;

	snprintf(time_zone_str, CITY_BUF_SIZE + GMT_BUF_SIZE + 2, "%s, GMT %s", _S(city), _(timezone));
	LOCKOPTIONS_DBG("time_zone_str is [%s]", time_zone_str);
	_lockscreen_options_update_timezone();

	free(city);
	free(timezone);
	city = timezone = NULL;
	LOCKOPTIONS_DBG("_launch_worldclock_result_ug_cb end.\n");
}

static void _launch_worldclock_destroy_ug_cb(ui_gadget_h ug,
						       void *priv)
{
	LOCKOPTIONS_DBG("_launch_worldclock_destroy_ug_cb begin.\n");

	if (!priv)
		return;

	if (ug) {
		ug_destroy(ug);
	}

	LOCKOPTIONS_DBG("_launch_worldclock_destroy_ug_cb end.\n");
}

static void _launch_worldclock(void *data)
{
	LOCKOPTIONS_DBG("Launch world clock begin.\n");

	if(data == NULL){
		LOCKOPTIONS_ERR("The data (ug_data) transferred to _lauch_worldclock is NULL.");
		return;
	}

	lockscreen_options_ug_data *ug_data = (lockscreen_options_ug_data *) data;

	struct ug_cbs *cbs = (struct ug_cbs *)calloc(1, sizeof(struct ug_cbs));
	if(cbs == NULL)
		return;
	cbs->layout_cb = _launch_worldclock_layout_ug_cb;
	cbs->result_cb = _launch_worldclock_result_ug_cb;
	cbs->destroy_cb = _launch_worldclock_destroy_ug_cb;
	cbs->priv = (void *)ug_data;

	LOCKOPTIONS_DBG("Launch worldclock-efl begin.\n");
	ui_gadget_h loading=ug_create(ug_data->ug, "worldclock-efl", UG_MODE_FULLVIEW, NULL, cbs);
	if (NULL == loading) {
		LOCKOPTIONS_ERR("Launch wordclock-efl failed.");
    }

	free(cbs);
	LOCKOPTIONS_DBG("Launch world clock end.\n");
}

static void _lockscreen_options_dualclock_select_cb(void *data, Evas_Object *obj,
					   void *event_info)
{
	elm_genlist_item_selected_set(genlist_item, EINA_FALSE);
	if(data == NULL){
		LOCKOPTIONS_ERR("ug_data is null.");
		return;
	}

	_launch_worldclock(data);
}

static void _lockscreen_options_dualclock_back_cb(void *data, Evas_Object * obj,
					     void *event_info)
{
	lockscreen_options_ug_data *ug_data =
		(lockscreen_options_ug_data *) data;

	if (ug_data == NULL){
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

static void _init_time_zone()
{
	snprintf(time_zone_str, CITY_BUF_SIZE + GMT_BUF_SIZE + 2, "%s, GMT %s", _("Nanjing"), _("+8"));
}


void lockscreen_options_dualclock_create_view(lockscreen_options_ug_data * ug_data)
{
	LOCKOPTIONS_DBG("lockscreen_options_dualclock_create_view begin\n");

	Evas_Object *navi_bar = ug_data->navi_bar;
	Evas_Object *back_button = NULL;
	Evas_Object *genlist = NULL;

	if (navi_bar == NULL) {
		LOCKOPTIONS_ERR("navi_bar is null.");
		return;
	}

	_init_time_zone();

	genlist = elm_genlist_add(navi_bar);
	elm_genlist_mode_set(genlist, ELM_LIST_COMPRESS);
	elm_object_style_set(genlist, "dialogue");

	_lockscreen_options_dualclock_create_gl_item(&itc_menu_2text);

	lockscreen_options_util_create_seperator(genlist);


	genlist_item = elm_genlist_item_append(genlist,
						       &itc_menu_2text,
						       NULL, NULL,
						       ELM_GENLIST_ITEM_NONE,
						       _lockscreen_options_dualclock_select_cb,
						       ug_data);


	back_button = elm_button_add(navi_bar);
	elm_object_style_set(back_button, "naviframe/back_btn/default");
	evas_object_smart_callback_add(back_button, "clicked",
				       _lockscreen_options_dualclock_back_cb,
				       ug_data);

	elm_naviframe_item_push(navi_bar, "Dual clock", back_button, NULL, genlist, NULL);
}



