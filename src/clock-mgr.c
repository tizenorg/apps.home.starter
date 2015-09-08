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

#include <ail.h>
#include <bundle.h>
#include <Elementary.h>
#include <Ecore_X.h>
#include <Ecore_Input.h>
#include <utilX.h>
#include <vconf.h>
#include <dd-display.h>
#if 0 // Disable temporarily for TIZEN 2.3 build
#include <message_port.h>
#endif

#include "starter_w.h"
#include "dbus-util_w.h"
#include "util.h"


#define APP_CONTROL_OPERATION_MAIN_KEY "__APP_SVC_OP_TYPE__"
#define APP_CONTROL_OPERATION_MAIN_VALUE "http://tizen.org/appcontrol/operation/main"
#define APP_CONTROL_OPERATION_DEFAULT_VALUE "http://tizen.org/appcontrol/operation/default"


#ifndef VCONFKEY_SHEALTH_VIEW_TYPE_STR
#define VCONFKEY_SHEALTH_VIEW_TYPE_STR "memory/shealth/view_type"
#endif

#ifndef VCONFKEY_HERE_TBT_STATUS_INT
#define VCONFKEY_HERE_TBT_STATUS_INT "memory/private/navigation/guidance"
#endif

#ifndef VCONFKEY_STARTER_RESERVED_APPS_STATUS
#define VCONFKEY_STARTER_RESERVED_APPS_STATUS "memory/starter/reserved_apps_status"		/*  2 digits for reserved apps & popup
																						 *	10 : popup
																						 *	01 : apps
																						 *								*/
#endif

#ifndef VCONFKEY_SETTING_SIMPLE_CLOCK_MODE
#define VCONFKEY_SETTING_SIMPLE_CLOCK_MODE	"db/setting/simpleclock_mode"
#endif



#define W_LOCKSCREEN_PKGNAME            "com.samsung.w-lockscreen"

#define W_ALPM_CLOCK_PKGNAME "com.samsung.alpm-clock-manager"
#define ALPM_CLOCK_OP "ALPM_CLOCK_OP"
#define ALPM_CLOCK_SHOW "alpm_clock_show"
#define ALPM_CLOCK_HIDE "alpm_clock_hide"

#define HOME_OPERATION_KEY "home_op"
#define POWERKEY_VALUE "powerkey"

#define PM_UNLOCK_TIMER_SEC 0.3

#define RESERVED_DISPLAY_MESSAGE_PORT_ID "Home.Reserved.Display"
#define RESERVED_DISPLAY_MESSAGE_KEY_STATE "state"
#define RESERVED_DISPLAY_MESSAGE_STATE_SHOW "show"
#define RESERVED_DISPLAY_MESSAGE_STATE_HIDE "hide"
#define RESERVED_DISPLAY_MESSAGE_STATE_POPUP_SHOW "popup_show"
#define RESERVED_DISPLAY_MESSAGE_STATE_POPUP_HIDE "popup_hide"

#define SHEALTH_SLEEP_PKGNAME 		"com.samsung.shealth.sleep"

static int _check_reserved_popup_status(void *data)
{
	int val = 0;
	int tmp = 0;
	struct appdata *ad = (struct appdata *)data;
	if (ad == NULL) {
		_E("ad is NULL");
		return -1;
	}

	vconf_get_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, &val);

	_W("Current reserved apps status : %x", val);

	tmp = val;
	tmp = tmp & 0x10;
	if(tmp == 0x10){
		if(aul_app_is_running(ad->reserved_popup_app_id) == 1){
			return TRUE;
		}
		else{
			_E("%s is not running now.", ad->reserved_popup_app_id);
			ad->reserved_popup_app_id = NULL;
			val = val ^ 0x10;
			vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, val);
			_W("now reserved apps status %x", val);
			return FALSE;
		}
	}
	else{
		return FALSE;
	}

}

static int _check_reserved_apps_status(void *data)
{
	int val = 0;
	struct appdata *ad = (struct appdata *)data;
	if (ad == NULL) {
		_E("ad is NULL");
		return -1;
	}

	vconf_get_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, &val);

	_W("Current reserved apps status : %x", val);

	if(val > 0){
		val = TRUE;
	}
	else{
		val = FALSE;
	}

	return val;
}

