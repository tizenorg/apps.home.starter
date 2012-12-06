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



#include "openlock-setting-debug.h"
#include "openlock-setting-util.h"

Evas_Object *openlock_setting_util_create_navigation(Evas_Object * parent)
{
	Evas_Object *navi_bar = NULL;

	if (parent == NULL) {
		OPENLOCKS_WARN("Parent is null.");
		return NULL;
	}

	navi_bar = elm_naviframe_add(parent);
	if (navi_bar == NULL) {
		OPENLOCKS_ERR("Cannot add naviagtionbar.");
		return NULL;
	}

	elm_object_part_content_set(parent, "elm.swallow.content", navi_bar);

	evas_object_show(navi_bar);

	return navi_bar;
}

Evas_Object *openlock_setting_util_create_layout(Evas_Object * parent)
{
	Evas_Object *layout = NULL;

	if (parent == NULL) {
		OPENLOCKS_WARN("Parent is null.");
		return NULL;
	}

	layout = elm_layout_add(parent);
	if (layout == NULL) {
		OPENLOCKS_ERR("Cannot add layout.");
		return NULL;
	}

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);

	evas_object_show(layout);

	return layout;
}
