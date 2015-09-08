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
#include <dd-deviced.h>
#include <syspopup_caller.h>
#include <utilX.h>
#include <vconf.h>
#include <system/media_key.h>
#include <aul.h>
#include <feedback.h>

#include "direct-access.h"
#include "hw_key.h"
#include "lock-daemon.h"
#include "menu_daemon.h"
#include "util.h"
#include "dbus-util.h"

#define _DEF_BEZEL_AIR_COMMAND		0

#define CAMERA_PKG_NAME "com.samsung.camera-app"
#define CALLLOG_PKG_NAME "com.samsung.calllog"
#define SVOICE_PKG_NAME "com.samsung.svoice"
#define MUSIC_PLAYER_PKG_NAME "com.samsung.music-player"
#define MUSIC_PLAYER_LITE_PKG_NAME "com.samsung.music-player-lite"

#ifdef FEATURE_LITE
#define SAFETY_ASSURANCE_PKG_NAME "com.samsung.emergency-message-lite"
#else
#define SAFETY_ASSURANCE_PKG_NAME "com.samsung.emergency-message"
#endif

#define WEBPAGE_PKG_NAME "com.samsung.browser"
#define MAIL_PKG_NAME "com.samsung.email"
#define DIALER_PKG_NAME "com.samsung.phone"
#define STR_ATOM_XKEY_COMPOSITION "_XKEY_COMPOSITION"
#define STR_ATOM_KEYROUTER_NOTIWINDOW "_KEYROUTER_NOTIWINDOW"

#define SERVICE_OPERATION_MAIN_KEY "__APP_SVC_OP_TYPE__"
#define SERVICE_OPERATION_MAIN_VALUE "http://tizen.org/appcontrol/operation/main"

#define SVOICE_LAUNCH_BUNDLE_KEY "domain"
#define SVOICE_LAUNCH_BUNDLE_VALUE "earjack"
#define SVOICE_LAUNCH_BUNDLE_HOMEKEY_VALUE "home_key"

#define _VCONF_QUICK_COMMADN_NAME		"db/aircommand/enabled"
#define _VCONF_QUICK_COMMADN_FLOATING	"db/aircommand/floating"
#define HALL_COVERED_STATUS		0

#define POWERKEY_TIMER_SEC 0.700
#define POWERKEY_TIMER_MSEC 700
#define POWERKEY_CODE "XF86PowerOff"

#define LONG_PRESS_TIMER_SEC 0.4
#define HOMEKEY_TIMER_SEC 0.2
#define UNLOCK_TIMER_SEC 0.8
#define CANCEL_KEY_TIMER_SEC 0.3

static struct {
	Ecore_X_Window win;
	Ecore_Event_Handler *key_up;
	Ecore_Event_Handler *key_down;
	Ecore_Event_Handler *client_msg_hd;
	Ecore_Event_Handler *two_finger_double_tap;
	Ecore_Event_Handler *two_finger_long_tap;
	Ecore_Event_Handler *bezel_gesture;
	Ecore_Timer *long_press_timer;
	Ecore_Timer *single_timer;
	Ecore_Timer *homekey_timer;
	Ecore_Timer *media_long_press;
	Ecore_Timer *client_msg_timer;
	Ecore_Timer *cancel_key_timer;
	Eina_Bool cancel;
	Eina_Bool is_cancel;
	Eina_Bool ignore_home_key;
	Eina_Bool enable_safety_assurance;
	Ecore_X_Window keyrouter_notiwindow;
	int homekey_count;
	int powerkey_count;
	unsigned int powerkey_time_stamp;
} key_info = {
	.win = 0x0,
	.key_up = NULL,
	.key_down = NULL,
	.client_msg_hd = NULL,
	.long_press_timer = NULL,
	.single_timer = NULL,
	.homekey_timer = NULL,
	.media_long_press = NULL,
	.client_msg_timer = NULL,
	.cancel_key_timer = NULL,
	.cancel = EINA_FALSE,
	.is_cancel = EINA_FALSE,
	.ignore_home_key = EINA_FALSE,
	.enable_safety_assurance = EINA_FALSE,
	.keyrouter_notiwindow = 0x0,
	.two_finger_double_tap = NULL,
	.two_finger_long_tap = NULL,
	.homekey_count = 0,
	.powerkey_count = 0,
	.powerkey_time_stamp = 0,
};

static Ecore_Timer *gtimer_launch = NULL;
static bool gbenter_idle = false;
static bool gbinit_floatwin = false;
static int gbRegisterDuobleTap = 0;



static Eina_Bool _launch_taskmgr_cb(void* data)
{
	int val = -1;
	_D("Launch TASKMGR");

	key_info.long_press_timer = NULL;

	if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) {
		_E("Cannot get VCONFKEY for lock state");
	}
	if (val == VCONFKEY_IDLE_LOCK) {
		_E("lock state, ignore home key long press..!!");
		return ECORE_CALLBACK_CANCEL;
	}

	if (lockd_get_hall_status() == HALL_COVERED_STATUS) {
		_E("scover is closed, ignore home key long press..!!");
		return ECORE_CALLBACK_CANCEL;
	}

	bundle *b;
	b = bundle_create();
	retv_if(NULL == b, ECORE_CALLBACK_CANCEL);
	bundle_add(b, "HIDE_LAUNCH", "0");

	int ret = menu_daemon_launch_app(menu_daemon_get_taskmgr_pkgname(), b);
	goto_if(0 > ret, OUT);

	if(ret > 0){
		if(starter_dbus_set_oomadj(ret, OOM_ADJ_VALUE_DEFAULT) < 0){
			_E("failed to send oom dbus signal");
		}
	}

OUT:
	bundle_free(b);
	return ECORE_CALLBACK_CANCEL;
}



#define DESKDOCK_PKG_NAME "com.samsung.desk-dock"
static Eina_Bool _launch_by_home_key(void *data)
{
	int lock_state = (int) data;
	int ret = 0;
	_D("lock_state : %d ", lock_state);

	key_info.single_timer = NULL;

	if (lock_state == VCONFKEY_IDLE_LOCK) {
		return ECORE_CALLBACK_CANCEL;
	}

	if (lockd_get_hall_status() == HALL_COVERED_STATUS) {
		_D("scover is closed, ignore home key..!!");
		return ECORE_CALLBACK_CANCEL;
	}

	if (lockd_get_lock_state() > VCONFKEY_IDLE_UNLOCK) {
		return ECORE_CALLBACK_CANCEL;
	}

	int cradle_status = menu_daemon_get_cradle_status();
	if (0 < cradle_status) {
		int ret;
		_SECURE_D("Cradle is enabled to [%d], we'll launch the desk dock[%s]", cradle_status, DESKDOCK_PKG_NAME);
		ret = menu_daemon_open_app(DESKDOCK_PKG_NAME);
		if (ret < 0) {
			_SECURE_E("cannot launch package %s(err:%d)", DESKDOCK_PKG_NAME, ret);
		}
	}

	ret = menu_daemon_open_homescreen(NULL);

	if(ret > 0){
		starter_dbus_home_raise_signal_send();
	}

	return ECORE_CALLBACK_CANCEL;
}



