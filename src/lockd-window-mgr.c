/*
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
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
 */

#include <Elementary.h>
#include <Ecore_X.h>
#include <utilX.h>
#include <ui-gadget.h>
#include <vconf.h>
#include <bundle.h>
#include <appcore-efl.h>
#include <app.h>

#include "lockd-debug.h"
#include "lockd-window-mgr.h"

#define PACKAGE 			"starter"
#define SOS_KEY_COUNT		3
#define SOS_KEY_INTERVAL	0.5

#define _MAKE_ATOM(a, s)                              \
   do {                                               \
        a = ecore_x_atom_get(s);                      \
        if (!a)                                       \
          fprintf(stderr,                             \
                  "##s creation failed.\n"); \
   } while(0)

#define STR_ATOM_PANEL_SCROLLABLE_STATE         "_E_MOVE_PANEL_SCROLLABLE_STATE"

struct _lockw_data {
	Evas_Object *main_win;
	Evas_Object *main_layout;

	Ecore_X_Window lock_x_window;

	Ecore_Event_Handler *h_keydown;
	Ecore_Event_Handler *h_wincreate;
	Ecore_Event_Handler *h_winshow;

#if 0
	Ecore_Timer *pTimerId; /* volume key timer */
	int volume_key_cnt;
#endif

	int phone_lock_state;	/* 0 : disable, 1 : enable */
	int phone_lock_app_pid;
};

