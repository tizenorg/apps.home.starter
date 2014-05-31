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
#include <bundle.h>
#include <Elementary.h>
#include <Ecore_X.h>
#include <Ecore_Input.h>
#include <utilX.h>
#include <vconf.h>

#include <syspopup_caller.h>
#include <dd-display.h>
#include <E_DBus.h>
#include <feedback.h>
#include <pkgmgr-info.h>
#include <system/media_key.h>

#include "starter.h"
#include "hw_key.h"
#include "util.h"

#define GRAB_TWO_FINGERS 2
#define POWERKEY_TIMER_SEC 0.25
#define POWERKEY_LCDOFF_TIMER_SEC 0.4
#define LONG_PRESS_TIMER_SEC 0.7
#define SYSPOPUP_END_TIMER_SEC 0.5

#define SERVICE_OPERATION_MAIN_KEY "__APP_SVC_OP_TYPE__"
#define SERVICE_OPERATION_MAIN_VALUE "http://tizen.org/appcontrol/operation/main"
#define SERVICE_OPERATION_DEFAULT_VALUE "http://tizen.org/appcontrol/operation/default"
#define HOME_OPERATION_KEY "home_op"
#define POWERKEY_VALUE "powerkey"

#define USE_DBUS_POWEROFF 1

#define POWEROFF_BUS_NAME       "org.tizen.system.popup"
#define POWEROFF_OBJECT_PATH    "/Org/Tizen/System/Popup/Poweroff"
#define POWEROFF_INTERFACE_NAME POWEROFF_BUS_NAME".Poweroff"
#define METHOD_POWEROFF_NAME	"PopupLaunch"
#define DBUS_REPLY_TIMEOUT (120 * 1000)

#define DOUBLE_PRESS_NONE "none"
#define DOUBLE_PRESS_RECENT_APPS "recent"
#define W_TASKMGR_PKGNAME "org.tizen.w-taskmanager"
#define W_CONTROLS_PKGNAME "org.tizen.windicator"

#define MUSIC_PLAYER_PKG_NAME "org.tizen.w-music-player"

static struct {
	Ecore_X_Window win;
	Ecore_Event_Handler *key_up;
	Ecore_Event_Handler *key_down;
	Ecore_Event_Handler *two_fingers_hold_hd;
	Ecore_Timer *long_press_timer;
	Ecore_Timer *powerkey_timer;
	Eina_Bool is_lcd_on;
	Eina_Bool is_long_press;
	int powerkey_count;
	Eina_Bool is_cancel;
} key_info = {
	.win = 0x0,
	.key_up = NULL,
	.key_down = NULL,
	.two_fingers_hold_hd = NULL,
	.long_press_timer = NULL,
	.powerkey_timer = NULL,
	.is_lcd_on = EINA_FALSE,
	.is_long_press = EINA_FALSE,
	.powerkey_count = 0,
	.is_cancel = EINA_FALSE,
};

#if 0 // This is not W code
/* NOTE: THIS FUNCTION Is ONLY USED FOR CONFIDENTIAL FEATURE. REMOVE ME */
static inline int _launch_running_apps_FOR_TEMPORARY(void)
{
	bundle *kb = NULL;
	char *package;
	int ret;

	package = menu_daemon_get_selected_pkgname();
	if (!package)
		return -ENOENT;

	if (!strcmp(package, MENU_SCREEN_PKG_NAME)) {
		free(package);
		return -EINVAL;
	}

	free(package);

	kb = bundle_create();
	if (!kb) {
		_E("Failed to create a bundle");
		return -EFAULT;
	}

	bundle_add(kb, "LONG_PRESS", "1");
	ret = menu_daemon_launch_app(APP_TRAY_PKG_NAME, kb);
	bundle_free(kb);

	if (ret < 0) {
		_E("Failed to launch the running apps, ret : %d", ret);
		return -EFAULT;
	} else if (ret > 0) {
		if (-1 == deviced_conf_set_mempolicy_bypid(ret, OOM_IGNORE)) {
			_E("Cannot set the memory policy for App tray(%d)", ret);
		} else {
			_E("Set the memory policy for App tray(%d)", ret);
		}
	}

	return 0;
}