inline static Eina_Bool _launch_svoice(void)
{
	const char *pkg_name = NULL;
	bundle *b = NULL;
	int val;

	pkg_name = menu_daemon_get_svoice_pkg_name();
	retv_if(NULL == pkg_name, EINA_FALSE);

#if 1
	if (vconf_get_int(VCONFKEY_SETAPPL_PSMODE , &val) < 0) {
		_E("Cannot get VCONFKEY");
	}
	if (val == SETTING_PSMODE_EMERGENCY) {
		_D("Emergency mode, ignore svoice key..!!");
		return EINA_FALSE;
	}
#endif

	if (vconf_get_int(VCONFKEY_SVOICE_OPEN_VIA_HOME_KEY, &val) < 0) {
		_D("Cannot get VCONFKEY");
	}

	if (val != 1) {
		_D("Launch nothing");
		return EINA_FALSE;
	}

#if 1
	if (!strcmp(pkg_name, SVOICE_PKG_NAME)) {
		b = bundle_create();
		retv_if(!b, EINA_FALSE);

		bundle_add(b, SVOICE_LAUNCH_BUNDLE_KEY, SVOICE_LAUNCH_BUNDLE_HOMEKEY_VALUE);
		bundle_add(b, SERVICE_OPERATION_MAIN_KEY, SERVICE_OPERATION_MAIN_VALUE);
	}

	if (menu_daemon_launch_app(pkg_name, b) < 0)
		_SECURE_E("Failed to launch %s", pkg_name);

	if (b != NULL)
		bundle_free(b);
#else
	if (menu_daemon_open_app(pkg_name) < 0)
		_SECURE_E("Failed to open %s", pkg_name);
#endif
	return EINA_TRUE;
}



static void _launch_safety_assurance(void)
{
	_SECURE_D("Launch %s", SAFETY_ASSURANCE_PKG_NAME);
	if (menu_daemon_open_app(SAFETY_ASSURANCE_PKG_NAME) < 0) {
		_SECURE_E("Cannot open %s", SAFETY_ASSURANCE_PKG_NAME);
	}
}


static Eina_Bool _launch_svoice_cb(void* data)
{
	key_info.media_long_press = NULL;

	int val = -1;

	if (vconf_get_int(VCONFKEY_SETAPPL_PSMODE , &val) < 0) {
		_E("Cannot get VCONFKEY");
	}
	if (val == SETTING_PSMODE_EMERGENCY) {
		_D("Emergency mode, ignore KEY_MEDIA key..!!");
		return ECORE_CALLBACK_CANCEL;
	}

	if (vconf_get_int(VCONFKEY_SVOICE_OPEN_VIA_EARPHONE_KEY, &val) < 0) {
		_E("Cannot get VCONFKEY");
	}
	if (1 == val) {
		_D("Launch SVOICE");
#if 1
		bundle *b;
		b = bundle_create();
		retv_if(!b, ECORE_CALLBACK_CANCEL);

		bundle_add(b, SVOICE_LAUNCH_BUNDLE_KEY, SVOICE_LAUNCH_BUNDLE_VALUE);
		bundle_add(b, SERVICE_OPERATION_MAIN_KEY, SERVICE_OPERATION_MAIN_VALUE);
		if (menu_daemon_launch_app(SVOICE_PKG_NAME, b) < 0)
			_SECURE_E("Failed to launch %s", SVOICE_PKG_NAME);
		bundle_free(b);
#else
		if (menu_daemon_open_app(SVOICE_PKG_NAME) < 0)
			_SECURE_E("Failed to open %s", SVOICE_PKG_NAME);
#endif
	}

	return ECORE_CALLBACK_CANCEL;
}



#if 0
static Eina_Bool _client_msg_timer_cb(void* data)
{
	_D("_client_msg_timer_cb, safety assurance is enable");

	key_info.enable_safety_assurance = EINA_TRUE;
	_D("Launch SafetyAssurance");
	_launch_safety_assurance();
	key_info.client_msg_timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}
#endif



static Eina_Bool _set_unlock(void *data)
{
	_D("_set_unlock");
	vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_UNLOCK);
	return ECORE_CALLBACK_CANCEL;
}


inline static int _release_home_key(int lock_state)
{
	int val = -1;

	if (vconf_get_int(VCONFKEY_SETAPPL_PSMODE , &val) < 0) {
		_E("Cannot get VCONFKEY");
	}
	if (val != SETTING_PSMODE_EMERGENCY) {
		retv_if(NULL == key_info.long_press_timer, EXIT_SUCCESS);
		ecore_timer_del(key_info.long_press_timer);
		key_info.long_press_timer = NULL;
	}

	if (NULL == key_info.single_timer) {
		key_info.single_timer = ecore_timer_add(0.3, _launch_by_home_key, (void *) lock_state);
		return EXIT_SUCCESS;
	}
	ecore_timer_del(key_info.single_timer);
	key_info.single_timer = NULL;

	if(EINA_TRUE == _launch_svoice()) {
		if (lock_state == VCONFKEY_IDLE_LOCK) {
			ecore_timer_add(0.8, _set_unlock, NULL);
		}
	}
	return EXIT_SUCCESS;
}



inline static void _release_multimedia_key(const char *value)
{
	ret_if(NULL == value);

	_D("Multimedia key is released with %s", value);

	bundle *b;
	b = bundle_create();
	if (!b) {
		_E("Cannot create bundle");
		return;
	}
	bundle_add(b, "multimedia_key", value);

	int ret;
#ifdef FEATURE_LITE
	ret = menu_daemon_launch_app(MUSIC_PLAYER_LITE_PKG_NAME, b);
#else
	ret = menu_daemon_launch_app(MUSIC_PLAYER_PKG_NAME, b);
#endif
	if (ret < 0)
		_E("Failed to launch the running apps, ret : %d", ret);

	bundle_free(b);
}


