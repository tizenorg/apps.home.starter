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

#include <stdio.h>
#include <Ecore.h>
#include <dd-display.h>
#include <dd-led.h>
#include <aul.h>
#include <vconf.h>
#include <Ecore_X.h>
#include <Ecore_Input.h>
#include <utilX.h>
#include <feedback.h>
#include<Elementary.h>
#ifndef FEATURE_LITE
#include <tts.h>
#endif

#include "starter.h"
#include "util.h"
#include "direct-access.h"

#define _(str) dgettext("starter", str)

#define DBUS_REPLY_TIMEOUT (120 * 1000)
#define DEVICED_BUS_NAME        "org.tizen.system.deviced"
#define DEVICED_OBJECT_PATH     "/Org/Tizen/System/DeviceD"
#define DEVICED_INTERFACE_NAME  DEVICED_BUS_NAME

#define ALWAYS_ASK_BUS_NAME       "org.tizen.system.popup"
#define ALWAYS_ASK_OBJECT_PATH    "/Org/Tizen/System/Popup/System"
#define ALWAYS_ASK_INTERFACE_NAME ALWAYS_ASK_BUS_NAME".System"
#define METHOD_ALWAYS_ASK_NAME    "AccessibilityPopupLaunch"
#define ALWAYS_ASK_PARAM_1        "_SYSPOPUP_CONTENT_"
#define ALWAYS_ASK_PARAM_2        "accessibility"

#ifndef FEATURE_LITE
static tts_h tts;
#endif
static int tts_status;

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


DBusMessage *invoke_dbus_method_sync(const char *dest, const char *path,
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
}


DBusMessage *invoke_dbus_method(const char *dest, const char *path,
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

	r = dbus_connection_send(conn, msg, NULL);
	dbus_message_unref(msg);
	if (r != TRUE) {
		_E("dbus_connection_send error(%s:%s:%s-%s)", dest, path, interface, method);
		return -ECOMM;
	}
	_D("dbus_connection_send, ret=%d", r);
	return NULL;
}








static void _set_assistive_light(void)
{
	int max, state, ret;

	max = led_get_max_brightness();
	if (max < 0)
		max = 1;

	state = led_get_brightness();
	if (state > 0) {
		ret = led_set_brightness_with_noti(0, true);
		if (ret == 0)
			vconf_set_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TORCH_LIGHT, 0);
	} else {
		ret = led_set_brightness_with_noti(max, true);
		if (ret == 0)
			vconf_set_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TORCH_LIGHT, 1);
	}

	if(feedback_initialize() == FEEDBACK_ERROR_NONE)
	{
		_D("make vibration effect");
		feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_TAP);
		feedback_deinitialize();
	}
}



#define LIVE_SETTING_APP "com.samsung.live-setting-app"
static int _launch_live_setting_app(void)
{
	bundle *b;
	int ret;

	b = bundle_create();
	if (!b) {
		_E("Failed to create bundle");
		return -ENOMEM;
	}

	ret = bundle_add(b, "popup", "zoom");
	if (ret < 0) {
		_E("Failed to add parameters to bundle");
		goto out;
	}

	ret = aul_launch_app(LIVE_SETTING_APP, b);
	if (ret < 0)
		_E("Failed to launch app(%s)", LIVE_SETTING_APP);

out:
	bundle_free(b);
	return ret;
}

static bool _is_aircommand_on(void)
{
	int state;
	if (vconf_get_bool(VCONFKEY_AIRCOMMAND_ENABLED, &state) == 0
			&& state == 1)
		return true;
	return false;
}

#define PROP_ZOOM      "_E_ACC_ENABLE_ZOOM_UI_"
static void _set_zoom(void)
{
	int ret;
	unsigned int value;
	Ecore_X_Window rootWin;
	Ecore_X_Atom atomZoomUI;

	rootWin = ecore_x_window_root_first_get();
	atomZoomUI = ecore_x_atom_get(PROP_ZOOM);

	ret = ecore_x_window_prop_card32_get(rootWin, atomZoomUI, &value, 1);
	if (ret == 1 && value == 1)
		value = 0;
	else
		value = 1;

	ecore_x_window_prop_card32_set(rootWin, atomZoomUI, &value, 1);
	ecore_x_flush();

	vconf_set_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_SCREEN_ZOOM, value);
}



#define PROP_HIGH_CONTRAST "_E_ACC_ENABLE_HIGH_CONTRAST_"
static void _set_contrast(void)
{
	int ret;
	unsigned int value;
	Ecore_X_Window rootWin;
	Ecore_X_Atom atomHighContrast;

	rootWin = ecore_x_window_root_first_get();
	atomHighContrast = ecore_x_atom_get(PROP_HIGH_CONTRAST);

	ret = ecore_x_window_prop_card32_get(rootWin, atomHighContrast, &value, 1);
	if (ret == 1 && value == 1)
		value = 0;
	else
		value = 1;

	ecore_x_window_prop_card32_set(rootWin, atomHighContrast, &value, 1);
	ecore_x_flush();

	vconf_set_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_HIGH_CONTRAST, value);
}
#ifndef FEATURE_LITE
void utterance_completed_callback(tts_h tts, int utt_id, void* user_data){
	_D("");
	int ret = 0;
	int u_id = 0;
	ret = tts_stop(tts);
	if(ret != TTS_ERROR_NONE){
		_E("fail to stop(%d)", ret);
		return;
	}
	ret = tts_unprepare(tts);
	if(ret != TTS_ERROR_NONE){
		_E("fail to unprepare(%d)", ret);
		return;
	}
	if(tts){
		ret = tts_destroy(tts);
		tts = NULL;
	}
	return;
}