#define DESKDOCK_PKG_NAME "org.tizen.desk-dock"
static Eina_Bool _launch_by_home_key(void *data)
{
	int lock_state = (int) data;
	_D("lock_state : %d ", lock_state);

	key_info.single_timer = NULL;

	if (lock_state == VCONFKEY_IDLE_LOCK) {
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

	menu_daemon_open_homescreen(NULL);

	return ECORE_CALLBACK_CANCEL;
}



inline static Eina_Bool _launch_svoice(void)
{
	const char *pkg_name = NULL;
	bundle *b;

	pkg_name = menu_daemon_get_svoice_pkg_name();
	retv_if(NULL == pkg_name, EINA_FALSE);

	if (!strcmp(pkg_name, SVOICE_PKG_NAME)) {
		int val = -1;

		if (vconf_get_int(VCONFKEY_SVOICE_OPEN_VIA_HOME_KEY, &val) < 0) {
			_D("Cannot get VCONFKEY");
		}

		if (val != 1) {
			_D("Launch nothing");
			return EINA_FALSE;
		}
	}

	b = bundle_create();
	retv_if(!b, EINA_FALSE);

	bundle_add(b, SVOICE_LAUNCH_BUNDLE_KEY, SVOICE_LAUNCH_BUNDLE_HOMEKEY_VALUE);
	if (menu_daemon_launch_app(pkg_name, b) < 0)
		_SECURE_E("Failed to launch %s", pkg_name);
	bundle_free(b);

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
	if (vconf_get_int(VCONFKEY_SVOICE_OPEN_VIA_EARPHONE_KEY, &val) < 0) {
		_D("Cannot get VCONFKEY");
	}
	if (1 == val) {
		_D("Launch SVOICE");

		bundle *b;
		b = bundle_create();
		retv_if(!b, ECORE_CALLBACK_CANCEL);

		bundle_add(b, SVOICE_LAUNCH_BUNDLE_KEY, SVOICE_LAUNCH_BUNDLE_VALUE);
		if (menu_daemon_launch_app(SVOICE_PKG_NAME, b) < 0)
			_SECURE_E("Failed to launch %s", TASKMGR_PKG_NAME);
		bundle_free(b);
	}

	return ECORE_CALLBACK_CANCEL;
}


static Eina_Bool _client_msg_timer_cb(void* data)
{
	_D("_client_msg_timer_cb, safety assurance is enable");

	key_info.enable_safety_assurance = EINA_TRUE;
	_D("Launch SafetyAssurance");
	_launch_safety_assurance();
	key_info.client_msg_timer = NULL;

	return ECORE_CALLBACK_CANCEL;
}


static Eina_Bool _set_unlock(void *data)
{
	_D("_set_unlock");
	vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_UNLOCK);
	return ECORE_CALLBACK_CANCEL;
}


inline static int _release_home_key(int lock_state)
{
	retv_if(NULL == key_info.long_press, EXIT_SUCCESS);
	ecore_timer_del(key_info.long_press);
	key_info.long_press = NULL;

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
	ret = menu_daemon_launch_app(MUSIC_PLAYER_PKG_NAME, b);
	if (ret < 0)
		_E("Failed to launch the running apps, ret : %d", ret);

	bundle_free(b);
}


static Eina_Bool _client_message_cb(void *data, int type, void *event)
{
	int key_sum;
	int press;
	Ecore_X_Event_Client_Message *ev = event;
	Ecore_X_Atom safety_assurance_atom;

	if (ev->format != 32)
		return ECORE_CALLBACK_RENEW;

	safety_assurance_atom = ecore_x_atom_get(ATOM_KEY_COMPOSITION);
	if (ev->message_type == safety_assurance_atom) {
		press = ev->data.l[2];
		key_sum = ev->data.l[0]+ev->data.l[1];
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
	}
	return ECORE_CALLBACK_RENEW;
}

#endif // This is not W code



