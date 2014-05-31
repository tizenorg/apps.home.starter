/*
 *  starter
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Seungtaek Chung <seungtaek.chung@samsung.com>, Mi-Ju Lee <miju52.lee@samsung.com>, Xi Zhichan <zhichan.xi@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */

#include <ail.h>
#include <aul.h>
#include <dlog.h>
#include <Ecore.h>
#include <Ecore_X.h>
#include <Evas.h>
#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <sys/shm.h>
#include <vconf.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/extensions/Xcomposite.h>
#include <X11/extensions/XShm.h>

#include "menu_daemon.h"
#include "util.h"
#include "virtual_canvas.h"
#include "starter-util.h"
#include "xmonitor.h"

#define PACKAGE_LEN 1024



static struct info {
	Ecore_Event_Handler *create_handler;
	Ecore_Event_Handler *destroy_handler;
	Ecore_Event_Handler *focus_in_handler;
	Ecore_Event_Handler *focus_out_handler;
	Ecore_Timer *make_file_timer;
	int is_top;
} xmonitor_info = {
	.create_handler = NULL,
	.destroy_handler = NULL,
	.focus_in_handler = NULL,
	.focus_out_handler = NULL,
	.make_file_timer = NULL,
	.is_top = VCONFKEY_IDLE_SCREEN_TOP_FALSE,
};



int errno;



static inline int _get_pid(Ecore_X_Window win)
{
	int pid;
	Ecore_X_Atom atom;
	unsigned char *in_pid = NULL;
	int num;

	atom = ecore_x_atom_get("X_CLIENT_PID");
	if (ecore_x_window_prop_property_get(win, atom, ECORE_X_ATOM_CARDINAL,
				sizeof(int), &in_pid, &num) == EINA_FALSE) {
		if(in_pid != NULL) {
			free(in_pid);
			in_pid = NULL;
		}
		if (ecore_x_netwm_pid_get(win, &pid) == EINA_FALSE) {
			_E("Failed to get PID from a window 0x%X", win);
			return -EINVAL;
		}
	} else {
		pid = *(int *)in_pid;
		free(in_pid);
	}

	return pid;
}



#define VCONFKEY_IDLE_SCREEN_FOCUSED_PACKAGE "memory/idle-screen/focused_package"
#define BUFSZE 1024
bool _set_idlescreen_top(Ecore_X_Window win)
{
	int ret;

	if (!win) win = ecore_x_window_focus_get();

	int focused_pid;
	focused_pid = _get_pid(win);
	retv_if(focused_pid <= 0, false);

	do { // Set the focused package into the vconf key.
		char pkgname[BUFSZE];
		if (AUL_R_OK == aul_app_get_pkgname_bypid(focused_pid, pkgname, sizeof(pkgname))) {
			if (strcmp(APP_TRAY_PKG_NAME, pkgname)) {
				ret = vconf_set_str(VCONFKEY_IDLE_SCREEN_FOCUSED_PACKAGE, pkgname);
				_SECURE_D("Try to set the vconfkey[%s] as [%s]", VCONFKEY_IDLE_SCREEN_FOCUSED_PACKAGE, pkgname);
				if (0 != ret) {
					_E("cannot set string for FOCUSED_WINDOW");
				}
			}
		} else _D("cannot get pkgname from pid[%d]", focused_pid);
	} while (0);

	do { // Set the vconf key whether the idle-screen is top or not.
		int is_top;
		is_top = menu_daemon_is_homescreen(focused_pid)?
			VCONFKEY_IDLE_SCREEN_TOP_TRUE : VCONFKEY_IDLE_SCREEN_TOP_FALSE;
		if (is_top != xmonitor_info.is_top) {
			ret = vconf_set_int(VCONFKEY_IDLE_SCREEN_TOP, is_top);
			retv_if(0 != ret, false);
			xmonitor_info.is_top = is_top;
			_D("set the key of idlescreen_is_top as %d", is_top);
		}
	} while (0);

	return true;
}



#define TIMER_AFTER_LAUNCHING 5.0f
static Eina_Bool _create_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Create *info = event;

	_D("Create a window[%x]", info->win);

	ecore_x_window_client_sniff(info->win);

	return ECORE_CALLBACK_PASS_ON;
}



static Eina_Bool _destroy_cb(void *data, int type, void *event)
{
	return ECORE_CALLBACK_PASS_ON;
}



static Eina_Bool _focus_in_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Focus_In *info = event;

	_D("Focus in a window[%x]", info->win);

	retv_if(false == _set_idlescreen_top(info->win), ECORE_CALLBACK_PASS_ON);

	return ECORE_CALLBACK_PASS_ON;
}