static Eina_Bool _homekey_timer_cb(void *data)
{
	int direct_access_val = -1;
	int lock_state = (int) data;

	_W("%s, homekey count[%d], lock state[%d]", __func__, key_info.homekey_count, lock_state);
	key_info.homekey_timer = NULL;


	/* Check Direct Access */
	if (vconf_get_int(VCONFKEY_SETAPPL_ACCESSIBILITY_POWER_KEY_HOLD, &direct_access_val) < 0) {
		_E("Cannot get VCONFKEY_SETAPPL_ACCESSIBILITY_POWER_KEY_HOLD");
		direct_access_val = SETTING_POWERKEY_SHORTCUT_OFF;
	}


	if (SETTING_POWERKEY_SHORTCUT_OFF == direct_access_val) {
		/* Direct Access OFF */
		if(key_info.homekey_count%2 == 0) {
			/* double press operation */
			key_info.homekey_count = 0; //initialize powerkey count
			if(EINA_TRUE == _launch_svoice()) {
				if (lock_state == VCONFKEY_IDLE_LOCK) {
					ecore_timer_add(UNLOCK_TIMER_SEC, _set_unlock, NULL);
				}
			}
			return ECORE_CALLBACK_CANCEL;
		}
	} else {
		/* Direct Access ON */
		if(key_info.homekey_count == 2) {
			/* double press operation*/
			key_info.homekey_count = 0; //initialize powerkey count
			if(EINA_TRUE == _launch_svoice()) {
				if (lock_state == VCONFKEY_IDLE_LOCK) {
					ecore_timer_add(UNLOCK_TIMER_SEC, _set_unlock, NULL);
				}
			}
			return ECORE_CALLBACK_CANCEL;
		} else if(key_info.homekey_count >= 3) {
			_W("Launch Direct Access : %d", direct_access_val);
			key_info.homekey_count = 0; //initialize powerkey count
#ifdef FEATURE_LITE
			_E("Not supported in Lite feature");
#else
			if (launch_direct_access(direct_access_val) < 0) {
				_E("Fail Launch Direct Access..!!");
			}
#endif
			return ECORE_CALLBACK_CANCEL;
		}
	}

	/* Single homekey operation */
	key_info.homekey_count = 0; //initialize powerkey count
	_launch_by_home_key(data);
	return ECORE_CALLBACK_CANCEL;

}


static Eina_Bool _key_release_cb(void *data, int type, void *event)
{
	Evas_Event_Key_Up *ev = event;
	int val = -1;
	int lock_state = -1;

	if (!ev) {
		_D("_key_release_cb : Invalid event object");
		return ECORE_CALLBACK_RENEW;
	}

	if (!ev->keyname) {
		_D("_key_release_cb : Invalid event keyname object");
		return ECORE_CALLBACK_RENEW;
	} else {
		_D("_key_release_cb : %s Released", ev->keyname);
	}

	/* Safety Assistance */
	if (!strcmp(ev->keyname, POWERKEY_CODE)) {
		//double current_timestamp = ecore_loop_time_get();
		unsigned int current_timestamp = ev->timestamp;
		_D("current_timestamp[%d] previous_timestamp[%d]", current_timestamp, key_info.powerkey_time_stamp);

		if ((current_timestamp - key_info.powerkey_time_stamp) > POWERKEY_TIMER_MSEC) {
			key_info.powerkey_count = 0;
		}
		key_info.powerkey_count++;
		if (key_info.powerkey_count >= 3) {
			_launch_safety_assurance();
			key_info.powerkey_count = 0;
		}
		_D("powerkey count:%d", key_info.powerkey_count);
		key_info.powerkey_time_stamp = current_timestamp;
	} else {
		key_info.powerkey_count = 0;
	}


	if (!strcmp(ev->keyname, KEY_END)) {
	} else if (!strcmp(ev->keyname, KEY_CONFIG)) {
	} else if (!strcmp(ev->keyname, KEY_SEND)) {
	} else if (!strcmp(ev->keyname, KEY_HOME)) {
		_W("Home Key is released");

		if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &lock_state) < 0) {
			_D("Cannot get VCONFKEY");
		}
		if ((lock_state == VCONFKEY_IDLE_LOCK) && (lockd_get_lock_type() == 1)) {
			_D("phone lock state, ignore home key..!!");
			key_info.homekey_count = 0; //initialize homekey count
			return ECORE_CALLBACK_RENEW;
		}

		if (lockd_get_hall_status() == HALL_COVERED_STATUS) {
			_D("scover is closed, ignore home key..!!");
			key_info.homekey_count = 0; //initialize homekey count
			return ECORE_CALLBACK_RENEW;
		}
#if 0
		_release_home_key(val);
#else
		// Check homekey timer
		if(key_info.homekey_timer) {
			ecore_timer_del(key_info.homekey_timer);
			key_info.homekey_timer = NULL;
			_D("delete homekey timer");
		}

		// Cancel key operation
		if (EINA_TRUE == key_info.cancel) {
			_D("Cancel key is activated");
			key_info.cancel = EINA_FALSE;
			key_info.homekey_count = 0; //initialize homekey count
			return ECORE_CALLBACK_RENEW;
		}
		else{
			ecore_timer_del(key_info.cancel_key_timer);
			key_info.cancel_key_timer = NULL;
			key_info.is_cancel = EINA_FALSE;
			syspopup_destroy_all();
			_D("delete cancelkey timer");
		}

#if 0
		if (vconf_get_int(VCONFKEY_SETAPPL_PSMODE , &val) < 0) {
			_E("Cannot get VCONFKEY");
		}
		if (val != SETTING_PSMODE_EMERGENCY) {
			// Check long press timer
			if(key_info.long_press_timer) {
				ecore_timer_del(key_info.long_press_timer);
				key_info.long_press_timer = NULL;
				_D("delete long press timer");
			} else {
				key_info.homekey_count = 0; //initialize homekey count
				return ECORE_CALLBACK_RENEW;
			}
		}
#else
		// Check long press timer
		if(key_info.long_press_timer) {
			ecore_timer_del(key_info.long_press_timer);
			key_info.long_press_timer = NULL;
			_D("delete long press timer");
		} else {
			key_info.homekey_count = 0; //initialize homekey count
			return ECORE_CALLBACK_RENEW;
		}
#endif
		key_info.homekey_timer = ecore_timer_add(HOMEKEY_TIMER_SEC, _homekey_timer_cb, (void *) lock_state);
		return ECORE_CALLBACK_RENEW;