static int _append_variant(DBusMessageIter *iter, const char *sig, char *param[])
{
	char *ch;
	int i;
	int int_type;
	uint64_t int64_type;

	if (!sig || !param)
		return 0;

	for (ch = (char*)sig, i = 0; *ch != '\0'; ++i, ++ch) {
		switch (*ch) {
		case 'i':
			int_type = atoi(param[i]);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &int_type);
			break;
		case 'u':
			int_type = atoi(param[i]);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &int_type);
			break;
		case 't':
			int64_type = atoi(param[i]);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &int64_type);
			break;
		case 's':
			dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param[i]);
			break;
		default:
			return -EINVAL;
		}
	}

	return 0;
}


DBusMessage *_invoke_dbus_method_sync(const char *dest, const char *path,
		const char *interface, const char *method,
		const char *sig, char *param[])
{
	DBusConnection *conn;
	DBusMessage *msg;
	DBusMessageIter iter;
	DBusMessage *reply;
	DBusError err;
	int r;

	conn = dbus_bus_get(DBUS_BUS_SYSTEM, NULL);
	if (!conn) {
		_E("dbus_bus_get error");
		return NULL;
	}

	msg = dbus_message_new_method_call(dest, path, interface, method);
	if (!msg) {
		_E("dbus_message_new_method_call(%s:%s-%s)", path, interface, method);
		return NULL;
	}

	dbus_message_iter_init_append(msg, &iter);
	r = _append_variant(&iter, sig, param);
	if (r < 0) {
		_E("append_variant error(%d)", r);
		return NULL;
	}

#if 0 //Temp block sync call from power off popup.
	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_REPLY_TIMEOUT, &err);

	if (!reply) {
		_E("dbus_connection_send error(No reply)");
	}

	if (dbus_error_is_set(&err)) {
		_E("dbus_connection_send error(%s:%s)", err.name, err.message);
		reply = NULL;
	}

	dbus_message_unref(msg);
	dbus_error_free(&err);

	return reply;
#else //Temp async call
	r = dbus_connection_send(conn, msg, NULL);
	dbus_message_unref(msg);
	if (r != TRUE) {
		_E("dbus_connection_send error(%s:%s:%s-%s)", dest, path, interface, method);
		return -ECOMM;
	}
	_D("dbus_connection_send, ret=%d", r);
	return NULL;
#endif
}


static int _request_Poweroff(void)
{
	DBusError err;
	DBusMessage *msg;
	int ret, ret_val;

	msg = _invoke_dbus_method_sync(POWEROFF_BUS_NAME, POWEROFF_OBJECT_PATH, POWEROFF_INTERFACE_NAME,
			METHOD_POWEROFF_NAME, NULL, NULL);
	if (!msg)
		return -EBADMSG;

	dbus_error_init(&err);

	ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &ret_val, DBUS_TYPE_INVALID);
	if (!ret) {
		_E("no message : [%s:%s]", err.name, err.message);
		ret_val = -EBADMSG;
	}

	dbus_message_unref(msg);
	dbus_error_free(&err);

	_D("%s-%s : %d", POWEROFF_INTERFACE_NAME, METHOD_POWEROFF_NAME, ret_val);
	return ret_val;
}

char *_get_app_type(const char *pkgname)
{
	int ret = 0;
	char *apptype = NULL;
	char *re_apptype = NULL;
	pkgmgrinfo_pkginfo_h handle;
	ret = pkgmgrinfo_pkginfo_get_pkginfo(pkgname, &handle);
	if (ret != PMINFO_R_OK)
		return NULL;
	ret = pkgmgrinfo_pkginfo_get_type(handle, &apptype);
	if (ret != PMINFO_R_OK) {
		pkgmgrinfo_pkginfo_destroy_pkginfo(handle);
		return NULL;
	}

	/*after call pkgmgrinfo_appinfo_destroy_appinfo, mainappid is destroyed with handle, so must copy it*/
	re_apptype = strdup(apptype);
	_SECURE_D("apptype : %s - %s", apptype, re_apptype);
	pkgmgrinfo_pkginfo_destroy_pkginfo(handle);

	return re_apptype;
}

