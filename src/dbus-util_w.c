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

#include <Ecore.h>
#include <dd-display.h>
#include <aul.h>

#include "starter_w.h"
#include "dbus-util_w.h"
#include "util.h"


#define DEVICED_BUS_NAME        "org.tizen.system.deviced"
#define DEVICED_OBJECT_PATH     "/Org/Tizen/System/DeviceD"
#define DEVICED_INTERFACE_NAME  DEVICED_BUS_NAME

#define POWEROFF_BUS_NAME       "org.tizen.system.popup"
#define POWEROFF_OBJECT_PATH    "/Org/Tizen/System/Popup/Poweroff"
#define POWEROFF_INTERFACE_NAME POWEROFF_BUS_NAME".Poweroff"
#define METHOD_POWEROFF_NAME	"PopupLaunch"
#define DBUS_REPLY_TIMEOUT      (120 * 1000)

#define DISPLAY_OBJECT_PATH     DEVICED_OBJECT_PATH"/Display"
#define DEVICED_INTERFACE_DISPLAY  DEVICED_INTERFACE_NAME".display"
#define MEMBER_ALPM_ON          "ALPMOn"
#define MEMBER_ALPM_OFF         "ALPMOff"
#define MEMBER_LCD_ON 			"LCDOn"
#define MEMBER_LCD_OFF 			"LCDOff"
#define LCD_ON_BY_GESTURE		"gesture"


#define CPU_BOOSTER_OBJECT_PATH DEVICED_OBJECT_PATH"/PmQos"
#define CPU_BOOSTER_INTERFACE 	DEVICED_BUS_NAME".PmQos"
#define METHOD_CPU_BOOSTER		"AppLaunchHome"
#define DBUS_CPU_BOOSTER_SEC 	200

#define COOL_DOWN_MODE_PATH 		DEVICED_OBJECT_PATH"/SysNoti"
#define COOL_DOWN_MODE_INTERFACE 	DEVICED_BUS_NAME".SysNoti"
#define SIGNAL_COOL_DOWN_MODE		"CoolDownChanged"
#define METHOD_COOL_DOWN_MODE		"GetCoolDownStatus"
#define COOL_DOWN_MODE_RELEASE		"Release"
#define COOL_DOWN_MODE_LIMITACTION 	"LimitAction"

#define NIKE_RUNNING_STATUS_PATH		"/Com/Nike/Nrunning/RunningMgr"
#define NIKE_RUNNING_STATUS_INTERFACE 	"com.nike.nrunning.runningmgr"
#define NIKE_RUNNING_STATUS_SIGNAL		"RunningStatus"
#define NIKE_RUNNING_STATUS_START		"start"
#define NIKE_RUNNING_STATUS_END			"end"

#define DBUS_ALPM_CLOCK_PATH 			"/Org/Tizen/System/AlpmMgr"
#define DBUS_ALPM_CLOCK_INTERFACE 		"org.tizen.system.alpmmgr"
#define DBUS_ALPM_CLOCK_MEMBER_STATUS 	"ALPMStatus"
#define ALPM_CLOCK_SHOW 				"show"

#define DBUS_STARTER_ALPMCLOCK_PATH			"/Org/Tizen/Coreapps/starter"
#define DBUS_STARTER_ALPMCLOCK_INTERFACE	"org.tizen.coreapps.starter.alpmclock"
#define DBUS_STARTER_ALPMCLOCK_MEMBER		"show"


static struct _info {
	DBusConnection *connection;
} s_info = {
	.connection = NULL,
};

static int pid_ALPM_clock = 0;

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

DBusConnection *starter_dbus_connection_get(void) {
	if (s_info.connection == NULL) {
		_W("no connection for dbus. get dbus connection");
		DBusError derror;
		DBusConnection *connection = NULL;

		dbus_error_init(&derror);
		connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &derror);
		if (connection == NULL) {
			_E("Failed to get dbus connection:%s", derror.message);
			dbus_error_free(&derror);
			return NULL;
		}
		dbus_connection_setup_with_g_main(connection, NULL);
		dbus_error_free(&derror);

		s_info.connection = connection;
	}

	return s_info.connection;
}