#endif
	} else if (!strcmp(ev->keyname, KEY_PAUSE)) {
	} else if (!strcmp(ev->keyname, KEY_CANCEL)) {
		_D("CANCEL Key is released");
		key_info.cancel = EINA_FALSE;
	} else if (!strcmp(ev->keyname, KEY_MEDIA)) {
		if (key_info.media_long_press) { // Short press
			ecore_timer_del(key_info.media_long_press);
			key_info.media_long_press = NULL;

			_release_multimedia_key("KEY_PLAYCD");
		}
	} else if (!strcmp(ev->keyname, KEY_APPS)) {
		_D("App tray key is released");
		menu_daemon_launch_app_tray();
	} else if (!strcmp(ev->keyname, KEY_TASKSWITCH)) {
		_D("Task switch key is released");
		_launch_taskmgr_cb(NULL);
	} else if (!strcmp(ev->keyname, KEY_WEBPAGE)) {
		_D("Web page key is released");
		if (menu_daemon_open_app(WEBPAGE_PKG_NAME) < 0)
			_E("Failed to launch the web page");
	} else if (!strcmp(ev->keyname, KEY_MAIL)) {
		_D("Mail key is released");
		if (menu_daemon_open_app(MAIL_PKG_NAME) < 0)
			_E("Failed to launch the mail");
	} else if (!strcmp(ev->keyname, KEY_CONNECT)) {
		_D("Connect key is released");
		if (menu_daemon_open_app(DIALER_PKG_NAME) < 0)
			_E("Failed to launch the dialer");
	} else if (!strcmp(ev->keyname, KEY_SEARCH)) {
		_D("Search key is released");
#if 0
		if (menu_daemon_open_app(SEARCH_PKG_NAME) < 0)
			_E("Failed to launch the search");
#else
		if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) {
			_D("Cannot get VCONFKEY");
		}
		if (val == VCONFKEY_IDLE_LOCK) {
			_D("lock state, ignore search key..!!");
			return ECORE_CALLBACK_RENEW;
		}

		if (lockd_get_hall_status() == HALL_COVERED_STATUS) {
			_D("scover is closed, ignore search key..!!");
			return ECORE_CALLBACK_RENEW;
		}

		if (menu_daemon_launch_search() < 0)
			_E("Failed to launch the search");
#endif
	} else if (!strcmp(ev->keyname, KEY_VOICE)) {
		_D("Voice key is released");
		if (EINA_FALSE == _launch_svoice())
			_E("Failed to launch the svoice");
	}

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool _destroy_syspopup_cb(void* data)
{
	_D("timer for cancel key operation");
	key_info.cancel_key_timer = NULL;
	if(key_info.is_cancel == EINA_TRUE){
		_W("cancel key is activated. Do not destroy syspopup");
		key_info.is_cancel = EINA_FALSE;
		return ECORE_CALLBACK_CANCEL;
	}
	key_info.is_cancel = EINA_FALSE;
	syspopup_destroy_all();
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _key_press_cb(void *data, int type, void *event)
{
	Evas_Event_Key_Down *ev = event;
	int val = -1;

	if (!ev) {
		_D("_key_press_cb : Invalid event object");
		return ECORE_CALLBACK_RENEW;
	}

	if (!ev->keyname) {
		_D("_key_press_cb : Invalid event keyname object");
		return ECORE_CALLBACK_RENEW;
	} else {
		_D("_key_press_cb : %s Pressed", ev->keyname);
	}

	if (!strcmp(ev->keyname, KEY_SEND)) {
		_D("Launch calllog");
		if (menu_daemon_open_app(CALLLOG_PKG_NAME) < 0)
			_SECURE_E("Failed to launch %s", CALLLOG_PKG_NAME);
	} else if(!strcmp(ev->keyname, KEY_CONFIG)) {
		_D("Launch camera");
		if (menu_daemon_open_app(CAMERA_PKG_NAME) < 0)
			_SECURE_E("Failed to launch %s", CAMERA_PKG_NAME);
	} else if (!strcmp(ev->keyname, KEY_HOME)) {
		_W("Home Key is pressed");
		if (key_info.long_press_timer) {
			ecore_timer_del(key_info.long_press_timer);
			key_info.long_press_timer = NULL;
		}

//		syspopup_destroy_all();
		key_info.cancel_key_timer = ecore_timer_add(CANCEL_KEY_TIMER_SEC, _destroy_syspopup_cb, NULL);

		if (vconf_get_int(VCONFKEY_SETAPPL_PSMODE , &val) < 0) {
			_E("Cannot get VCONFKEY");
		}
		if (val == SETTING_PSMODE_EMERGENCY) {
			key_info.homekey_count = 1;
			_D("Emergency mode, ignore home key..!!");
		} else {
			// Check homekey press count
			key_info.homekey_count++;
			_W("homekey count : %d", key_info.homekey_count);

			// Check homekey timer
			if(key_info.homekey_timer) {
				ecore_timer_del(key_info.homekey_timer);
				key_info.homekey_timer = NULL;
				_D("delete homekey timer");
			}
		}
		_D("create long press timer");
		key_info.long_press_timer = ecore_timer_add(LONG_PRESS_TIMER_SEC, _launch_taskmgr_cb, NULL);
		if (!key_info.long_press_timer)
			_E("Failed to add timer for long press detection");
	} else if (!strcmp(ev->keyname, KEY_CANCEL)) {
		_D("Cancel button is pressed");
		key_info.cancel = EINA_TRUE;
		key_info.is_cancel = EINA_TRUE;
		if (key_info.long_press_timer) {
			ecore_timer_del(key_info.long_press_timer);
			key_info.long_press_timer = NULL;
		}
	} else if (!strcmp(ev->keyname, KEY_MEDIA)) {
		_D("Media key is pressed");

		if (key_info.media_long_press) {
			ecore_timer_del(key_info.media_long_press);
			key_info.media_long_press = NULL;
		}

		key_info.media_long_press = ecore_timer_add(0.5, _launch_svoice_cb, NULL);
		if (!key_info.media_long_press)
			_E("Failed to add timer for long press detection");
	} else if (!strcmp(ev->keyname, KEY_APPS)) {
		_D("App tray key is pressed");
	} else if (!strcmp(ev->keyname, KEY_TASKSWITCH)) {
		_D("Task switch key is pressed");
	} else if (!strcmp(ev->keyname, KEY_WEBPAGE)) {
		_D("Web page key is pressed");
	} else if (!strcmp(ev->keyname, KEY_MAIL)) {
		_D("Mail key is pressed");
	} else if (!strcmp(ev->keyname, KEY_SEARCH)) {
		_D("Search key is pressed");
	} else if (!strcmp(ev->keyname, KEY_VOICE)) {
		_D("Voice key is pressed");
	} else if (!strcmp(ev->keyname, KEY_CONNECT)) {
		_D("Connect key is pressed");
	}

	return ECORE_CALLBACK_RENEW;
}



void _media_key_event_cb(media_key_e key, media_key_event_e status, void *user_data)
{
	_D("MEDIA KEY EVENT : %d", key);
	if (MEDIA_KEY_STATUS_PRESSED == status) return;

	if (MEDIA_KEY_PAUSE == key) {
		_release_multimedia_key("KEY_PAUSECD");
	} else if (MEDIA_KEY_PLAY == key) {
		_release_multimedia_key("KEY_PLAYCD");
	} else if (MEDIA_KEY_PLAYPAUSE == key) {
		_release_multimedia_key("KEY_PLAYPAUSECD");
	}
}


#define APP_ID_SPLIT_LAUNCHER "com.samsung.split-launcher"
#define APP_ID_MINIAPPS_LAUNCHER "com.samsung.mini-apps"
#define STR_ATOM_KEY_BACK_LONGPRESS "KEY_BACK_LONGPRESS"
#define STR_ATOM_KEY_MENU_LONGPRESS "KEY_MENU_LONGPRESS"
static Eina_Bool _client_message_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Client_Message *ev = event;
	Ecore_X_Atom safety_assurance_atom;
	Ecore_X_Atom atomMenuLongPress;
	Ecore_X_Atom atomBackLongPress;

	if (ev->format != 32)
		return ECORE_CALLBACK_RENEW;

	safety_assurance_atom = ecore_x_atom_get(STR_ATOM_XKEY_COMPOSITION);
	atomMenuLongPress = ecore_x_atom_get(STR_ATOM_KEY_MENU_LONGPRESS);
	atomBackLongPress = ecore_x_atom_get(STR_ATOM_KEY_BACK_LONGPRESS);
#if 0
	_D("_client_message_cb, safety_assurance_atom=[0x%x] atomMenuLongPress=[0x%x] atomBackLongPress=[0x%x]",
		safety_assurance_atom, atomMenuLongPress, atomBackLongPress);
#endif
	if ((ev->win == key_info.keyrouter_notiwindow) &&
			(ev->message_type == safety_assurance_atom) && (ev->format == 32)) {
#if 0 	/* Safety Assistance is changed to power key */
		int press = ev->data.l[2];
		int key_sum = ev->data.l[0]+ev->data.l[1];
		if (key_sum == 245) {
			_D("check key_sum[%d] to 122(volume_down)+123(volume_up), press=[%d]", key_sum, press);
			if (press) {
				if (key_info.client_msg_timer) {
					ecore_timer_del(key_info.client_msg_timer);
					key_info.client_msg_timer = NULL;
				}
				key_info.enable_safety_assurance = EINA_FALSE;
				key_info.client_msg_timer = ecore_timer_add(3, _client_msg_timer_cb, NULL);
				if (!key_info.client_msg_timer)
					_E("Failed to add timer for clent message");
			} else {
				if (key_info.client_msg_timer) {
					ecore_timer_del(key_info.client_msg_timer);
					key_info.client_msg_timer = NULL;
				}
#if 0
				if (key_info.enable_safety_assurance == EINA_TRUE) {
					_D("Launch SafetyAssurance");
					_launch_safety_assurance();
				}
				key_info.enable_safety_assurance = EINA_FALSE;
#endif
			}
		}
#endif
	} else if (ev->message_type == atomBackLongPress) {
		// Back key is long-pressed.
		_D("ev->message_type=[0x%x], Back key long press", ev->message_type);

		int val = 0;
		if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) {
			_D("Cannot get VCONFKEY");
		}

		if (VCONFKEY_IDLE_LOCK == val) {
			_D("Lock state, ignore back key long-press.");
			return ECORE_CALLBACK_RENEW;
		}

		if (vconf_get_int(VCONFKEY_PWLOCK_STATE, &val) < 0) {
			_E("Cannot get VCONFKEY_PWLOCK_STATE");
		}

		if (VCONFKEY_PWLOCK_BOOTING_LOCK == val || VCONFKEY_PWLOCK_RUNNING_LOCK == val) {
			_E("PW-lock state, ignore back key long-press.");
			return ECORE_CALLBACK_RENEW;
		}

		if (lockd_get_hall_status() == HALL_COVERED_STATUS) {
			_D("Scover is closed, ignore back ke long-press.");
			return ECORE_CALLBACK_RENEW;
		}

#ifdef SPLIT_LAUNCHER_ENABLE //ORG
		int multiwindow_enabled = 0;
		if (vconf_get_bool(VCONFKEY_QUICKSETTING_MULTIWINDOW_ENABLED, &multiwindow_enabled) < 0) {
			_E("Cannot get VCONFKEY");
			multiwindow_enabled = 0;
		}

		if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &val) < 0) {
			_D("Cannot get VCONFKEY");
			val = 0;
		}

		if ((val == 1) || (multiwindow_enabled == 0)) {
			_D("TTS : %d, Multiwindow enabled : %d", val, multiwindow_enabled);
			return ECORE_CALLBACK_RENEW;
		}

		_D("Launch the split-launcher");

		int ret = aul_launch_app(APP_ID_SPLIT_LAUNCHER, NULL);
		if (0 > ret) _E("cannot launch the split-launcher (%d)", ret);