#define W_LAUNCHER_PKGNAME "org.tizen.w-launcher-app"
static Eina_Bool _launch_home_by_powerkey(void)
{
	bundle *b = NULL;
	int pid = 0;

	_D("%s", __func__);

	b = bundle_create();
	if(!b) {
		_E("Failed to create bundle");
		return EINA_FALSE;
	}
	bundle_add(b, HOME_OPERATION_KEY, POWERKEY_VALUE);

	pid = w_launch_app(W_LAUNCHER_PKGNAME, b);
	_SECURE_D("launch[%s], pid[%d]", W_LAUNCHER_PKGNAME, pid);

	if(pid < AUL_R_OK) {
		if(b) {
			bundle_free(b);
		}
		return EINA_FALSE;
	}

	if(b) {
		bundle_free(b);
	}
	return EINA_TRUE;
}


static Eina_Bool _syspopup_end_timer_cb(void *data)
{
	/* terminate syspopup */
	syspopup_destroy_all();
	return ECORE_CALLBACK_CANCEL;
}


#define APP_TYPE_WIDGET "wgt"
#define W_CAMERA_PKGNAME "org.tizen.w-camera-app"
static Eina_Bool _launch_app_by_double_press(void)
{
	char *appid = NULL;
	char *pkgname = NULL;
	char *pkgtype = NULL;
	bundle *b = NULL;
	int pid = 0;
	int ret = 0;
	int ret_aul = 0;

	_D("%s", __func__);


	appid = vconf_get_str(VCONFKEY_WMS_POWERKEY_DOUBLE_PRESSING);

	if (appid == NULL) {
		_E("appid is NULL");
	} else if (!strcmp(appid, DOUBLE_PRESS_NONE)) {
		_D("none : DOUBLE_PRESS_NONE !!");
		return EINA_TRUE;
	} else if (!strcmp(appid, DOUBLE_PRESS_RECENT_APPS)) {
		_D("recent : launch task mgr..!!");
		ret = w_launch_app(W_TASKMGR_PKGNAME, NULL);
		if (ret >= 0) {
			_SECURE_D("[%s] is launched, pid=[%d]", W_TASKMGR_PKGNAME, ret);
		}
		return EINA_TRUE;
	} else {
		char *last;
		char *temp;
		char *new_appid;
		last = strrchr(appid, '/');
		if (last == NULL || *(last + 1) == NULL) {
			_E("Invaild data");
		} else {
			_D("appid has class name");
			new_appid = strdup(last + 1);
			if(new_appid == NULL) {
				_E("appid is NULL");
			}
			temp = strtok(appid, "/");
			if(temp == NULL){
				_E("Invalid data");
			}
			else{
				pkgname = strdup(temp);
			}
		}
		free(appid);
		appid = new_appid;
	}

	if(appid == NULL) {
		_E("appid is NULL. set default to none.");
		return EINA_FALSE;
	}

	pkgtype = _get_app_type(pkgname);
	if(pkgtype == NULL && appid != NULL) {
		_E("Failed to get app_type. app_type is NULL");
		if(appid){
			free(appid);
		}
		if(pkgname){
			free(pkgname);
		}
		return EINA_FALSE;
	}

	_SECURE_D("appid : %s, pkgname : %s, pkgtype : %s", appid, pkgname, pkgtype);

	if(!strcmp(pkgtype, APP_TYPE_WIDGET)){
		ret_aul = aul_open_app(appid);
		if(ret_aul < AUL_R_OK) {
			_D("Launching app ret : [%d]", ret_aul);
			free(appid);
			free(pkgname);
			free(pkgtype);
			return EINA_FALSE;
		}
		if(appid) {
			free(appid);
		}
	}
	else{

		b = bundle_create();
		if(!b) {
			_E("Failed to create bundle");
			if(appid) {
				free(appid);
			}
			return EINA_FALSE;
		}
		bundle_add(b, SERVICE_OPERATION_MAIN_KEY, SERVICE_OPERATION_MAIN_VALUE);

		pid = w_launch_app(appid, b);
		_SECURE_D("launch[%s], pid[%d]", appid, pid);

		if(pid < AUL_R_OK) {
			if(b) {
				bundle_free(b);
			}
			if(appid) {
				free(appid);
			}
			return EINA_FALSE;
		}

#if 0
		if (strcmp(appid, W_CONTROLS_PKGNAME)) {
			/* terminate syspopup */
			//syspopup_destroy_all();
			ecore_timer_add(SYSPOPUP_END_TIMER_SEC, _syspopup_end_timer_cb, NULL);
		}
#endif
		if(b) {
			bundle_free(b);
		}
		if(appid) {
			free(appid);
		}
	}
	if(pkgname){
		free(pkgname);
	}
	if(pkgtype){
		free(pkgtype);
	}
	return EINA_TRUE;
}