static int _dbus_message_send(const char *path, const char *interface, const char *member)
{
	int ret = 0;
	DBusMessage *msg = NULL;
	DBusConnection *conn = NULL;

	conn = (DBusConnection *)starter_dbus_connection_get();
	if (!conn) {
		_E("dbus_bus_get error");
		return -1;
	}
	_E("dbus_message_new_signal(%s:%s-%s)", path, interface, member);

	msg = dbus_message_new_signal(path, interface, member);
	if (!msg) {
		_E("dbus_message_new_signal(%s:%s-%s)", path, interface, member);
		return -1;
	}

	ret = dbus_connection_send(conn, msg, NULL); //async call
	dbus_message_unref(msg);
	if (ret != TRUE) {
		_E("dbus_connection_send error(%s:%s-%s)", path, interface, member);
		return -ECOMM;
	}
	_D("dbus_connection_send, ret=%d", ret);
	return 0;
}

void starter_dbus_alpm_clock_signal_send(void *data)
{
	int ret = 0;

	struct appdata *ad = (struct appdata *)data;
	if(ad == NULL){
		_E("app data is NULL");
		return;
	}
#if 0
	if(ad->lcd_status == 1){
		_W("LCD is already on. Do not send alpm clock show msg.");
		return;
	}
#endif

	ret = _dbus_message_send(
			DBUS_STARTER_ALPMCLOCK_PATH,
			DBUS_STARTER_ALPMCLOCK_INTERFACE,
			DBUS_STARTER_ALPMCLOCK_MEMBER);
	_E("Sending alpmclock show signal, result:%d", ret);
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

DBusMessage *_invoke_dbus_method_async(const char *dest, const char *path,
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

int get_dbus_cool_down_mode(void *data)
{
	_D();
	struct appdata *ad = (struct appdata *)data;
	if(ad == NULL){
		_E("app data is NULL");
		return -1;
	}
	DBusError err;
	DBusMessage *msg;
	int ret = 0;
	char *str = NULL;
	int ret_val = 0;

	msg = _invoke_dbus_method_sync(DEVICED_BUS_NAME, COOL_DOWN_MODE_PATH, COOL_DOWN_MODE_INTERFACE,
			METHOD_COOL_DOWN_MODE, NULL, NULL);
	if (!msg)
		return -EBADMSG;

	dbus_error_init(&err);

	ret = dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);
	if (!ret) {
		_E("no message : [%s:%s]", err.name, err.message);
		ret_val = -EBADMSG;
	}

	dbus_message_unref(msg);
	dbus_error_free(&err);

	_D("%s-%s : %s", COOL_DOWN_MODE_INTERFACE, METHOD_COOL_DOWN_MODE, str);

	if(!strncmp(str, COOL_DOWN_MODE_RELEASE, strlen(COOL_DOWN_MODE_RELEASE))){
		_D("%s", COOL_DOWN_MODE_RELEASE);
		ad->cool_down_mode = 0;
		return ret_val;
	}
	if(!strncmp(str, COOL_DOWN_MODE_LIMITACTION, strlen(COOL_DOWN_MODE_LIMITACTION))){
		_D("%s", COOL_DOWN_MODE_LIMITACTION);
		ad->cool_down_mode = 1;
		return ret_val;
	}
}

int request_dbus_cpu_booster(void)
{
	DBusError err;
	DBusMessage *msg;
	int ret, ret_val;
	char *arr[1];
	char val[32];

	snprintf(val, sizeof(val), "%d", DBUS_CPU_BOOSTER_SEC);
	arr[0] = val;

	msg = _invoke_dbus_method_async(DEVICED_BUS_NAME, CPU_BOOSTER_OBJECT_PATH, CPU_BOOSTER_INTERFACE,
			METHOD_CPU_BOOSTER, "i", arr);
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

	_D("%s-%s : %d", CPU_BOOSTER_INTERFACE, METHOD_CPU_BOOSTER, ret_val);
	return ret_val;
}


int request_Poweroff(void)
{
	DBusError err;
	DBusMessage *msg;
	int ret, ret_val;

	msg = _invoke_dbus_method_async(POWEROFF_BUS_NAME, POWEROFF_OBJECT_PATH, POWEROFF_INTERFACE_NAME,
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


#define W_CLOCK_ALPM_PKGNAME "com.samsung.idle-clock-alpm"
static void _on_ALPM_changed_receive(void *data, DBusMessage *msg)
{
	DBusError err;
	char *str;
	int response;
	int r;
	int alpm_on = 0;	
	int alpm_off = 0;

	_D("ALPM signal is received");

	alpm_on = dbus_message_is_signal(msg, DEVICED_INTERFACE_DISPLAY, MEMBER_ALPM_ON);
	alpm_off = dbus_message_is_signal(msg, DEVICED_INTERFACE_DISPLAY, MEMBER_ALPM_OFF);

	if (alpm_on) {
		pid_ALPM_clock = w_launch_app(W_CLOCK_ALPM_PKGNAME, NULL);
		_D("Launch ALPM clock[%s], pid[%d]", W_CLOCK_ALPM_PKGNAME, pid_ALPM_clock);
	} else if (alpm_off) {
#if 1
		aul_terminate_pid(pid_ALPM_clock);
#else
		if (!pid_ALPM_clock) {
			aul_terminate_pid(pid_ALPM_clock);
			_D("Terminate ALPM clock pid[%d]", pid_ALPM_clock);
			pid_ALPM_clock = 0;
		} else {
			_E(" ALPM clock pid is 0");
		}
#endif
	} else {
		_E("%s dbus_message_is_signal error", DEVICED_INTERFACE_DISPLAY);
	}
}


int init_dbus_ALPM_signal(void *data)
{
	E_DBus_Connection *conn;
	E_DBus_Signal_Handler *handler;
	int r;

	g_type_init();
	e_dbus_init();

	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (conn == NULL) {
		_E("e_dbus_bus_get error");
		return -1;
	}

	handler = e_dbus_signal_handler_add(conn, NULL, DISPLAY_OBJECT_PATH,
								DEVICED_INTERFACE_DISPLAY, MEMBER_ALPM_ON,
								_on_ALPM_changed_receive, NULL);
	if (handler == NULL) {
		_E("e_dbus_signal_handler_add error");
		return -1;
	}


	handler = e_dbus_signal_handler_add(conn, NULL, DISPLAY_OBJECT_PATH,
								DEVICED_INTERFACE_DISPLAY, MEMBER_ALPM_OFF,
								_on_ALPM_changed_receive, NULL);
	if (handler == NULL) {
		_E("e_dbus_signal_handler_add error");
		return -1;
	}
}

static void _on_COOL_DOWN_MODE_changed_receive(void *data, DBusMessage *msg)
{
	struct appdata *ad = (struct appdata *)data;
	if(ad == NULL){
		_E("app data is NULL");
		return;
	}
	DBusError err;
	const char *str = NULL;
	int response;
	int r;
	int mode_release = 0;
	int mode_limitaction = 0;

	_W("COOL DOWN MODE signal is received");
	dbus_error_init(&err);

	r = dbus_message_is_signal(msg, COOL_DOWN_MODE_INTERFACE, SIGNAL_COOL_DOWN_MODE);

	r = dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);
	if (!r) {
		_E("no message : [%s:%s]", err.name, err.message);
		return;
	}

	_D("received siganl : %s", str);

	if (!strncmp(str, COOL_DOWN_MODE_RELEASE, strlen(COOL_DOWN_MODE_RELEASE))) {
		_W("%s", COOL_DOWN_MODE_RELEASE);
		ad->cool_down_mode = 0;
	} else if (!strncmp(str, COOL_DOWN_MODE_LIMITACTION, strlen(COOL_DOWN_MODE_LIMITACTION))) {
		_W("%s", COOL_DOWN_MODE_LIMITACTION);
		ad->cool_down_mode = 1;
	} else {
		_E("%s dbus_message_is_signal error", SIGNAL_COOL_DOWN_MODE);
	}
}

int init_dbus_COOL_DOWN_MODE_signal(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	if(ad == NULL){
		_E("app data is NULL");
		return -1;
	}
	E_DBus_Connection *conn;
	E_DBus_Signal_Handler *handler;
	int r;

	g_type_init();
	e_dbus_init();

	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (conn == NULL) {
		_E("e_dbus_bus_get error");
		return -1;
	}

	handler = e_dbus_signal_handler_add(conn, NULL, COOL_DOWN_MODE_PATH,
								COOL_DOWN_MODE_INTERFACE, SIGNAL_COOL_DOWN_MODE,
								_on_COOL_DOWN_MODE_changed_receive, ad);
	if (handler == NULL) {
		_E("e_dbus_signal_handler_add error");
		return -1;
	}
}
#if 0
static void _on_NIKE_RUNNING_STATUS_changed_receive(void *data, DBusMessage *msg)
{
	struct appdata *ad = (struct appdata *)data;
	if(ad == NULL){
		_E("app data is NULL");
		return;
	}
	DBusError err;
	const char *str = NULL;
	int response;
	int r;
	int mode_release = 0;
	int mode_limitaction = 0;

	_W("NIKE RUNNING STATUS signal is received");
	dbus_error_init(&err);

	r = dbus_message_is_signal(msg, NIKE_RUNNING_STATUS_INTERFACE, NIKE_RUNNING_STATUS_SIGNAL);

	r = dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);
	if (!r) {
		_E("no message : [%s:%s]", err.name, err.message);
		return;
	}

	_W("received siganl : %s", str);

	if (!strncmp(str, NIKE_RUNNING_STATUS_START, strlen(NIKE_RUNNING_STATUS_START))) {
		_W("%s", NIKE_RUNNING_STATUS_START);
		ad->nike_running_status = 1;
	} else if (!strncmp(str, NIKE_RUNNING_STATUS_END, strlen(NIKE_RUNNING_STATUS_END))) {
		_W("%s", NIKE_RUNNING_STATUS_END);
		ad->nike_running_status = 0;
	} else {
		_E("%s dbus_message_is_signal error", NIKE_RUNNING_STATUS_SIGNAL);
	}
	clock_mgr_set_reserved_apps_status(ad->nike_running_status, STARTER_RESERVED_APPS_NIKE, ad);
}

int init_dbus_NIKE_RUNNING_STATUS_signal(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	if(ad == NULL){
		_E("app data is NULL");
		return -1;
	}
	E_DBus_Connection *conn;
	E_DBus_Signal_Handler *handler;
	int r;

	g_type_init();
	e_dbus_init();

	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (conn == NULL) {
		_E("e_dbus_bus_get error");
		return -1;
	}

	handler = e_dbus_signal_handler_add(conn, NULL, NIKE_RUNNING_STATUS_PATH,
								NIKE_RUNNING_STATUS_INTERFACE, NIKE_RUNNING_STATUS_SIGNAL,
								_on_NIKE_RUNNING_STATUS_changed_receive, ad);
	if (handler == NULL) {
		_E("e_dbus_signal_handler_add error");
		return -1;
	}
}
#endif

static void _on_lcd_changed_receive(void *data, DBusMessage *msg)
{
	DBusError err;
	const char *str = NULL;
	int response;
	int r;
	int lcd_on = 0;
	int lcd_off = 0;

	struct appdata *ad = (struct appdata *)data;

	_D("LCD signal is received");

	lcd_on = dbus_message_is_signal(msg, DEVICED_INTERFACE_DISPLAY, MEMBER_LCD_ON);
	lcd_off = dbus_message_is_signal(msg, DEVICED_INTERFACE_DISPLAY, MEMBER_LCD_OFF);


	if (lcd_on) {
#if 0
		dbus_error_init(&err);

		r = dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);

		if (!r) {
			_E("no message : [%s:%s]", err.name, err.message);
			return;
		}

		_W("%s, %d, str=[%s]", __func__, __LINE__, str);

		if (!strncmp(str, LCD_ON_BY_GESTURE, strlen(LCD_ON_BY_GESTURE))) {
			_W("LCD ON by [%s], do not terminate ALPM clock", LCD_ON_BY_GESTURE);
		} else {
			if (ad->pid_ALPM_clock > 0) {
				_W("LCD ON by [%s], terminate ALPM clock pid[%d]", str, ad->pid_ALPM_clock);
				aul_terminate_pid(ad->pid_ALPM_clock);
				ad->pid_ALPM_clock = 0;
			}
		}
#endif
		_W("LCD on");
		ad->lcd_status = 1;
	}
	else if(lcd_off){
		_W("LCD off");
		ad->lcd_status = 0;
	}else {
		_E("%s dbus_message_is_signal error", DEVICED_INTERFACE_DISPLAY);
	}
}