#else //DCM 
		char *package = menu_daemon_get_selected_pkgname();
		if (!package) return ECORE_CALLBACK_RENEW;

		int apptray_enabled = !strcmp(package, CLUSTER_HOME_PKG_NAME);
		free(package);
		if (!apptray_enabled) return ECORE_CALLBACK_RENEW;

		_D("Launch the app-tray");

		bundle *b = bundle_create();
		retv_if(NULL == b, false);
		bundle_add(b, "LAUNCH", "ALL_APPS");
		int ret = aul_launch_app(APP_TRAY_PKG_NAME, b);
		if (0 > ret) _E("cannot launch the app-tray (%d)", ret);
		bundle_free(b);
#endif
	} else if (ev->message_type == atomMenuLongPress) {
		// Menu key is long-pressed.
		_D("ev->message_type=[0x%x], Menu key long press", ev->message_type);

		int val = 0;
		if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) {
			_E("Cannot get VCONFKEY_IDLE_LOCK_STATE");
		}

		if (VCONFKEY_IDLE_LOCK == val) {
			_E("Lock state, ignore menu key long-press.");
			return ECORE_CALLBACK_RENEW;
		}

		if (vconf_get_int(VCONFKEY_PWLOCK_STATE, &val) < 0) {
			_E("Cannot get VCONFKEY_PWLOCK_STATE");
		}

		if (VCONFKEY_PWLOCK_BOOTING_LOCK == val || VCONFKEY_PWLOCK_RUNNING_LOCK == val) {
			_E("PW-lock state, ignore menu key long-press.");
			return ECORE_CALLBACK_RENEW;
		}

		if (vconf_get_int(VCONFKEY_SETAPPL_PSMODE, &val) < 0) {
			_E("Cannot get VCONFKEY_SETAPPL_PSMODE");
		}

		if (SETTING_PSMODE_EMERGENCY == val) {
			_E("Emergency mode, ignore menu key long-press.");
			return ECORE_CALLBACK_RENEW;
		}

		if (lockd_get_hall_status() == HALL_COVERED_STATUS) {
			_D("Scover is closed, ignore menu key long-press.");
			return ECORE_CALLBACK_RENEW;
		}

		_D("Launch the mini-apps");
	    int ret_aul;
		bundle *b;
		b = bundle_create();
		retv_if(NULL == b, ECORE_CALLBACK_RENEW);
		bundle_add(b, "HIDE_LAUNCH", "0");
		ret_aul = aul_launch_app("com.samsung.mini-apps", b);
		if (0 > ret_aul) _E("cannot launch the mini apps (%d)", ret_aul);
		bundle_free(b);
	}
	return ECORE_CALLBACK_RENEW;
}