static Eina_Bool _powerkey_timer_cb(void *data)
{
	int val = -1;
	int trigger_val = -1;

	_W("%s, powerkey count[%d]", __func__, key_info.powerkey_count);
#if 0
	if(key_info.long_press_timer) {
		ecore_timer_del(key_info.long_press_timer);
		key_info.long_press_timer = NULL;
		_D("delete long_press_timer");
	}

	if(key_info.powerkey_timer) {
		ecore_timer_del(key_info.powerkey_timer);
		key_info.powerkey_timer = NULL;
		_D("delete powerkey_timer");
	}
#endif

	key_info.powerkey_timer = NULL;

	/* setdup_wizard is running : should not turn off LCD*/
	if(vconf_get_int(VCONFKEY_SETUP_WIZARD_STATE, &val) < 0) {
		_SECURE_E("Failed to get vconfkey[%s]", VCONFKEY_SETUP_WIZARD_STATE);
		val = -1;
	}
	if(val == VCONFKEY_SETUP_WIZARD_LOCK) {
		_E("setdup_wizard is running");
		key_info.powerkey_count = 0;
		return ECORE_CALLBACK_CANCEL;
	}

	/* Check critical low batt clock mode */
	if(vconf_get_int(VCONFKEY_PM_KEY_IGNORE, &val) < 0) {
		_SECURE_E("Failed to get vconfkey[%s]", VCONFKEY_PM_KEY_IGNORE);
		val = -1;
	}
	if(val == 1) { //Critical Low Batt Clock Mode
		_E("Critical Low Batt Clock Mode");
		key_info.powerkey_count = 0; //initialize powerkey count
		if(!key_info.is_lcd_on) {
			_W("just turn on LCD by powerkey.. starter ignore powerkey operation");
		} else {
			_W("just turn off LCD");
			display_change_state(LCD_OFF);
		}
		return ECORE_CALLBACK_CANCEL;
	}


	/* Safety Assitance */
	if(vconf_get_int(VCONFKEY_WMS_SAFETY_ENABLE, &val) < 0) {
		_SECURE_E("Failed to get vconfkey[%s]", VCONFKEY_WMS_SAFETY_ENABLE);
		val = -1;
	}
	if(val == 1) { //Safety Assistance is ON
		if(key_info.powerkey_count == 2) {
			/* double press */
			_W("powerkey double press");
			key_info.powerkey_count = 0;
			if(!_launch_app_by_double_press()) {
				_E("Failed to launch by double press");
			}
			return ECORE_CALLBACK_CANCEL;
		} else if(key_info.powerkey_count >= 3) {
			_E("Safety Assistance : safety is enabled");
			key_info.powerkey_count = 0;
			if(vconf_get_int(VCONFKEY_WMS_SAFETY_MESSAGE_TRIGGER, &trigger_val) < 0) {
				_SECURE_E("Failed to get vconfkey[%s]", VCONFKEY_WMS_SAFETY_MESSAGE_TRIGGER);
			}
			_E("Safety Assistance trigger status : [%d]", trigger_val);
			if(trigger_val == 0) {
				//set wms trigger
				if(vconf_set_int(VCONFKEY_WMS_SAFETY_MESSAGE_TRIGGER, 2) < 0) {
					_SECURE_E("Failed to set vconfkey[%s]", VCONFKEY_WMS_SAFETY_MESSAGE_TRIGGER);
				}

#if 1 // TO DO.. NEED UX Flow
				feedback_initialize();
				feedback_play(FEEDBACK_PATTERN_SAFETY_ASSISTANCE);
				feedback_deinitialize();
#endif
			}
			return ECORE_CALLBACK_CANCEL;
		}
	} else { //Safety Assistance is OFF
		if(key_info.powerkey_count%2 == 0) {
			/* double press */
			_W("powerkey double press");
			key_info.powerkey_count = 0;
			if(!_launch_app_by_double_press()) {
				_E("Failed to launch by double press");
			}
			return ECORE_CALLBACK_CANCEL;
		}
	}
	key_info.powerkey_count = 0; //initialize powerkey count

	if(!key_info.is_lcd_on) {
		_D("just turn on LCD by powerkey.. starter ignore powerkey operation");
		return ECORE_CALLBACK_CANCEL;
	}

	/* check Call state */
	if(vconf_get_int(VCONFKEY_CALL_STATE, &val) < 0) {
		_E("Failed to get call state");
		val = -1;
	}
	if(val == VCONFKEY_CALL_VOICE_ACTIVE) {
		_W("call state is [%d] -> just turn off LCD", val);
		display_change_state(LCD_OFF);
		return ECORE_CALLBACK_CANCEL;
	}

	/* checkLockstate */
	if(vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) {
		_E("Failed to get lock state");
		val = -1;
	}
	if(val == VCONFKEY_IDLE_LOCK) {
		_W("lock state is [%d] -> just turn off LCD", val);
		display_change_state(LCD_OFF);
		return ECORE_CALLBACK_CANCEL;
	}

	/* Show Idle-Clock */
	if(!_launch_home_by_powerkey())
		_E("Failed to send powerkey to home..!!");
#if 0
	/* terminate syspopup */
	syspopup_destroy_all();
#endif

	/* To notify powerkey to fmd & fmw */
	vconf_set_int("memory/wfmd/wfmd_end_key", 1);
	vconf_set_int("memory/wfmw/wfmw_end_key", 1);

	return ECORE_CALLBACK_CANCEL;
}