static Eina_Bool _focus_out_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Focus_Out *info = event;

	_D("Focus out a window[%x]", info->win);

	return ECORE_CALLBACK_PASS_ON;
}



static inline void _sniff_all_windows(void)
{
	Ecore_X_Window root;
	Ecore_X_Window ret;
	struct stack_item *new_item;
	struct stack_item *item;
	Eina_List *win_stack = NULL;
	struct stack_item {
		Ecore_X_Window *wins;
		int nr_of_wins;
		int i;
	};

	root = ecore_x_window_root_first_get();
	ecore_x_window_sniff(root);

	new_item = malloc(sizeof(*new_item));
	if (!new_item) {
		_E("Error(%s)\n", strerror(errno));
		return;
	}

	new_item->nr_of_wins = 0;
	new_item->wins =
		ecore_x_window_children_get(root, &new_item->nr_of_wins);
	new_item->i = 0;

	if (new_item->wins)
		win_stack = eina_list_append(win_stack, new_item);
	else
		free(new_item);

	while ((item = eina_list_nth(win_stack, 0))) {
		win_stack = eina_list_remove(win_stack, item);

		if (!item->wins) {
			free(item);
			continue;
		}

		while (item->i < item->nr_of_wins) {
			ret = item->wins[item->i];

			/*
			 * Now we don't need to care about visibility of window,
			 * just check whether it is registered or not.
			 * (ecore_x_window_visible_get(ret))
			 */
			ecore_x_window_client_sniff(ret);

			new_item = malloc(sizeof(*new_item));
			if (!new_item) {
				_E("Error %s\n", strerror(errno));
				item->i++;
				continue;
			}

			new_item->i = 0;
			new_item->nr_of_wins = 0;
			new_item->wins =
				ecore_x_window_children_get(ret,
							&new_item->nr_of_wins);
			if (new_item->wins) {
				win_stack =
					eina_list_append(win_stack, new_item);
			} else {
				free(new_item);
			}

			item->i++;
		}

		free(item->wins);
		free(item);
	}

	return;
}



int xmonitor_init(void)
{
	if (ecore_x_composite_query() == EINA_FALSE)
		_D("====> COMPOSITOR IS NOT ENABLED");

	xmonitor_info.create_handler =
		ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CREATE, _create_cb, NULL);
	goto_if(NULL == xmonitor_info.create_handler, Error);

	xmonitor_info.destroy_handler =
		ecore_event_handler_add(ECORE_X_EVENT_WINDOW_DESTROY, _destroy_cb, NULL);
	goto_if(NULL == xmonitor_info.destroy_handler, Error);

	xmonitor_info.focus_in_handler =
		ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_IN, _focus_in_cb, NULL);
	goto_if(NULL == xmonitor_info.focus_in_handler, Error);

	xmonitor_info.focus_out_handler =
		ecore_event_handler_add(ECORE_X_EVENT_WINDOW_FOCUS_OUT, _focus_out_cb, NULL);
	goto_if(NULL == xmonitor_info.focus_out_handler, Error);

	_sniff_all_windows();
	if (false == _set_idlescreen_top(0)) _E("cannot set idlescreen_is_top");

	return 0;

Error:
	if (xmonitor_info.create_handler) {
		ecore_event_handler_del(xmonitor_info.create_handler);
		xmonitor_info.create_handler = NULL;
	} else return -EFAULT;

	if (xmonitor_info.destroy_handler) {
		ecore_event_handler_del(xmonitor_info.destroy_handler);
		xmonitor_info.destroy_handler = NULL;
	} else return -EFAULT;

	if (xmonitor_info.focus_in_handler) {
		ecore_event_handler_del(xmonitor_info.focus_in_handler);
		xmonitor_info.focus_in_handler = NULL;
	} else return -EFAULT;

	if (xmonitor_info.focus_out_handler) {
		ecore_event_handler_del(xmonitor_info.focus_out_handler);
		xmonitor_info.focus_out_handler = NULL;
	} else return -EFAULT;

	return -EFAULT;
}

void xmonitor_fini(void)
{
	ecore_event_handler_del(xmonitor_info.create_handler);
	xmonitor_info.create_handler = NULL;

	ecore_event_handler_del(xmonitor_info.destroy_handler);
	xmonitor_info.destroy_handler = NULL;

	ecore_event_handler_del(xmonitor_info.focus_in_handler);
	xmonitor_info.focus_in_handler = NULL;

	ecore_event_handler_del(xmonitor_info.focus_out_handler);
	xmonitor_info.focus_out_handler = NULL;
}

/* End of a file */