static bool gesture_is_availble_key(void)
{

	int ret = 0;
	int status = 0;


	ret = vconf_get_int(VCONFKEY_PWLOCK_STATE, &status);
	if (ret != 0)
	{
		_E("fail to get memory/pwlock/state%d", ret);
		return false;
	}


	if( status == VCONFKEY_PWLOCK_RUNNING_UNLOCK || status == VCONFKEY_PWLOCK_BOOTING_UNLOCK)
	{
		_D("enter the idle mode (%d)", status);
	}
	else
	{
		_D("don't enter the idle mode(%d)", status);
		return false;
	}

	_D("checking idle lock");

	ret = vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &status);
	if (ret != 0)
	{
		_E("fail to get memory/pwlock/state%d", ret);
		return false;
	}


	if( status == VCONFKEY_IDLE_LOCK)
	{
		_D("enter the lock mode(%d)", status);
		return false;
	}
	else
	{
		_D("unlock state");
	}

	return true;
}


static inline void launch_app(int x, int y)
{
	// e->cx, e->cy	
	bundle *param;
	int pid;
	int status = 0;
	int ret = 0;

	if(gesture_is_availble_key() == false)
	{
		_D("can't launch");
		return;
	}

	_D("checking status");
	ret = vconf_get_int(VCONFKEY_SETAPPL_PSMODE , &status);
	if (ret != 0) _D("fail to VCONFKEY_SETAPPL_PSMODE", ret);

	if( status == SETTING_PSMODE_EMERGENCY)
	{
		_D(" emergency on");
		return;
	}

	ret = vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS , &status);
	if (ret != 0) _D("fail to VCONFKEY_SETAPPL_ACCESSIBILITY_TTS", ret);

	if( status == 1)
	{
		_D(" tts on");
		return;
	}

	ret = vconf_get_bool(_VCONF_QUICK_COMMADN_NAME, &status);
	if (ret != 0) _E("fail to get db/aircommand/enabled:%d", ret);

	if( status == 0)
	{
		_D("can't launch because of off");
		return;
	}

	ret = vconf_get_bool("db/aircommand/floating", &status);
	if (ret != 0) _E("fail to get db/aircommand/floating:%d", ret);

	if( status == 1)
	{
		_D("only floating");
		return;
	}

	param = bundle_create();
	if (param) {
		char coord[16];
		snprintf(coord, sizeof(coord), "%dx%d", x, y);
		bundle_add(param, "coordinate", coord);
	}

	feedback_play_type(FEEDBACK_TYPE_SOUND, FEEDBACK_PATTERN_TOUCH_TAP);

	pid = aul_launch_app("com.samsung.air-command", param);
	_D("Launch Pie Menu: %d\n", pid);

	if (param) {
		bundle_free(param);
	}
}



#if 0
static int gesture_hold_cb(void *data, int ev_type, void *ev)
{
	Ecore_X_Event_Gesture_Notify_Hold *e = ev;

	if (e->num_fingers != 2) {
		return 1;
	}

	switch (e->subtype) {
	case ECORE_X_GESTURE_BEGIN:
		_D("Begin: hold[%d]\n", e->hold_time);
		launch_app(e->cx, e->cy);
		break;
	case ECORE_X_GESTURE_UPDATE:
		_D("Update: hold[%d]\n", e->hold_time);
		break;
	case ECORE_X_GESTURE_END:
		_D("End: hold[%d]\n", e->hold_time);
		break;
	default:
		break;
	}

	return 1;
}
#endif



static int gesture_cb(void *data, int ev_type, void *ev)
{
	Ecore_X_Event_Gesture_Notify_Tap *e = ev;

	_D("key");

	if (e->tap_repeat != 2) {
		return 1;
	}

	if (e->num_fingers == 2) {
		launch_app(e->cx, e->cy);
	}

	return 1;
}



static int gesture_flick_cb(void *data, int ev_type, void *ev)
{
	Ecore_X_Event_Gesture_Notify_Flick *e = ev;

	_D(" input bezel");

	if(e->num_fingers == 1)
	{
		launch_app(0, 0);
	}

	return 1;
}



static void gesture_key_init(void)
{
	int status;

	_D("Init gesture for quick command");

	if( gbRegisterDuobleTap > 1)
	{
		_E("Already registered callback cnt[%d]", gbRegisterDuobleTap);
		return;
	}

	gbRegisterDuobleTap++;

	status = ecore_x_gesture_event_grab(key_info.win, ECORE_X_GESTURE_EVENT_TAP, 2);
	if (!status) {
		_E("%d\n", status);
		return;
	}
#if _DEF_BEZEL_AIR_COMMAND
	status = ecore_x_gesture_event_grab(key_info.win, ECORE_X_GESTURE_EVENT_FLICK, 1);
	if (!status) {
		_E("%d\n", status);
		return;
	}
#endif
/*
	status = ecore_x_gesture_event_grab(key_info.win, ECORE_X_GESTURE_EVENT_HOLD, 2);
	if (!status) {
		_E("%d\n", status);
		return;
	}
*/
	key_info.two_finger_double_tap = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_TAP, (Ecore_Event_Handler_Cb)gesture_cb, NULL);
	if (!key_info.two_finger_double_tap) {
		_E("Failed to add handler\n");
	}
#if _DEF_BEZEL_AIR_COMMAND
	key_info.bezel_gesture = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_FLICK, (Ecore_Event_Handler_Cb)gesture_flick_cb, NULL);
	if (!key_info.bezel_gesture) {
		_E("Failed to add handle about bezel_gesture\n");
	}
#endif
/*
	key_info.two_finger_long_tap = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_HOLD, (Ecore_Event_Handler_Cb)gesture_hold_cb, NULL);
	if (!key_info.two_finger_long_tap) {
		_E("Failed to add handler\n");
	}
*/
//	feedback_initialize();
}