static Eina_Bool _long_press_timer_cb(void* data)
{
	int pid;
	bundle *b;
	int test_mode = -1;

	_W("%s", __func__);

	key_info.long_press_timer = NULL;
	key_info.is_long_press = EINA_TRUE;
	key_info.powerkey_count = 0; //initialize powerkey count

	vconf_get_int(VCONFKEY_TESTMODE_POWER_OFF_POPUP, &test_mode);
	if (test_mode == VCONFKEY_TESTMODE_POWER_OFF_POPUP_DISABLE) {
		_E("test mode => skip poweroff popup");
		return ECORE_CALLBACK_CANCEL;
	}

	//Check single powerkey press/release
	if(key_info.powerkey_timer) {
		ecore_timer_del(key_info.powerkey_timer);
		key_info.powerkey_timer = NULL;
		_D("delete powerkey_timer");
	}

#if USE_DBUS_POWEROFF
	_request_Poweroff();
#else
	b = bundle_create();
	if (!b)
		return ECORE_CALLBACK_CANCEL;
	pid = syspopup_launch("poweroff-syspopup", b);
	_D("launch power off syspopup, pid : %d", pid);
	bundle_free(b);
#endif

	return ECORE_CALLBACK_CANCEL;
}


static Eina_Bool _key_release_cb(void *data, int type, void *event)
{
	Evas_Event_Key_Up *ev = event;
	int val = -1;

	if (!ev) {
		_D("Invalid event object");
		return ECORE_CALLBACK_RENEW;
	}

	if (!ev->keyname) {
		_D("_key_release_cb : Invalid event keyname object");
		return ECORE_CALLBACK_RENEW;
	} else {
		_D("_key_release_cb : %s Released", ev->keyname);
	}

	if (!strcmp(ev->keyname, KEY_POWER)) {

		_W("POWER Key is released");

		// Check long press timer
		if(key_info.long_press_timer) {
			ecore_timer_del(key_info.long_press_timer);
			key_info.long_press_timer = NULL;
			_D("delete long press timer");
		}

		// Check powerkey timer
		if(key_info.powerkey_timer) {
			ecore_timer_del(key_info.powerkey_timer);
			key_info.powerkey_timer = NULL;
			_D("delete powerkey timer");
		}

		// Cancel key operation
		if (EINA_TRUE == key_info.is_cancel) {
			_D("Cancel key is activated");
			key_info.is_cancel = EINA_FALSE;
			key_info.powerkey_count = 0; //initialize powerkey count
			return ECORE_CALLBACK_RENEW;
		}

		// Check long press operation
		if(key_info.is_long_press) {
			_D("ignore power key release by long poress");
			key_info.is_long_press = EINA_FALSE;
			return ECORE_CALLBACK_RENEW;
		}

		if(!key_info.is_lcd_on) {
			_D("lcd off --> [%f]sec timer", POWERKEY_LCDOFF_TIMER_SEC);
			key_info.powerkey_timer = ecore_timer_add(POWERKEY_LCDOFF_TIMER_SEC, _powerkey_timer_cb, NULL);
		} else {
			key_info.powerkey_timer = ecore_timer_add(POWERKEY_TIMER_SEC, _powerkey_timer_cb, NULL);
		}

	} else if (!strcmp(ev->keyname, KEY_CANCEL)) {
		_D("CANCEL Key is released");
		key_info.is_cancel = EINA_FALSE;
	}

	return ECORE_CALLBACK_RENEW;
}