static void reserved_apps_message_received_cb(int local_port_id, const char *remote_app_id, const char *remote_port, bool trusted_port, bundle* msg)
{
	ret_if(remote_app_id == NULL);
	ret_if(msg == NULL);
	struct appdata *ad = starter_get_app_data();
	Eina_List *l = NULL;
	char *state = NULL;
	char *data = NULL;
	char *appid = NULL;
	int count = 0;
	int i = 0;
	int val = 0;
	int tmp = 0;

	vconf_get_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, &val);
	_W("current reserved apps status : %x", val);

	state = bundle_get_val(msg, RESERVED_DISPLAY_MESSAGE_KEY_STATE);
	_W("appid [%s], msg value[%s]", remote_app_id, state);
	if (strncmp(state, RESERVED_DISPLAY_MESSAGE_STATE_SHOW, strlen(state)) == 0) {
		//reserved app is start
		EINA_LIST_FOREACH(ad->reserved_apps_list, l, data){
			if(strncmp(data, remote_app_id, strlen(data)) == 0){
				_W("%s is already in the list", data);
				ad->reserved_apps_list = eina_list_remove_list(ad->reserved_apps_list, l);
				break;
			}
		}
		appid = strdup(remote_app_id);
		ad->reserved_apps_list = eina_list_append(ad->reserved_apps_list, appid);
		val = val | 0x01;
		vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, val);
		_W("now reserved apps status %x", val);
	}
	else if (strncmp(state, RESERVED_DISPLAY_MESSAGE_STATE_HIDE, strlen(state)) == 0){
		//reserved app is stop
		EINA_LIST_FOREACH(ad->reserved_apps_list, l, data){
			if(strncmp(data, remote_app_id, strlen(data)) == 0){
				ad->reserved_apps_list = eina_list_remove_list(ad->reserved_apps_list, l);
				break;
			}
		}
		count = eina_list_count(ad->reserved_apps_list);
		if(count == 0){
			_W("there is no reserved app.");
			tmp = val;
			val = tmp & 0x01;
			if(val == 0x01){
				tmp = tmp ^ 0x01;
			}
			vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, tmp);
			_W("now reserved apps status %x", tmp);
		}
	}
	else if (strncmp(state, RESERVED_DISPLAY_MESSAGE_STATE_POPUP_SHOW, strlen(state)) == 0){
		//reserved popup is show
		ad->reserved_popup_app_id = strdup(remote_app_id);
		val = val | 0x0010;
		vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, val);
		_W("now reserved apps status %x", val);
	}
	else if (strncmp(state, RESERVED_DISPLAY_MESSAGE_STATE_POPUP_HIDE, strlen(state)) == 0){
		//reserved popup is hide
		if(ad->reserved_popup_app_id == NULL){
			_E("there is no reserved popup already");
		}
		else{
			if(strncmp(ad->reserved_popup_app_id, remote_app_id, strlen(ad->reserved_popup_app_id)) == 0){
				ad->reserved_popup_app_id = NULL;
				tmp = val;
				val = tmp & 0x10;
				if(val == 0x10){
					tmp = tmp ^ 0x10;
				}
				vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, tmp);
				_W("now reserved apps status %x", tmp);
			}
		}
	}
	else{
		// no info
	}
	EINA_LIST_REVERSE_FOREACH(ad->reserved_apps_list, l, data){
		_W("%d. %s", i++, data);
	}
}
#if 0
static void _shealth_view_type_changed_cb(keynode_t* node, void *data)
{
	char *val = NULL;
	struct appdata *ad = (struct appdata *)data;

	_D("%s, %d", __func__, __LINE__);

	if (ad == NULL) {
		_E("ad is NULL");
		return;
	}

	if (node) {
		val = vconf_keynode_get_str(node);
	} else {
		val = vconf_get_str(VCONFKEY_SHEALTH_VIEW_TYPE_STR);
	}

	clock_mgr_set_reserved_apps_status(val, STARTER_RESERVED_APPS_SHEALTH, ad);
}