#if 0
Eina_Bool volume_key_expire_cb(void *pData)
{
	int api_ret = 0;
	int vconf_val = 0;
	lockw_data *lockw = (lockw_data *) pData;

	_DBG("volume_key_expire_cb..!!");

	lockw->volume_key_cnt = 0;

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _lockd_window_key_down_cb(void *data, int type, void *event)
{
	Ecore_Event_Key *ev = event;
	lockw_data *lockw = (lockw_data *) data;
	int vconf_val = 0;
	int ret = 0;

	LOCKD_DBG("Key Down CB : %s", ev->keyname);

	ret = vconf_get_int(VCONFKEY_MESSAGE_SOS_STATE, &vconf_val);

	if(ret != 0)
	{
		LOCKD_ERR("_lockd_window_key_down_cb:VCONFKEY_MESSAGE_SOS_STATE FAILED");
		return ECORE_CALLBACK_CANCEL;
	}

	if (!strcmp(ev->keyname, KEY_VOLUMEUP) || !strcmp(ev->keyname, KEY_VOLUMEDOWN)) {
		if (vconf_val == VCONFKEY_MESSAGE_SOS_IDLE) {
			if (lockw->volume_key_cnt == 0) {
				lockw->volume_key_cnt++;
				LOCKD_DBG("Volume key is pressed %d times", lockw->volume_key_cnt);
				lockw->pTimerId = ecore_timer_add(SOS_KEY_INTERVAL, volume_key_expire_cb, lockw);
			} else if (lockw->volume_key_cnt == SOS_KEY_COUNT) {
				LOCKD_DBG("SOS msg invoked");
				if (lockw->pTimerId != NULL) {
					ecore_timer_del(lockw->pTimerId);
					lockw->pTimerId = NULL;
					lockw->volume_key_cnt =0;
					vconf_set_int(VCONFKEY_MESSAGE_SOS_STATE, VCONFKEY_MESSAGE_SOS_INVOKED);
					ecore_x_pointer_grab(lockw->lock_x_window);
				}
			} else {
				if (lockw->pTimerId != NULL) {
					ecore_timer_del(lockw->pTimerId);
					lockw->pTimerId = NULL;
					lockw->volume_key_cnt++;
					LOCKD_DBG("Volume key is pressed %d times", lockw->volume_key_cnt);
					lockw->pTimerId = ecore_timer_add(SOS_KEY_INTERVAL, volume_key_expire_cb, lockw);
				}
			}
		}
	} else if (!strcmp(ev->keyname, KEY_HOME)) {
		if (vconf_val != VCONFKEY_MESSAGE_SOS_IDLE) {
			LOCKD_DBG("Home key is pressed set to idle", lockw->volume_key_cnt);
			vconf_set_int(VCONFKEY_MESSAGE_SOS_STATE, VCONFKEY_MESSAGE_SOS_IDLE);
			ecore_x_pointer_ungrab();
		}
	}

	return ECORE_CALLBACK_PASS_ON;
}
#endif

static int
_lockd_window_check_validate_rect(Ecore_X_Display * dpy, Ecore_X_Window window)
{
	Ecore_X_Window root;
	Ecore_X_Window child;

	int rel_x = 0;
	int rel_y = 0;
	int abs_x = 0;
	int abs_y = 0;

	unsigned int width = 0;
	unsigned int height = 0;
	unsigned int border = 0;
	unsigned int depth = 0;
	unsigned int root_w = 0;
	unsigned int root_h = 0;

	Eina_Bool ret = FALSE;

	root = ecore_x_window_root_first_get();
	XGetGeometry(dpy, root, &root, &rel_x, &rel_y, &root_w, &root_h, &border, &depth);
	LOCKD_DBG("root rel_x[%d] rel_y[%d] border[%d] width[%d] height[%d]", rel_x, rel_y, border, root_w, root_h);
	if (XGetGeometry
	    (dpy, window, &root, &rel_x, &rel_y, &width, &height, &border,
	     &depth)) {
		if (XTranslateCoordinates
		    (dpy, window, root, 0, 0, &abs_x, &abs_y, &child)) {
		    LOCKD_DBG("abs_x[%d] abs_y[%d] border[%d] width[%d] height[%d]", abs_x, abs_y, border, width, height);
			if ((abs_x - border) >= root_w || (abs_y - border) >= root_h
			    || (width + abs_x) <= 0 || (height + abs_y) <= 0) {
				ret = FALSE;
			} else {
				ret = (width == root_w) && (height == root_h);
			}
		}
	}
	return ret;
}

static Evas_Object *lockd_create_main_window(const char *pkgname)
{
	Evas_Object *eo = NULL;
	int w, h;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	eo = elm_win_add(NULL, pkgname, ELM_WIN_BASIC);
	if (eo) {
		elm_win_title_set(eo, pkgname);
		elm_win_borderless_set(eo, EINA_TRUE);
		ecore_x_window_size_get(ecore_x_window_root_first_get(),
					&w, &h);
		LOCKD_DBG("%s, %d, w = %d, h = %d", __func__, __LINE__, w, h);
		evas_object_resize(eo, w, h);
	}
	return eo;
}

static Evas_Object *lockd_create_main_layout(Evas_Object * parent)
{
	Evas_Object *ly = NULL;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	ly = elm_layout_add(parent);
	if (!ly) {
		LOCKD_ERR("UI layout add error");
		return NULL;
	}

	elm_layout_theme_set(ly, "layout", "application", "default");
	evas_object_size_hint_weight_set(ly, EVAS_HINT_EXPAND,
					 EVAS_HINT_EXPAND);
	evas_object_show(ly);

	elm_win_indicator_mode_set(parent, ELM_WIN_INDICATOR_SHOW);

	return ly;
}

static void _lockd_phone_lock_alpha_ug_layout_cb(ui_gadget_h ug,
						 enum ug_mode mode, void *priv)
{
	lockw_data *lockw = (lockw_data *) priv;;
	Evas_Object *base = NULL;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	if (!ug || !lockw)
		return;

	base = ug_get_layout(ug);
	if (!base)
		return;

	switch (mode) {
	case UG_MODE_FULLVIEW:
		evas_object_size_hint_weight_set(base, EVAS_HINT_EXPAND,
						 EVAS_HINT_EXPAND);
		ug_disable_effect(ug);	/* not use effect when destroy phone lock UG  */
		evas_object_show(base);
		break;
	case UG_MODE_FRAMEVIEW:
		/* elm_layout_content_set(lockw->main_layout, "content", base); *//* not used */
		break;
	default:
		break;
	}
}

static void _lockd_phone_lock_alpha_ug_result_cb(ui_gadget_h ug,
						 app_control_h app_control, void *priv)
{
	int alpha;
	const char *val1 = NULL, *val2 = NULL;
	lockw_data *lockw = (lockw_data *) priv;;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	if (!ug || !lockw)
		return;

	app_control_get_extra_data(app_control, "name", &val1);

	if (val1 == NULL)
		return;

	LOCKD_DBG("val1 = %s", val1);

	app_control_get_extra_data(app_control, "result", &val2);

	if (val2 == NULL){
		if(val1 != NULL)
			free(val1);
		return;
	}

	LOCKD_DBG("val2 = %s", val2);


	if (!strcmp(val1, "phonelock-ug")) {
		if (!strcmp(val2, "success")) {
			LOCKD_DBG("password verified. Unlock!\n");
		}
	} else if (!strcmp(val1, "phonelock-ug-alpha")) {
		alpha = atoi(val2);
	}

	if(val1 != NULL)
		free(val1);

	if(val2 != NULL)
		free(val2);
}

static void _lockd_phone_lock_alpha_ug_destroy_cb(ui_gadget_h ug,
						  void *priv)
{
	lockw_data *lockw = (lockw_data *) priv;;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	if (!ug || !lockw)
		return;

	ug_destroy(ug);
	ug = NULL;
	lockd_destroy_ug_window(lockw);
	vconf_set_bool(VCONFKEY_LOCKSCREEN_PHONE_LOCK_VERIFICATION, TRUE);
}

static void _lockd_ug_window_set_win_prop(void *data, int type)
{
	Ecore_X_Window w;
	Evas_Object *win = NULL;
	lockw_data *lockw = (lockw_data *) data;;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	if (!lockw)
		return;

	win = lockw->main_win;

	w = elm_win_xwindow_get(win);

	if (type == ECORE_X_WINDOW_TYPE_NORMAL) {
		ecore_x_netwm_window_type_set(w, ECORE_X_WINDOW_TYPE_NORMAL);
	} else if (type == ECORE_X_WINDOW_TYPE_NOTIFICATION) {
		ecore_x_netwm_window_type_set(w,
					      ECORE_X_WINDOW_TYPE_NOTIFICATION);
		utilx_set_system_notification_level(ecore_x_display_get(), w,
						    UTILX_NOTIFICATION_LEVEL_NORMAL);
	}
}

static void _lockd_ug_window_set_key_grab(void *data)
{
	Ecore_X_Window w;
	int ret = 0;
	Evas_Object *win = NULL;
	lockw_data *lockw = (lockw_data *) data;;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	if (!lockw)
		return;

	win = lockw->main_win;
	w = elm_win_xwindow_get(win);
	ret = utilx_grab_key(ecore_x_display_get(), w, KEY_HOME, EXCLUSIVE_GRAB);
	if(ret)
	{
		LOCKD_ERR("Key grab error : KEY_HOME");
	}
	ret = utilx_grab_key(ecore_x_display_get(), w, KEY_CONFIG, TOP_POSITION_GRAB);
	if(ret)
	{
		LOCKD_ERR("Key grab error : KEY_CONFIG");
	}
}

static void _lockd_ug_window_set_key_ungrab(void *data)
{
	Ecore_X_Window xwin;
	Ecore_X_Display *disp = NULL;
	lockw_data *lockw = (lockw_data *) data;;

	if (!lockw)
		return;

	LOCKD_DBG("%s, %d", __func__, __LINE__);
	disp = ecore_x_display_get();
	xwin = elm_win_xwindow_get(lockw->main_win);

	utilx_ungrab_key(disp, xwin, KEY_HOME);
	utilx_ungrab_key(disp, xwin, KEY_CONFIG);
}

static void _lockd_ug_window_vconf_call_state_changed_cb(keynode_t * node,
							 void *data)
{
	int api_ret = 0;
	int vconf_val = 0;
	lockw_data *lockw = (lockw_data *) data;

	if (!lockw)
		return;

	LOCKD_DBG("%s, %d", __func__, __LINE__);
	api_ret = vconf_get_int(VCONFKEY_CALL_STATE, &vconf_val);
	if (api_ret != 0) {
		LOCKD_DBG("fail to get vconf key value"
			  );
	} else {
		if (vconf_val == VCONFKEY_CALL_OFF) {
			LOCKD_DBG("call off state..");
			_lockd_ug_window_set_win_prop(lockw,
						      ECORE_X_WINDOW_TYPE_NOTIFICATION);
		} else {
			LOCKD_DBG("call on state..");
			_lockd_ug_window_set_win_prop(lockw,
						      ECORE_X_WINDOW_TYPE_NORMAL);
		}
	}
	return;
}

static void _lockd_ug_window_register_vconf_changed(void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	if (vconf_notify_key_changed
	    (VCONFKEY_CALL_STATE, _lockd_ug_window_vconf_call_state_changed_cb,
	     data) != 0) {
		LOCKD_DBG("Fail to register");
	}
}

static void _lockd_ug_window_unregister_vconf_changed(void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	if (vconf_ignore_key_changed
	    (VCONFKEY_CALL_STATE,
	     _lockd_ug_window_vconf_call_state_changed_cb) != 0) {
		LOCKD_DBG("Fail to unregister");
	}

}

void _lockd_window_transient_for_set(void *data)
{
	lockw_data *lockw = (lockw_data *) data;
	Ecore_X_Window xwin_ug;
	Ecore_X_Window xwin_lock;

	if (!lockw)
		return;

	LOCKD_DBG("%s, %d", __func__, __LINE__);
	xwin_ug = elm_win_xwindow_get(lockw->main_win);
	xwin_lock = lockw->lock_x_window;

	LOCKD_DBG("ug win id : %x, and lock win id is :%x", xwin_ug, xwin_lock);
	ecore_x_icccm_transient_for_set(xwin_ug, xwin_lock);
}

Eina_Bool _lockd_window_set_window_property_timer_cb(void *data)
{
	Ecore_X_Window win = (Ecore_X_Window) data;
	LOCKD_DBG
	    ("[MINSU] win id(%x) set lock screen window property to notification and level low\n",
	     win);

	/*  Set notification type */
	ecore_x_netwm_window_type_set(win, ECORE_X_WINDOW_TYPE_NOTIFICATION);

	/*  Set notification's priority */
	utilx_set_system_notification_level(ecore_x_display_get(), win,
					    UTILX_NOTIFICATION_LEVEL_LOW);

	return EINA_FALSE;
}

static Window get_user_created_window(Window win)
{
	Atom type_ret = 0;
	int ret, size_ret = 0;
	unsigned long num_ret = 0, bytes = 0;
	unsigned char *prop_ret = NULL;
	unsigned int xid;
	Atom prop_user_created_win;

	prop_user_created_win =
	    XInternAtom(ecore_x_display_get(), "_E_USER_CREATED_WINDOW", False);

	ret =
	    XGetWindowProperty(ecore_x_display_get(), win,
			       prop_user_created_win, 0L, 1L, False, 0,
			       &type_ret, &size_ret, &num_ret, &bytes,
			       &prop_ret);

	if (ret != Success) {
		if (prop_ret)
			XFree((void *)prop_ret);
		return win;
	} else if (!prop_ret) {
		return win;
	}

	memcpy(&xid, prop_ret, sizeof(unsigned int));
	XFree((void *)prop_ret);

	return xid;

}

int
lockd_window_mgr_get_focus_win_pid(void)
{
	Ecore_X_Window x_win_focused = 0;
	int pid = 0;
	int ret = -1;

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	x_win_focused = ecore_x_window_focus_get();
	ret = ecore_x_netwm_pid_get(x_win_focused, &pid);
	if(ret != 1) {
		LOCKD_ERR("Can't get pid for focus x window (%x)\n", x_win_focused);
		return -1;
	}
	LOCKD_DBG("PID(%d) for focus x window (%x)\n", pid, x_win_focused);

	return pid;
}

Eina_Bool
lockd_window_set_window_property(lockw_data * data, int lock_app_pid,
				 void *event)
{
	Ecore_X_Event_Window_Create *e = event;
	Ecore_X_Window user_window = 0;
	lockw_data *lockw = (lockw_data *) data;
	int pid = 0;

	if (!lockw) {
		return EINA_FALSE;
	}
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	user_window = get_user_created_window((Window) (e->win));

	int ret = ecore_x_netwm_pid_get(user_window, &pid);
	if(ret != 1) {
		return EINA_FALSE;
	}

	LOCKD_DBG("Check PID(%d) window. (lock_app_pid : %d)\n", pid,
		  lock_app_pid);

	if (lock_app_pid == pid) {
		if (_lockd_window_check_validate_rect
		    (ecore_x_display_get(), user_window) == TRUE) {
			lockw->lock_x_window = user_window;
			LOCKD_DBG
			    ("This is lock application. Set window property. win id : %x",
			     user_window);

			/* window effect : fade in /out */
			ecore_x_icccm_name_class_set(user_window, "LOCK_SCREEN",
						     "LOCK_SCREEN");

			/* Set notification type */
			ecore_x_netwm_window_type_set(user_window,
						      ECORE_X_WINDOW_TYPE_NOTIFICATION);

			/* Set notification's priority */
			utilx_set_system_notification_level(ecore_x_display_get
							    (), user_window,
							    UTILX_NOTIFICATION_LEVEL_NORMAL);
			/* Set opaque state */
			utilx_set_window_opaque_state(ecore_x_display_get(),
						      user_window,
						      UTILX_OPAQUE_STATE_ON);
			return EINA_TRUE;
		}
	}
	return EINA_FALSE;
}

Eina_Bool
lockd_window_set_window_effect(lockw_data * data, int lock_app_pid, void *event)
{
	Ecore_X_Event_Window_Create *e = event;
	Ecore_X_Window user_window = 0;
	int pid = 0;

	user_window = get_user_created_window((Window) (e->win));
	int ret = ecore_x_netwm_pid_get(user_window, &pid);
	if(ret != 1) {
		return EINA_FALSE;
	}

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	LOCKD_DBG("PID(%d) window created. (lock_app_pid : %d)\n", pid,
		  lock_app_pid);

	if (lock_app_pid == pid) {
		if (_lockd_window_check_validate_rect
		    (ecore_x_display_get(), user_window) == TRUE) {
			LOCKD_DBG
			    ("This is lock application. Disable window effect. win id : %x\n",
			     user_window);

			/*utilx_set_window_effect_state(ecore_x_display_get(),
						      user_window, 0);*/

			Ecore_X_Atom ATOM_WINDOW_EFFECT_ENABLE = 0;
			unsigned int effect_state = 0;
			ATOM_WINDOW_EFFECT_ENABLE = ecore_x_atom_get("_NET_CM_WINDOW_EFFECT_ENABLE");
			if (ATOM_WINDOW_EFFECT_ENABLE) {
				ecore_x_window_prop_card32_set(user_window, ATOM_WINDOW_EFFECT_ENABLE, &effect_state, 1);
			} else {
				LOCKD_ERR("ecore_x_atom_get() failed");
			}
			return EINA_TRUE;
		}
	}
	return EINA_FALSE;
}

void
lockd_window_set_scroll_property(lockw_data * data, int lock_type)
{
	lockw_data *lockw = (lockw_data *) data;
	Ecore_X_Atom ATOM_PANEL_SCROLLABLE_STATE = 0;
	unsigned int val[3];

	if (!lockw) {
		return;
	}
	LOCKD_DBG("%s, %d", __func__, __LINE__);


	// init atoms
    _MAKE_ATOM(ATOM_PANEL_SCROLLABLE_STATE, STR_ATOM_PANEL_SCROLLABLE_STATE );

	if (lock_type == 1) {
		val[0] = 0; // always enable F
		val[1] = 0; // quickpanel enable F
		val[2] = 0; // apptray enable F
	} else {
		val[0] = 0; // always enable F
		val[1] = 1; // quickpanel enable T
		val[2] = 0; // apptray enable F
	}
	ecore_x_window_prop_card32_set(lockw->lock_x_window,
                         		   ATOM_PANEL_SCROLLABLE_STATE,
                          		   val,
                           		   3);
}

void lockd_window_set_phonelock_pid(lockw_data * data, int phone_lock_pid)
{
	lockw_data *lockw = (lockw_data *) data;

	if (!lockw) {
		return;
	}
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	lockw->phone_lock_app_pid = phone_lock_pid;
	LOCKD_DBG("%s, %d, lockw->phone_lock_app_pid = %d", __func__, __LINE__,
		  lockw->phone_lock_app_pid);
}

void
lockd_window_mgr_ready_lock(void *data, lockw_data * lockw,
			    Eina_Bool(*create_cb) (void *, int, void *),
			    Eina_Bool(*show_cb) (void *, int, void *))
{
	Ecore_X_Window root_window;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	if (lockw == NULL) {
		LOCKD_ERR("lockw is NULL.");
		return;
	}

	/* For getting window x event */
	root_window = ecore_x_window_root_first_get();
	ecore_x_window_client_sniff(root_window);

	/* Register window create CB */
	lockw->h_wincreate =
	    ecore_event_handler_add(ECORE_X_EVENT_WINDOW_CREATE, create_cb,
				    data);
	lockw->h_winshow =
	    ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW, show_cb, data);

#if 0
	lockw->volume_key_cnt = 0;

	/* Register keydown event handler */
	lockw->h_keydown =
	    ecore_event_handler_add(ECORE_EVENT_KEY_DOWN,
				    _lockd_window_key_down_cb, lockw);
#endif
}