static void gesture_key_fini(void)
{
	int status;

	_D("fini gesture for quick command");

	gbRegisterDuobleTap--;

	if( gbRegisterDuobleTap < 0 )
	{
		_D("register value can't be decreased");
		gbRegisterDuobleTap = 0;
	}

	if (key_info.two_finger_double_tap) {
		ecore_event_handler_del(key_info.two_finger_double_tap);
		key_info.two_finger_double_tap = NULL;
	}

	if (key_info.two_finger_long_tap) {
		ecore_event_handler_del(key_info.two_finger_long_tap);
		key_info.two_finger_long_tap = NULL;
	}

#if _DEF_BEZEL_AIR_COMMAND
	if (key_info.bezel_gesture) {
		ecore_event_handler_del(key_info.bezel_gesture);
		key_info.bezel_gesture = NULL;
	}
#endif

	status = ecore_x_gesture_event_ungrab(key_info.win, ECORE_X_GESTURE_EVENT_TAP, 2);
	if (!status) {
		_E("%d\n", status);
	}

	status = ecore_x_gesture_event_ungrab(key_info.win, ECORE_X_GESTURE_EVENT_HOLD, 2);
	if (!status) {
		_E("%d\n", status);
	}
#if _DEF_BEZEL_AIR_COMMAND
	status = ecore_x_gesture_event_ungrab(key_info.win, ECORE_X_GESTURE_EVENT_FLICK, 1);
	if (!status) {
		_E("%d\n", status);
	}
#endif
//	feedback_deinitialize();
}

static void gesture_key_vconf_cb(keynode_t *key, void* pUserData)
{
	char *keynode_name = vconf_keynode_get_name(key);
	ret_if(!keynode_name);

	if(!memcmp(keynode_name, _VCONF_QUICK_COMMADN_NAME, strlen(_VCONF_QUICK_COMMADN_NAME)))
	{
		if(vconf_keynode_get_int(key))
		{
			int ret = 0;
			int status = 0;

			ret = vconf_get_bool(_VCONF_QUICK_COMMADN_NAME, &status);
			if (ret != 0) _E("fail to get db/aircommand/enabled:%d", ret);

			if( status == 1)
			{
//				_D(" quick command on");
//				gesture_key_init();

				int nRet = 0;
				int nStatus = 0;

				_D(" quick command on");

				nRet = vconf_get_bool("db/aircommand/floating", &nStatus);
				if (nRet != 0) _E("fail to get db/aircommand/floating:%d", nRet);

				if( nStatus == 1)
				{
					int pid = aul_launch_app("com.samsung.air-command", NULL);
					_D("run a floating window after booting(%d)", pid);
//					gesture_key_fini();
				}
				else
				{
					_D(" quick command double tap");
					gesture_key_init();
				}

			}
			else
			{
				_D(" quick command off");

				gesture_key_fini();
			}
		}
	}
}

static Eina_Bool gesture_wating_launch_cb(void *data)
{
	int	pid = -1;

	pid = aul_launch_app("com.samsung.air-command", NULL);
	_D("launch callback (%d)", pid);

	return ECORE_CALLBACK_CANCEL;
}

static void gesture_floating_vconf_cb(keynode_t *key, void* pUserData)
{
	char *keynode_name = vconf_keynode_get_name(key);
	ret_if(!keynode_name);

	if(!memcmp(keynode_name, _VCONF_QUICK_COMMADN_FLOATING, strlen(_VCONF_QUICK_COMMADN_FLOATING)))
	{
		if(vconf_keynode_get_int(key))
		{
			int ret = 0;
			int status = 0;

			_D("changed floating mode");

			ret = vconf_get_bool(_VCONF_QUICK_COMMADN_NAME, &status);
			if (ret != 0) _E("fail to get db/aircommand/enabled:%d", ret);

			if( status == 1)
			{
				int nRet = 0;
				int nStatus = 0;

				_D(" quick command on");

				nRet = vconf_get_bool(_VCONF_QUICK_COMMADN_FLOATING, &nStatus);
				if (nRet != 0) _E("fail to get db/aircommand/floating:%d", nRet);

				if( nStatus == 1)
				{

					if(gtimer_launch)
					{
						ecore_timer_del(gtimer_launch);
						gtimer_launch = NULL;
					}

					gtimer_launch = ecore_timer_add(0.100f, gesture_wating_launch_cb, NULL);



/*
					service_h hService;

					_D(" quick floating window: on");

					service_create(&hService);
					service_set_package(hService, "com.samsung.air-command");

					if (service_send_launch_request(hService, NULL, this) != SERVICE_ERROR_NONE)
					{
						DLOG_QCOMMAND_ERR("Failed to com.samsung.air-command");
					}
					else
					{
						DLOG_QCOMMAND_DBG("Success to com.samsung.air-command");
					}

					service_destroy(hService);
*/
				}
				else
				{
					_D(" quick command double tap");
				}

			}
			else
			{
				_D(" already quick command off status");
			}
	
		}
	}
}


static void gesture_enter_idle_initialize(void)
{
	int ret = 0;
	int status = 0;

	_D("Idle enter checking...");

	ret = vconf_get_bool(_VCONF_QUICK_COMMADN_NAME, &status);
	if (ret != 0) _E("fail to get db/aircommand/enabled:%d", ret);

	if( status == 0)
	{
		_D("disabled qujick command");
		return;
	}
	
	status = 0;
	ret = vconf_get_int(VCONFKEY_PWLOCK_STATE, &status);
	if (ret != 0)
	{
		_E("fail to get db/aircommand/enabled:%d", ret);
		return;
	}

	if( status == VCONFKEY_PWLOCK_RUNNING_UNLOCK || status == VCONFKEY_PWLOCK_BOOTING_UNLOCK)
	{
		gbenter_idle = true;
		status = 0;

		/* If it has a floating window, run it*/
		ret = vconf_get_bool(_VCONF_QUICK_COMMADN_FLOATING, &status);
		if (ret != 0) _E("fail to get db/aircommand/floating:%d", ret);

		if( status == 1)
		{
			if( gbinit_floatwin == false)
			{
				int pid = aul_launch_app("com.samsung.air-command", NULL);
				_D("run a floating window after booting(%d)", pid);

				gbinit_floatwin = true;
			}
		}
		else
		{
			if( gbinit_floatwin == false)
			{
				_D("register touch grap");

				gesture_key_init();

				gbinit_floatwin = true;
			}

		}
	}
	else
	{
		_D("already no idle state(%d)", status);
	}
}

static void gesture_enter_idle_vconf_cb(keynode_t *key, void* pUserData)
{
	char *keynode_name = vconf_keynode_get_name(key);
	ret_if(!keynode_name);

	if(!memcmp(keynode_name, VCONFKEY_PWLOCK_STATE, strlen(VCONFKEY_PWLOCK_STATE)))
	{
		if(vconf_keynode_get_int(key))
		{
			_D("received cb");
			gesture_enter_idle_initialize();
		}

	}

}