static void _here_navigation_status_changed_cb(keynode_t* node, void *data)
{
	int val = NULL;
	struct appdata *ad = (struct appdata *)data;

	_D("%s, %d", __func__, __LINE__);

	if (ad == NULL) {
		_E("ad is NULL");
		return;
	}

	if (node) {
		val = vconf_keynode_get_int(node);
	} else {
		vconf_get_int(VCONFKEY_HERE_TBT_STATUS_INT, &val);
	}

	clock_mgr_set_reserved_apps_status(val, STARTER_RESERVED_APPS_HERE, ad);
}

void clock_mgr_set_reserved_apps_status(void *info, int type, void *data){
	char *str_info = NULL;
	int int_info = NULL;
	int val = 0;
	int tmp = 0;
	struct appdata *ad = (struct appdata *)data;
	_D("%s, %d", __func__, __LINE__);
	if (ad == NULL) {
		_E("ad is NULL");
		return;
	}

	vconf_get_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, &val);
	_W("current reserved apps status : %x", val);

	switch(type){
		case STARTER_RESERVED_APPS_SHEALTH:{
			str_info = (char *)info;
			_W("Shealth status is changed (%s)", str_info);
			if (strncmp(str_info, "exercise", strlen(str_info)) == 0){
				val = val | 0x1000;
				if(val >= 0x1100){
					val = val ^ 0x0100;
				}
				vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, val);
				_W("now reserved apps status %x", val);
			}
			else if (strncmp(str_info, "sleep", strlen(str_info)) == 0){
				val = val | 0x0100;
				if(val >= 0x1100){
					val = val ^ 0x1000;
				}
				vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, val);
				_W("now reserved apps status %x", val);
			}
			else{
				tmp = val;
				val = tmp & 0x1000;
				if(val == 0x1000){
					tmp = tmp ^ 0x1000;
				}
				val = tmp & 0x0100;
				if(val == 0x0100){
					tmp = tmp ^ 0x0100;
				}
				vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, tmp);
				_W("now reserved apps status %x", tmp);
			}
			break;
		}
		case STARTER_RESERVED_APPS_NIKE:{
			int_info = (int)info;
			_W("Nike status is changed (%d)", int_info);
			if(int_info == 1){
				val = val | 0x0010;
				vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, val);
				_W("now reserved apps status %x", val);
			}
			else{
				tmp = val;
				val = tmp & 0x0010;
				if(val == 0x0010){
					tmp = tmp ^ 0x0010;
				}
				vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, tmp);
				_W("now reserved apps status %x", tmp);
			}
			break;
		}
		case STARTER_RESERVED_APPS_HERE:{
			int_info = (int)info;
			_W("Here status is changed (%d)", int_info);
			if(int_info == 1){
				val = val | 0x0001;
				vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, val);
				_W("now reserved apps status %x", val);
			}
			else{
				tmp = val;
				val = tmp & 0x0001;
				if(val == 0x0001){
					tmp = tmp ^ 0x0001;
				}
				vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, tmp);
				_W("now reserved apps status %x", tmp);
			}
			break;
		}
		default:{
			_E("Unknown reserved app.");
			break;
		}
	}
	return 0;
}
#endif

static void _wake_up_setting_changed_cb(keynode_t* node, void *data)
{
	int val = 0;
	struct appdata *ad = (struct appdata *)data;

	_D("%s, %d", __func__, __LINE__);

	if (ad == NULL) {
		_E("ad is NULL");
		return;
	}

	if(vconf_get_int(VCONFKEY_WMS_WAKEUP_BY_GESTURE_SETTING, &val) < 0) {
		_E("Failed to get VCONFKEY_WMS_WAKEUP_BY_GESTURE_SETTING");
		val = 1;
	}
	ad->wake_up_setting = val;
	_W("wake_up_setting is changed to [%d]", ad->wake_up_setting);
}

static void _ambient_mode_setting_changed_cb(keynode_t* node, void *data)
{
	int ambient_mode = 0;
	struct appdata *ad = (struct appdata *)data;
	ret_if(!ad);

	ambient_mode = vconf_keynode_get_bool(node);
	_D("ambient mode : %d", ambient_mode);

	if (ambient_mode) {
		_D("launch w-clock-viewer");
		ad->pid_clock_viewer = w_launch_app(W_CLOCK_VIEWER_PKGNAME, NULL);
	} else {
		_D("kill w-clock-viewer(%d)", ad->pid_clock_viewer);
		if (ad->pid_clock_viewer > 0) {
			aul_kill_pid(ad->pid_clock_viewer);
			ad->pid_clock_viewer = 0;
		}
	}

	ad->ambient_mode = ambient_mode;
	_W("ambient_mode_setting is changed to [%d]", ad->ambient_mode);
}