void lockd_window_mgr_finish_lock(lockw_data * lockw)
{
	Ecore_X_Window root_window;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	if (lockw == NULL) {
		LOCKD_ERR("lockw is NULL.");
		return;
	}

	/* delete getting window x event */
	root_window = ecore_x_window_root_first_get();
	ecore_x_window_client_sniff(root_window);

	/* delete window create event handler */
	if (lockw->h_wincreate != NULL) {
		ecore_event_handler_del(lockw->h_wincreate);
		lockw->h_wincreate = NULL;
	}
	if (lockw->h_winshow != NULL) {
		ecore_event_handler_del(lockw->h_winshow);
		lockw->h_winshow = NULL;
	}

	ecore_x_pointer_ungrab();

	/* delete keydown event handler */
	if (lockw->h_keydown != NULL) {
		ecore_event_handler_del(lockw->h_keydown);
		lockw->h_keydown = NULL;
	}
}

lockw_data *lockd_window_init(void)
{
	lockw_data *lockw = NULL;
	long pid;

	/* Create lockd window data */
	lockw = (lockw_data *) malloc(sizeof(lockw_data));
	memset(lockw, 0x0, sizeof(lockw_data));

	pid = getpid();

	return lockw;
}

void lockd_create_ug_window(void *data)
{
	lockw_data *lockw = NULL;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	lockw = (lockw_data *) data;
	if (!lockw) {
		return;
	}

	lockd_destroy_ug_window(lockw);

	/* create main window */
	lockw->main_win = lockd_create_main_window(PACKAGE);

	/* create main layout */
	/* remove indicator in running time */
	/* lockw->main_layout = lockd_create_main_layout(lockw->main_win); */
	appcore_set_i18n(PACKAGE, NULL);
	_lockd_ug_window_set_key_grab(lockw);
	_lockd_ug_window_register_vconf_changed(lockw);
	lockw->phone_lock_state = 1;
}