static void gesture_key_register(void)
{
	int ret;
	char buf[1024] = { 0, };

	ret = vconf_notify_key_changed(_VCONF_QUICK_COMMADN_NAME,	(vconf_callback_fn)gesture_key_vconf_cb, NULL);
	if (ret < 0){

		_E("Error: Failed to sigaction[%s]", strerror_r(errno, buf, sizeof(buf)));
	}

	ret = vconf_notify_key_changed(_VCONF_QUICK_COMMADN_FLOATING,	(vconf_callback_fn)gesture_floating_vconf_cb, NULL);
	if (ret < 0){

		_E("Error: Failed to sigaction[%s]", strerror_r(errno, buf, sizeof(buf)));
	}

	ret = vconf_notify_key_changed(VCONFKEY_PWLOCK_STATE,	(vconf_callback_fn)gesture_enter_idle_vconf_cb, NULL);
	if (ret < 0){

		_E("Error: Failed to sigaction[%s]", strerror_r(errno, buf, sizeof(buf)));
	}

	gesture_enter_idle_initialize();
}

#define PROP_HWKEY_EMULATION "_HWKEY_EMULATION"
static void _set_long_press_time(void)
{
    Ecore_X_Window *keyrouter_input_window;
    Ecore_X_Atom atom_keyrouter = ecore_x_atom_get(PROP_HWKEY_EMULATION);

    int num = 0;
    if (EINA_FALSE == ecore_x_window_prop_property_get(ecore_x_window_root_first_get(),
            atom_keyrouter,
            ECORE_X_ATOM_WINDOW,
            32,
            (unsigned char **) &keyrouter_input_window,
            &num))
    {
        _E("Cannot get the property");
		return;
    }

    int longpress_timeout = 500; // miliseconds
    int menu_keycode = ecore_x_keysym_keycode_get(KEY_MENU);
    ecore_x_client_message32_send(*keyrouter_input_window,
            atom_keyrouter,
            ECORE_X_EVENT_MASK_NONE,
            key_info.win,
            menu_keycode,
            longpress_timeout,
            0,
            0);

    int back_keycode = ecore_x_keysym_keycode_get(KEY_BACK);
    ecore_x_client_message32_send(*keyrouter_input_window,
            atom_keyrouter,
            ECORE_X_EVENT_MASK_NONE,
            key_info.win,
            back_keycode,
            longpress_timeout,
            0,
            0);
}

void create_key_window(void)
{
	int ret;
	Ecore_X_Atom atomNotiWindow;
	Ecore_X_Window keyrouter_notiwindow;

	key_info.win = ecore_x_window_input_new(0, 0, 0, 1, 1);
	if (!key_info.win) {
		_D("Failed to create hidden window");
		return;
	}
	ecore_x_event_mask_unset(key_info.win, ECORE_X_EVENT_MASK_NONE);
	/*
	ecore_x_event_mask_unset(key_info.win, ECORE_X_EVENT_MASK_MOUSE_DOWN);
	ecore_x_event_mask_unset(key_info.win, ECORE_X_EVENT_MASK_MOUSE_UP);
	ecore_x_event_mask_unset(key_info.win, ECORE_X_EVENT_MASK_MOUSE_IN);
	ecore_x_event_mask_unset(key_info.win, ECORE_X_EVENT_MASK_MOUSE_OUT);
	ecore_x_event_mask_unset(key_info.win, ECORE_X_EVENT_MASK_MOUSE_MOVE);
	*/

	ecore_x_icccm_title_set(key_info.win, "menudaemon,key,receiver");
	ecore_x_netwm_name_set(key_info.win, "menudaemon,key,receiver");
	ecore_x_netwm_pid_set(key_info.win, getpid());

	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_HOME, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_VOLUMEDOWN, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_VOLUMEUP, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_CONFIG, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_MEDIA, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_APPS, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_TASKSWITCH, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_WEBPAGE, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_MAIL, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_SEARCH, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_VOICE, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_CONNECT, SHARED_GRAB);
	ret = utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_POWER, SHARED_GRAB);
	if (ret != 0) {
		_E("utilx_grab_key KEY_POWER GrabSHARED_GRAB failed, ret[%d]", ret);
	}

	key_info.key_up = ecore_event_handler_add(ECORE_EVENT_KEY_UP, _key_release_cb, NULL);
	if (!key_info.key_up)
		_E("Failed to register a key up event handler");

	key_info.key_down = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _key_press_cb, NULL);
	if (!key_info.key_down)
		_E("Failed to register a key down event handler");

	/* Get notifwindow */
	atomNotiWindow = ecore_x_atom_get(STR_ATOM_KEYROUTER_NOTIWINDOW);
	ret = ecore_x_window_prop_window_get(ecore_x_window_root_first_get(), atomNotiWindow, &keyrouter_notiwindow, 1);
	if (ret > 0) {
		_D("Succeed to get keyrouter notiwindow ! ret = %d (win=0x%x)\n", ret, keyrouter_notiwindow);
		/* mask set for  keyrouter notiwindow */
		ecore_x_window_sniff(keyrouter_notiwindow);
		key_info.keyrouter_notiwindow = keyrouter_notiwindow;
		//xkey_composition = ecore_x_atom_get(STR_ATOM_XKEY_COMPOSITION);
		/* Register client msg */
		key_info.client_msg_hd = ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE,
					_client_message_cb, NULL);
		if (!key_info.client_msg_hd)
			_E("failed to add handler(ECORE_X_EVENT_CLIENT_MESSAGE)");
	} else {
		_E("Failed to get keyrouter notiwindow!! ret = %d, atomNotiWindow = 0x%x, keyrouter_notiwindow = 0x%x", ret, atomNotiWindow, keyrouter_notiwindow);
	}

	media_key_reserve(_media_key_event_cb, NULL);
	//gesture_key_register();
	_set_long_press_time();
//	gesture_key_init();
}



void destroy_key_window(void)
{
	gesture_key_fini();
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_HOME);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_VOLUMEDOWN);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_VOLUMEUP);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_CONFIG);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_MEDIA);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_APPS);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_TASKSWITCH);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_WEBPAGE);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_MAIL);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_SEARCH);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_VOICE);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_CONNECT);

	if (key_info.key_up) {
		ecore_event_handler_del(key_info.key_up);
		key_info.key_up = NULL;
	}

	if (key_info.key_down) {
		ecore_event_handler_del(key_info.key_down);
		key_info.key_down = NULL;
	}

	ecore_x_window_delete_request_send(key_info.win);
	key_info.win = 0x0;

	media_key_release();
}



// End of a file