static Eina_Bool _w_lcdoff_idler_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	_D("%s, %d", __func__, __LINE__);

	w_open_app(ad->home_pkgname);

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _w_launch_pm_unlock_idler_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	_D("%s, %d", __func__, __LINE__);
	display_unlock_state(LCD_OFF, PM_SLEEP_MARGIN);

	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _pm_unlock_timer_cb(void *data){
	_D("%s, %d", __func__, __LINE__);
	ecore_idler_add(_w_launch_pm_unlock_idler_cb, data);
	return ECORE_CALLBACK_CANCEL;
}


static void _notify_pm_lcdoff_cb(keynode_t * node, void *data)
{

	int ret = -1;
	int setup_wizard_state = -1;
	bundle *b = NULL;
	int lock_type = 0;
	int tutorial_state = -1;
	Eina_List *l = NULL;
	char *info = NULL;
	int count = 0;
	int lcdoff_source = vconf_keynode_get_int(node);
	struct appdata *ad = (struct appdata *)data;


	if (ad == NULL) {
		_E("ad is NULL");
		return;
	}

	if (lcdoff_source < 0) {
		_E("Cannot get VCONFKEY, error (%d)", lcdoff_source);
		return;
	}

	_W("LCD OFF by lcd off source[%d], wake_up_setting[%d], ALPM_clock_state[%d]", lcdoff_source, ad->wake_up_setting, ad->ALPM_clock_state);

#if 0
	if(csc_feature_get_bool(CSC_FEATURE_DEF_BOOL_HOME_LAUNCH_LOCK_WHEN_LCD_ON) == CSC_FEATURE_BOOL_TRUE){
		_E("CHC bin.");
		vconf_get_int(VCONFKEY_SETAPPL_PRIVACY_LOCK_TYPE_INT, &lock_type);
		if(lock_type == 1){
			int r = 0;
			//PM LOCK - don't go to sleep
			display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
			_E("CHC bin & lock type is %d", lock_type);
			r = w_launch_app(W_LOCKSCREEN_PKGNAME, NULL);

			ecore_timer_add(PM_UNLOCK_TIMER_SEC, _pm_unlock_timer_cb, ad);
		}
	}
#endif

	if (vconf_get_int(VCONFKEY_SETUP_WIZARD_STATE, &setup_wizard_state) < 0) {
		_E("Failed to get [%s]", VCONFKEY_SETUP_WIZARD_STATE);
	} else {
		if (setup_wizard_state == VCONFKEY_SETUP_WIZARD_LOCK) {
			_E("don't react for lcd off, setup wizard state is [%d]", setup_wizard_state);
			return;
		}
	}



#if 0
	if (_check_reserved_apps_status(ad)) {
		_E("_check_running_heath = > don't react for lcd off except ALPM");
		if (ad->ambient_mode == 1)  {
			//Step1. Launch ALPM clock
			b = bundle_create();
			if(!b) {
				_E("Failed to create bundle");
				return;
			}
			//bundle_add(b, APP_CONTROL_OPERATION_MAIN_KEY, APP_CONTROL_OPERATION_MAIN_VALUE);
			bundle_add(b, ALPM_CLOCK_OP, ALPM_CLOCK_SHOW);

			//PM LOCK - don't go to sleep
			display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);

			ad->pid_ALPM_clock = 0;
			ret = w_launch_app(W_ALPM_CLOCK_PKGNAME, b);
			if (ret >= 0) {
				ad->pid_ALPM_clock =ret;
				_SECURE_D("[%s] is launched, pid=[%d]", W_ALPM_CLOCK_PKGNAME, ad->pid_ALPM_clock );
			}
			if(b){
				bundle_free(b);
			}
			//Step2. Do not raise Homescreen
			ecore_timer_add(PM_UNLOCK_TIMER_SEC, _pm_unlock_timer_cb, ad);
		}
		return;
	}


	if (ad->wake_up_setting == SETTING_WAKE_UP_GESTURE_CLOCK) {
		//Step1. Launch ALPM clock
		b = bundle_create();
		if(!b) {
			_E("Failed to create bundle");
			return;
		}
		//bundle_add(b, APP_CONTROL_OPERATION_MAIN_KEY, APP_CONTROL_OPERATION_MAIN_VALUE);
		bundle_add(b, ALPM_CLOCK_OP, ALPM_CLOCK_SHOW);

		//PM LOCK - don't go to sleep
		display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);

		ad->pid_ALPM_clock = 0;
		ret = w_launch_app(W_ALPM_CLOCK_PKGNAME, b);
		if (ret >= 0) {
			ad->pid_ALPM_clock =ret;
			_SECURE_D("[%s] is launched, pid=[%d]", W_ALPM_CLOCK_PKGNAME, ad->pid_ALPM_clock );
		}
		if(b) {
			bundle_free(b);
		}

		if(vconf_get_int("db/private/com.samsung.w-home/tutorial", &tutorial_state) < 0) {
			_E("Failed to get tutorial status");

		}
		if(!tutorial_state) {
			//Step2. Raise Homescreen
			w_open_app(ad->home_pkgname);
		}
		ecore_timer_add(PM_UNLOCK_TIMER_SEC, _pm_unlock_timer_cb, ad);
	} else if (ad->ambient_mode == 1)  {

		//Step1. Launch ALPM clock
		b = bundle_create();
		if(!b) {
			_E("Failed to create bundle");
			return;
		}
		//bundle_add(b, APP_CONTROL_OPERATION_MAIN_KEY, APP_CONTROL_OPERATION_MAIN_VALUE);
		bundle_add(b, ALPM_CLOCK_OP, ALPM_CLOCK_SHOW);

		//PM LOCK - don't go to sleep
		display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);

		ad->pid_ALPM_clock = 0;
		ret = w_launch_app(W_ALPM_CLOCK_PKGNAME, b);
		if (ret >= 0) {
			ad->pid_ALPM_clock =ret;
			_SECURE_D("[%s] is launched, pid=[%d]", W_ALPM_CLOCK_PKGNAME, ad->pid_ALPM_clock );
		}
		if(b) {
			bundle_free(b);
		}
		//Step2. Do not raise Homescreen
		ecore_timer_add(PM_UNLOCK_TIMER_SEC, _pm_unlock_timer_cb, ad);
	}