static Eina_Bool _key_press_cb(void *data, int type, void *event)
{
	Evas_Event_Key_Down *ev = event;
	int pm_val = -1;
	int val = -1;


	if (!ev) {
		_D("Invalid event object");
		return ECORE_CALLBACK_RENEW;
	}

	if (!ev->keyname) {
		_D("_key_press_cb : Invalid event keyname object");
		return ECORE_CALLBACK_RENEW;
	} else {
		_D("_key_press_cb : %s Pressed", ev->keyname);
	}

	if (!strcmp(ev->keyname, KEY_POWER)) {

		_W("POWER Key is pressed");

		// Check LCD status
		if(vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
			_D("Cannot get VCONFKEY_PM_STATE");
		}
		_W("LCD state : %d", val);
		if(val <= VCONFKEY_PM_STATE_LCDDIM) {
			key_info.is_lcd_on = EINA_TRUE;
		} else if(val >= VCONFKEY_PM_STATE_LCDOFF) {
			key_info.is_lcd_on = EINA_FALSE;
		}

		// Check powerkey press count
		key_info.powerkey_count++;
		_W("powerkey count : %d", key_info.powerkey_count);

		// Check powerkey timer
		if(key_info.powerkey_timer) {
			ecore_timer_del(key_info.powerkey_timer);
			key_info.powerkey_timer = NULL;
			_D("delete powerkey timer");
		}

		// Check long press
		if (key_info.long_press_timer) {
			ecore_timer_del(key_info.long_press_timer);
			key_info.long_press_timer = NULL;
		}
		_D("create long press timer");
		key_info.is_long_press = EINA_FALSE;
		key_info.long_press_timer = ecore_timer_add(LONG_PRESS_TIMER_SEC, _long_press_timer_cb, NULL);
		if(!key_info.long_press_timer) {
			_E("Failed to add long_press_timer");
		}

	} else if (!strcmp(ev->keyname, KEY_CANCEL)) {
		_D("CANCEL key is pressed");
		key_info.is_cancel = EINA_TRUE;
	}

	return ECORE_CALLBACK_RENEW;
}



