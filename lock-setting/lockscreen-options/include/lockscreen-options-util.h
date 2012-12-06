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

#define USE_TITLE_AND_CAMERA 0

enum {
	IDS_COM_SK_OK = 0,
	IDS_COM_SK_CANCEL,
	IDS_LOCKSCREEN_OPTIONS_SYSTEM_STRING_MAX,
};

enum {
	IDS_LOCKSCREEN_OPTIONS_SHORTCUTS =
	    IDS_LOCKSCREEN_OPTIONS_SYSTEM_STRING_MAX,
	IDS_LOCKSCREEN_OPTIONS_SHORTCUTS_HELP,
#if USE_TITLE_AND_CAMERA
	IDS_LOCKSCREEN_OPTIONS_LOCK_SCREEN_TITLE,
	IDS_LOCKSCREEN_OPTIONS_LOCK_SCREEN_TITLE_HELP,
#endif
	IDS_LOCKSCREEN_OPTIONS_EVENT_NOTIFICATIONS,
	IDS_LOCKSCREEN_OPTIONS_EVENT_NOTIFICATIONS_HELP,
	IDS_LOCKSCREEN_OPTIONS_CONTEXTAWARE_NOTI,
	IDS_LOCKSCREEN_OPTIONS_CONTEXTAWARE_NOTI_HELP,
#if USE_TITLE_AND_CAMERA
	IDS_LOCKSCREEN_OPTIONS_CAMERA_QUICK_ACCESS,
	IDS_LOCKSCREEN_OPTIONS_CAMERA_QUICK_ACCESS_HELP,
#endif
	IDS_LOCKSCREEN_OPTIONS_CLOCK,
//	IDS_LOCKSCREEN_OPTIONS_DUAL_CLOCK,
//	IDS_LOCKSCREEN_OPTIONS_DUAL_CLOCK_HELP,
	IDS_LOCKSCREEN_OPTIONS_WEATHER,
	IDS_LOCKSCREEN_OPTIONS_HELPTEXT,
	IDS_LOCKSCREEN_OPTIONS_HELPTEXT_HELP,
	IDS_LOCKSCREEN_OPTIONS_SET_SHORTCUTS,
	IDS_LOCKSCREEN_OPTIONS_EDIT_SHORTCUTS,
	IDS_LOCKSCREEN_OPTIONS_SET_SHORTCUTS_ON_LOCKSCREEN,
	IDS_LOCKSCREEN_OPTIONS_TAP_SHORTCUTS,
	IDS_LOCKSCREEN_OPTIONS_DRAG_DROP_SHORTCUTS,
	IDS_LOCKSCREEN_OPTIONS_SELECT_APPLICATIONS,
	IDS_LOCKSCREEN_OPTIONS_LOCK_SCREEN_TITLE_GUIDE_TEXT,
	IDS_LOCKSCREEN_OPTIONS_APP_STRING_MAX,	/* 45 */
};

Evas_Object *lockscreen_options_util_create_navigation(Evas_Object * parent);
Evas_Object *lockscreen_options_util_create_layout(Evas_Object * parent,
						   const char *file,
						   const char *group);
void lockscreen_options_util_create_seperator(Evas_Object * genlist);
void lockscreen_options_util_create_underline(Evas_Object * genlist);
char *lockscreen_optoins_get_string(int id);

#endif				/* __OPENLOCK_SETTING_UTIL_H__ */