#else
	if(_check_reserved_popup_status(ad)){
		_W("reserved popup is on top. do nothing");
		if((ad->ambient_mode == 1) && (ad->ALPM_clock_state == 0)){
			starter_dbus_alpm_clock_signal_send(ad);
		}
		return;
	}

	if (_check_reserved_apps_status(ad)) {
		_W("reserved app is running now. raise it.");

		EINA_LIST_FOREACH(ad->reserved_apps_list, l, info){
			if(strncmp(info, SHEALTH_SLEEP_PKGNAME, strlen(info)) == 0){
				_W("%s is in the list. check running state", info);
				if(aul_app_is_running(info) == 1){
					_W("%s is now running. raise it.", info);
					//PM LOCK - don't go to sleep
					display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
					w_open_app(info);
					ecore_timer_add(PM_UNLOCK_TIMER_SEC, _pm_unlock_timer_cb, ad);
					return;
				}
				else{
					_W("%s is not running now", info);
					ad->reserved_apps_list = eina_list_remove_list(ad->reserved_apps_list, l);
					break;
				}
			}
		}

		EINA_LIST_REVERSE_FOREACH(ad->reserved_apps_list, l, info){
			if(aul_app_is_running(info) == 1){
				//PM LOCK - don't go to sleep
				display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
				w_open_app(info);
				ecore_timer_add(PM_UNLOCK_TIMER_SEC, _pm_unlock_timer_cb, ad);
				break;
			}
			else{
				_W("%s is not running now", info);
				ad->reserved_apps_list = eina_list_remove_list(ad->reserved_apps_list, l);
				continue;
			}
		}
		count = eina_list_count(ad->reserved_apps_list);
		if(count == 0){
			_W("there is no reserved app.");
			vconf_set_int(VCONFKEY_STARTER_RESERVED_APPS_STATUS, 0);
		}
	}

	if ((ad->ambient_mode == 1) && (ad->ALPM_clock_state == 0))  {
#if 0
		//Step1. Launch ALPM clock
		b = bundle_create();
		if(!b) {
			_E("Failed to create bundle");
			return;
		}
		//bundle_add(b, APP_CONTROL_OPERATION_MAIN_KEY, APP_CONTROL_OPERATION_MAIN_VALUE);
		bundle_add(b, ALPM_CLOCK_OP, ALPM_CLOCK_SHOW);

		//PM LOCK - don't go to sleep
		display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);

		ad->pid_ALPM_clock = 0;
		ret = w_launch_app(W_ALPM_CLOCK_PKGNAME, b);
		if (ret >= 0) {
			ad->pid_ALPM_clock =ret;
			_SECURE_D("[%s] is launched, pid=[%d]", W_ALPM_CLOCK_PKGNAME, ad->pid_ALPM_clock );
		}
		if(b){
			bundle_free(b);
		}
		//Step2. Do not raise Homescreen
		ecore_timer_add(PM_UNLOCK_TIMER_SEC, _pm_unlock_timer_cb, ad);
