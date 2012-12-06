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



#ifndef __OPENLOCK_SETTING_MAIN_H__
#define __OPENLOCK_SETTING_MAIN_H__

#include <Elementary.h>
#include "openlock-setting.h"

typedef struct _openlock_setting_appdata openlock_setting_appdata;

struct _openlock_setting_appdata {
	openlock_setting_appdata *prev;
	openlock_setting_appdata *next;

	char *pkg_name;
	char *app_name;
	int index;

	Evas_Object *radio;
	Evas_Object *entry;
	Evas_Object *editfield_layout;
	Evas_Object *popup;
	/* for pw */
	Evas_Object *ly;
	Evas_Object *genlist;
	int count;
	openlock_ug_data *openlock_data;
};

void openlock_setting_main_create_view(openlock_ug_data * openlock_data);

#endif				/* __OPENLOCK_SETTING_MAIN_H__ */
