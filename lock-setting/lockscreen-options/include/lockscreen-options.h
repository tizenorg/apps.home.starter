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



#ifndef __LOCKSCREEN_OPTIONS_H__
#define __LOCKSCREEN_OPTIONS_H__

#include <Elementary.h>
#include <libintl.h>
#include <ui-gadget.h>

#define PKGNAME "ug-lockscreen-options"

#define _EDJ(o)			elm_layout_edje_get(o)
#define _S(str)			dgettext("sys_string", str)
#define _(s)			dgettext(PKGNAME, s)
#define dgettext_noop(s)	(s)
#define N_(s)			dgettext_noop(s)

enum{
	ENUM_LOCKSCREEN_GENLIST_STYLE_SEPERATOR = 0,
	ENUM_LOCKSCREEN_GENLIST_STYLE_1TEXT1ICON,
	ENUM_LOCKSCREEN_GENLIST_STYLE_2TEXT1ICON,
	ENUM_LOCKSCREEN_GENLIST_STYLE_HELP,
	ENUM_LOCKSCREEN_GENLIST_STYLE_UNDERLINE
};

#define LOCKSCREEN_GENLIST_STYLE_SEPERATOR "dialogue/separator/21/with_line"
#define LOCKSCREEN_GENLIST_STYLE_1TEXT1ICON "dialogue/1text.1icon"
#define LOCKSCREEN_GENLIST_STYLE_2TEXT1ICON "dialogue/2text.1icon.6"
#define LOCKSCREEN_GENLIST_STYLE_HELP "multiline/1text"
#define LOCKSCREEN_GENLIST_STYLE_UNDERLINE "dialogue/separator/1/with_line"


typedef struct _lockscreen_options_ug_data {
	int index;

	Evas_Object *win_main;
	Evas_Object *base;
	ui_gadget_h ug;

	Evas_Object *navi_bar;

	Elm_Gen_Item_Class itc_separator;
	Elm_Gen_Item_Class itc_menu_1text1icon;
	Elm_Gen_Item_Class itc_help_1text;

} lockscreen_options_ug_data;

#endif