#endif
		starter_dbus_alpm_clock_signal_send(ad);
	} else {
		int val = 0;
		if(vconf_get_bool(VCONFKEY_SETTING_SIMPLE_CLOCK_MODE, &val) < 0) {
			_E("Failed to get VCONFKEY_SETTING_SIMPLE_CLOCK_MODE");
		}


		// Not yet fix the setting concept.
		//if ((_check_reserved_apps_status(ad) == FALSE) && (val == TRUE) && (ad->wake_up_setting == SETTING_WAKE_UP_GESTURE_CLOCK)) {
		if ((_check_reserved_apps_status(ad) == FALSE) && (ad->wake_up_setting == SETTING_WAKE_UP_GESTURE_CLOCK) && (ad->ALPM_clock_state == 0)) {
			//_W("Not reserved apss status AND wake_up_setting is CLOCK AND simple clock setting is [%d] => show simple clock..!!", val);
			_W("Not reserved apss status AND wake_up_setting is CLOCK => show simple clock..!!");
#if 0
			//Step1. Launch ALPM clock
			b = bundle_create();
			if(!b) {
				_E("Failed to create bundle");
				return;
			}
			//bundle_add(b, APP_CONTROL_OPERATION_MAIN_KEY, APP_CONTROL_OPERATION_MAIN_VALUE);
			bundle_add(b, ALPM_CLOCK_OP, ALPM_CLOCK_SHOW);

			//PM LOCK - don't go to sleep
			display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);

			ad->pid_ALPM_clock = 0;
			ret = w_launch_app(W_ALPM_CLOCK_PKGNAME, b);
			if (ret >= 0) {
				ad->pid_ALPM_clock =ret;
				_SECURE_D("[%s] is launched, pid=[%d]", W_ALPM_CLOCK_PKGNAME, ad->pid_ALPM_clock );
			}
			if(b){
				bundle_free(b);
			}
			//Step2. Do not raise Homescreen
			ecore_timer_add(PM_UNLOCK_TIMER_SEC, _pm_unlock_timer_cb, ad);
#endif
			starter_dbus_alpm_clock_signal_send(ad);
		}
	}
#endif
}


