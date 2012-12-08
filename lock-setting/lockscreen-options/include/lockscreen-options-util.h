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



#ifndef __LOCKSCREEN_OPTIONS_UTIL_H__
#define __LOCKSCREEN_OPTIONS_UTIL_H__

#include <Elementary.h>

enum {
	IDS_COM_SK_OK = 0,
	IDS_COM_SK_CANCEL,
	IDS_LOCKSCREEN_OPTIONS_SYSTEM_STRING_MAX,
};

enum {
	IDS_LOCKSCREEN_OPTIONS_EVENT_NOTIFICATIONS =
		IDS_LOCKSCREEN_OPTIONS_SYSTEM_STRING_MAX,
	IDS_LOCKSCREEN_OPTIONS_EVENT_NOTIFICATIONS_HELP,
	IDS_LOCKSCREEN_OPTIONS_CLOCK,
	IDS_LOCKSCREEN_OPTIONS_HELPTEXT,
	IDS_LOCKSCREEN_OPTIONS_HELPTEXT_HELP,
	IDS_LOCKSCREEN_OPTIONS_APP_STRING_MAX,
};

Evas_Object *lockscreen_options_util_create_navigation(Evas_Object * parent);
Evas_Object *lockscreen_options_util_create_layout(Evas_Object * parent,
						   const char *file,
						   const char *group);
void lockscreen_options_util_create_seperator(Evas_Object * genlist);
void lockscreen_options_util_create_underline(Evas_Object * genlist);
char *lockscreen_optoins_get_string(int id);

#endif