static int _w_gesture_hold_cb(void *data, int ev_type, void *ev)
{
	Ecore_X_Event_Gesture_Notify_Hold *e = ev;
	int ret = 0;
	int val = -1;

	_D("_w_gesture_hold_cb..!!");

	/* Check critical low batt clock mode */
	if(vconf_get_int(VCONFKEY_PM_KEY_IGNORE, &val) < 0) {
		_SECURE_E("Failed to get vconfkey[%s]", VCONFKEY_PM_KEY_IGNORE);
	}
	if(val ==1) { //Critical Low Batt Clock Mode
		_E("Critical Low Batt Clock Mode, ignore gesture");
		return ECORE_CALLBACK_RENEW;
	}

	if(e->num_fingers == GRAB_TWO_FINGERS) {
		_D("subtype[%d]: hold[%d]\n", e->subtype, e->hold_time);
		if (e->subtype == ECORE_X_GESTURE_BEGIN) {
			_D("Begin : launch task mgr..!!");
			ret = w_launch_app(W_TASKMGR_PKGNAME, NULL);
			if (ret >= 0) {
				_SECURE_D("[%s] is launched, pid=[%d]", W_TASKMGR_PKGNAME, ret);
#if 0
				/* terminate syspopup */
				//syspopup_destroy_all();
				ecore_timer_add(SYSPOPUP_END_TIMER_SEC*2, _syspopup_end_timer_cb, NULL);
#endif
			}
		}
	}

	return ECORE_CALLBACK_RENEW;
}



inline static void _release_multimedia_key(const char *value)
{
	int pid = 0;

	ret_if(NULL == value);

	_D("Multimedia key is released with %s", value);

	bundle *b;
	b = bundle_create();
	if (!b) {
		_E("Cannot create bundle");
		return;
	}
	bundle_add(b, "multimedia_key", value);
	bundle_add(b, SERVICE_OPERATION_MAIN_KEY, SERVICE_OPERATION_DEFAULT_VALUE);

	pid = w_launch_app(MUSIC_PLAYER_PKG_NAME, b);
	if (pid < 0)
		_E("Failed to launch music player, ret : %d", pid);

	bundle_free(b);
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



void create_key_window(void)
{
	int status = -1;
	int ret = -1;

	_W("create_key_window..!!");

	key_info.win = ecore_x_window_input_new(0, 0, 0, 1, 1);
	if (!key_info.win) {
		_E("Failed to create hidden window");
		return;
	}
	ecore_x_event_mask_unset(key_info.win, ECORE_X_EVENT_MASK_NONE);
	ecore_x_icccm_title_set(key_info.win, "w_starter,key,receiver");
	ecore_x_netwm_name_set(key_info.win, "w_starter,key,receiver");
	ecore_x_netwm_pid_set(key_info.win, getpid());

	g_type_init();
	e_dbus_init();

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

	status = ecore_x_gesture_event_grab(key_info.win, ECORE_X_GESTURE_EVENT_HOLD, GRAB_TWO_FINGERS);
	_E("ECORE_X_GESTURE_EVENT_HOLD Grab(%d fingers) status[%d]\n", GRAB_TWO_FINGERS, status);

	key_info.two_fingers_hold_hd = ecore_event_handler_add(ECORE_X_EVENT_GESTURE_NOTIFY_HOLD,
		(Ecore_Event_Handler_Cb)_w_gesture_hold_cb, NULL);
	if (!key_info.two_fingers_hold_hd) {
		_E("Failed to register handler : ECORE_X_EVENT_GESTURE_NOTIFY_TAPNHOLD\n");
	}

	media_key_reserve(_media_key_event_cb, NULL);

}



void destroy_key_window(void)
{
	int status;

	if (key_info.two_fingers_hold_hd) {
		ecore_event_handler_del(key_info.two_fingers_hold_hd);
		key_info.two_fingers_hold_hd = NULL;
	}

	status = ecore_x_gesture_event_ungrab(key_info.win, ECORE_X_GESTURE_EVENT_HOLD, GRAB_TWO_FINGERS);
	if (!status) {
		_E("ECORE_X_GESTURE_EVENT_HOLD UnGrab(%d fingers) failed, status[%d]\n", GRAB_TWO_FINGERS, status);
	}

	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_POWER);

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

}



// End of a file