void _tts_state_changed_cb(tts_h tts, tts_state_e previous, tts_state_e current, void* data){
	int ret = 0;
	int u_id = 0;
	bindtextdomain("starter", "/usr/share/locale");
	appcore_set_i18n();
	if(TTS_STATE_CREATED == previous && current == TTS_STATE_READY){
		if(tts_status){
			ret = tts_add_text(tts, _("IDS_TPLATFORM_BODY_SCREEN_READER_ENABLED_T_TTS"), NULL, TTS_VOICE_TYPE_AUTO, TTS_SPEED_AUTO, &u_id);
			if(ret != TTS_ERROR_NONE){
				_E("fail to add text(%d)", ret);
				return;
			}
		}
		else{
			ret = tts_add_text(tts, _("IDS_TPLATFORM_BODY_SCREEN_READER_DISABLED_T_TTS"), NULL, TTS_VOICE_TYPE_AUTO, TTS_SPEED_AUTO, &u_id);
			if(ret != TTS_ERROR_NONE){
				_E("fail to add text(%d)", ret);
				return;
			}
		}
		ret = tts_play(tts);
		if(ret != TTS_ERROR_NONE){
			_E("fail to play(%d)", ret);
			return;
		}
	}
}

static void _starter_tts_play(void){
	int ret = 0;
	int u_id = 0;

	ret = tts_create(&tts);
	if(ret != TTS_ERROR_NONE){
		_E("fail to get handle(%d)", ret);
		return;
	}
	ret = tts_set_state_changed_cb(tts, _tts_state_changed_cb, NULL);
	if(ret != TTS_ERROR_NONE){
		_E("fail to set state changed cb(%d)", ret);
		return;
	}
	ret = tts_set_mode(tts, TTS_MODE_NOTIFICATION);
	if(ret != TTS_ERROR_NONE){
		_E("fail to set mode(%d)", ret);
		return;
	}
	ret = tts_set_utterance_completed_cb(tts, utterance_completed_callback, NULL);
	if(ret != TTS_ERROR_NONE){
		_E("fail to prepare(%d)", ret);
		return;
	}
	ret = tts_prepare(tts);
	if(ret != TTS_ERROR_NONE){
		_E("fail to prepare(%d)", ret);
		return;
	}
	return;
}
#endif
static int _set_accessibility_tts(void)
{
	if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &tts_status) < 0) {
		_E("FAIL: vconf_get_bool()");
		return -1;
	}

	if (tts_status == FALSE)
		tts_status = TRUE;
	else
		tts_status = FALSE;
	if (vconf_set_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, tts_status) < 0) {
		_E("FAIL: vconf_set_bool()");
		return -1;
	}
#ifndef FEATURE_LITE
	_starter_tts_play();
#endif
	return 0;
}



static int _launch_always_ask(void)
{
	DBusError err;
	DBusMessage *msg;
	int ret, ret_val;
	char *arr[2];
	char val[32];

	arr[0] = ALWAYS_ASK_PARAM_1;
	arr[1] = ALWAYS_ASK_PARAM_2;

	msg = invoke_dbus_method(ALWAYS_ASK_BUS_NAME, ALWAYS_ASK_OBJECT_PATH, ALWAYS_ASK_INTERFACE_NAME,
			METHOD_ALWAYS_ASK_NAME, "ss", arr);
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

	_D("%s-%s : %d", ALWAYS_ASK_INTERFACE_NAME, METHOD_ALWAYS_ASK_NAME, ret_val);
	return ret_val;
}



int launch_direct_access(int access_val)
{
	int ret = 0;
	_D("launch_direct_access [%d]", access_val);

	/* Direct Access Operation */
	switch (access_val) {
		case SETTING_POWERKEY_SHORTCUT_ALWAYS_ASK:
			if (!_launch_always_ask())
				ret = -1;
			break;
		case SETTING_POWERKEY_SHORTCUT_SCREEN_READER_TTS:
			if (!_set_accessibility_tts())
				ret = -1;
			break;
		case SETTING_POWERKEY_SHORTCUT_NEGATIVE_COLOURS:
			_set_contrast();
			break;
		case SETTING_POWERKEY_SHORTCUT_ZOOM:
			if (_is_aircommand_on()) {
				ret = -1;
				if (_launch_live_setting_app() < 0)
					_E("Failed to launch (%s)", LIVE_SETTING_APP);
				return ret;
			}
			_set_zoom();
			break;
		case SETTING_POWERKEY_SHORTCUT_ASSISTIVE_LIGHT:
			_set_assistive_light();
			break;
		case SETTING_POWERKEY_SHORTCUT_SHOT_READER:

			break;
		default:
			ret = -1;
			break;
	}
	return ret;
}