int init_dbus_lcd_on_off_signal(void *data)
{
	E_DBus_Connection *conn;
	E_DBus_Signal_Handler *handler;
	int r;

	g_type_init();
	e_dbus_init();

	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (conn == NULL) {
		_E("e_dbus_bus_get error");
		return -1;
	}


	handler = e_dbus_signal_handler_add(conn, NULL, DISPLAY_OBJECT_PATH,
								DEVICED_INTERFACE_DISPLAY, MEMBER_LCD_ON,
								_on_lcd_changed_receive, data);
	if (handler == NULL) {
		_E("e_dbus_signal_handler_add error");
		return -1;
	}

	handler = e_dbus_signal_handler_add(conn, NULL, DISPLAY_OBJECT_PATH,
								DEVICED_INTERFACE_DISPLAY, MEMBER_LCD_OFF,
								_on_lcd_changed_receive, data);
	if (handler == NULL) {
		_E("e_dbus_signal_handler_add error");
		return -1;
	}
}



static void _on_ALPM_clock_state_changed_receive(void *data, DBusMessage *msg)
{
	DBusError err;
	const char *str = NULL;
	int response;
	int r;
	int lcd_on = 0;	

	struct appdata *ad = (struct appdata *)data;

	_D("ALPM clock state is received");

	lcd_on = dbus_message_is_signal(msg, DBUS_ALPM_CLOCK_INTERFACE, DBUS_ALPM_CLOCK_MEMBER_STATUS);


	if (lcd_on) {
		dbus_error_init(&err);

		r = dbus_message_get_args(msg, &err, DBUS_TYPE_STRING, &str, DBUS_TYPE_INVALID);

		if (!r) {
			_E("no message : [%s:%s]", err.name, err.message);
			return;
		}

		_D("%s, %d, str=[%s]", __func__, __LINE__, str);
		if (!strncmp(str, ALPM_CLOCK_SHOW, strlen(ALPM_CLOCK_SHOW))) {
			_W("ALPM clock state is [%s]", ALPM_CLOCK_SHOW);
			ad->ALPM_clock_state = 1;
		} else {
			ad->ALPM_clock_state = 0;
		}
	} else {
		_E("%s dbus_message_is_signal error", DBUS_ALPM_CLOCK_INTERFACE);
	}
}

int init_dbus_ALPM_clock_state_signal(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	if(ad == NULL){
		_E("app data is NULL");
		return -1;
	}
	E_DBus_Connection *conn;
	E_DBus_Signal_Handler *handler;
	int r;

	g_type_init();
	e_dbus_init();

	ad->ALPM_clock_state = 0;
	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (conn == NULL) {
		_E("e_dbus_bus_get error");
		return -1;
	}

	handler = e_dbus_signal_handler_add(conn, NULL, DBUS_ALPM_CLOCK_PATH,
								DBUS_ALPM_CLOCK_INTERFACE, DBUS_ALPM_CLOCK_MEMBER_STATUS,
								_on_ALPM_clock_state_changed_receive, ad);

	if (handler == NULL) {
		_E("e_dbus_signal_handler_add error");
		return -1;
	}

	return 1;
}


