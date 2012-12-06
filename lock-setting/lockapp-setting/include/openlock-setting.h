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



#ifndef __openlock_setting_H__
#define __openlock_setting_H__

#include <Elementary.h>
#include <libintl.h>
#include <ui-gadget.h>

#define PKGNAME "ug-openlock-setting"

#define _EDJ(o)			elm_layout_edje_get(o)
#define dgettext_noop(s)	(s)

typedef struct _openlock_ug_data {
	Evas_Object *win_main;
	Evas_Object *base;
	ui_gadget_h ug;

	Evas_Object *navi_bar;
	;
} openlock_ug_data;

#endif				/* __openlock_setting_H__ */