void init_clock_mgr(void *data)
{
	int status = -1;
	int ret = -1;
	int val = -1;
	int reserved_apps_msg_port_id = 0;
	struct appdata *ad = (struct appdata *)data;

	_W("init_clock_mgr.!!");
#if 0
	//register message port for reserved apps
	if ((reserved_apps_msg_port_id = message_port_register_local_port(RESERVED_DISPLAY_MESSAGE_PORT_ID, reserved_apps_message_received_cb)) <= 0) {
		_E("Failed to register reserved_apps message port cb");
	}
	_E("port_id:%d", reserved_apps_msg_port_id);
	ad->reserved_apps_local_port_id = reserved_apps_msg_port_id;
#endif
	if(init_dbus_ALPM_clock_state_signal(ad) < 0) {
		_E("Failed to init_dbus_ALPM_clock_state_signal");
	}

	//register wake up gesture setting changed.
	if(vconf_get_int(VCONFKEY_WMS_WAKEUP_BY_GESTURE_SETTING, &val) < 0) {
		_E("Failed to get VCONFKEY_WMS_WAKEUP_BY_GESTURE_SETTING");
		val = 1;
	}
	ad->wake_up_setting = val;
	_W("wake_up_setting : %d", ad->wake_up_setting);

	if (vconf_notify_key_changed(VCONFKEY_WMS_WAKEUP_BY_GESTURE_SETTING, _wake_up_setting_changed_cb, ad) < 0) {
		_E("Failed to add the callback for VCONFKEY_WMS_WAKEUP_BY_GESTURE_SETTING changed");
	}

	//register ambient mode changed.
	if(vconf_get_bool(VCONFKEY_SETAPPL_AMBIENT_MODE_BOOL, &val) < 0) {
		_E("Failed to get VCONFKEY_SETAPPL_AMBIENT_MODE_BOOL");
		val = 0;
	}
	ad->ambient_mode = val;
	_W("ambient_mode : %d", ad->ambient_mode);

	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_AMBIENT_MODE_BOOL, _ambient_mode_setting_changed_cb, ad) < 0) {
		_E("Failed to add the callback for VCONFKEY_SETAPPL_AMBIENT_MODE_BOOL changed");
	}

	if (vconf_notify_key_changed(VCONFKEY_PM_LCDOFF_SOURCE, _notify_pm_lcdoff_cb, ad) != 0) {
		_E("Fail vconf_notify_key_changed : VCONFKEY_PM_LCDOFF_SOURCE");
	}

#if 0
	if (vconf_notify_key_changed(VCONFKEY_SHEALTH_VIEW_TYPE_STR, _shealth_view_type_changed_cb, ad) < 0) {
		_E("Failed to add the callback for VCONFKEY_SHEALTH_VIEW_TYPE_STR changed");
	}

	if (vconf_notify_key_changed(VCONFKEY_HERE_TBT_STATUS_INT, _here_navigation_status_changed_cb, ad) < 0) {
		_E("Failed to add the callback for VCONFKEY_HERE_TBT_STATUS_INT changed");
	}
#endif
}



void fini_clock_mgr(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	if(vconf_ignore_key_changed(VCONFKEY_WMS_WAKEUP_BY_GESTURE_SETTING, _wake_up_setting_changed_cb) < 0) {
		_E("Failed to ignore the VCONFKEY_WMS_WAKEUP_BY_GESTURE_SETTING");
	}

	if(vconf_ignore_key_changed(VCONFKEY_PM_LCDOFF_SOURCE, _notify_pm_lcdoff_cb) < 0) {
		_E("Failed to ignore the VCONFKEY_PM_LCDOFF_SOURCE");
	}

	if (vconf_ignore_key_changed(VCONFKEY_SETAPPL_AMBIENT_MODE_BOOL, _ambient_mode_setting_changed_cb) < 0) {
		_E("Failed to ignore the VCONFKEY_SETAPPL_AMBIENT_MODE_BOOL");
	}

#if 0
	if(vconf_ignore_key_changed(VCONFKEY_SHEALTH_VIEW_TYPE_STR, _shealth_view_type_changed_cb) < 0) {
		_E("Failed to ignore the VCONFKEY_SHEALTH_VIEW_TYPE_STR");
	}

	if(vconf_ignore_key_changed(VCONFKEY_HERE_TBT_STATUS_INT, _here_navigation_status_changed_cb) < 0) {
		_E("Failed to ignore the VCONFKEY_HERE_TBT_STATUS_INT");
	}
#endif

#if 0 // Disable temporarily for TIZEN 2.3 build
	if (ad->reserved_apps_local_port_id >= 0) {
		if (message_port_unregister_local_port(ad->reserved_apps_local_port_id) != MESSAGE_PORT_ERROR_NONE) {
			_E("Failed to unregister reserved apps message port cb");
		}
	}
#endif

}



// End of a file