void lockd_destroy_ug_window(void *data)
{
	lockw_data *lockw = NULL;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	lockw = (lockw_data *) data;
	if (!lockw) {
		return;
	}

	_lockd_ug_window_set_key_ungrab(lockw);
	_lockd_ug_window_unregister_vconf_changed(lockw);

	if (lockw->main_win) {
		evas_object_del(lockw->main_win);
		lockw->main_win = NULL;
	}
	lockw->phone_lock_state = 0;
}

void lockd_show_phonelock_alpha_ug(void *data)
{
	lockw_data *lockw = NULL;
	app_control_h app_control;
	struct ug_cbs cbs = { 0, };
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	lockw = (lockw_data *) data;
	if (!lockw) {
		return;
	}

	cbs.layout_cb = _lockd_phone_lock_alpha_ug_layout_cb;
	cbs.result_cb = _lockd_phone_lock_alpha_ug_result_cb;
	cbs.destroy_cb = _lockd_phone_lock_alpha_ug_destroy_cb;
	cbs.priv = (void *)data;

	app_control_create(&app_control);

	app_control_add_extra_data(app_control, "phone-lock-type", "phone-lock");
	app_control_add_extra_data(app_control, "window-type", "alpha");

	elm_win_alpha_set(lockw->main_win, TRUE);
	evas_object_color_set(lockw->main_win, 0, 0, 0, 0);

	/* window effect : fade in /out */
	ecore_x_icccm_name_class_set(elm_win_xwindow_get(lockw->main_win),
				     "LOCK_SCREEN", "LOCK_SCREEN");

	UG_INIT_EFL(lockw->main_win, UG_OPT_INDICATOR_ENABLE);
	ug_create(NULL, "phone-lock-efl", UG_MODE_FULLVIEW, app_control, &cbs);
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	app_control_destroy(app_control);

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	evas_object_show(lockw->main_win);
	_lockd_ug_window_set_win_prop(lockw, ECORE_X_WINDOW_TYPE_NOTIFICATION);
}
