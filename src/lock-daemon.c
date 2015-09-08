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

#include <vconf.h>
#include <vconf-keys.h>

#include <glib.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <errno.h>
//#include <sensors.h>
#include <feedback.h>
#include <alarm.h>
#include <time.h>
#include <dd-deviced.h>
#include <dd-display.h>
#include <E_DBus.h>
#include <aul.h>

#ifndef FEATURE_LITE
#include <context_manager.h>
#endif

#include "lockd-debug.h"
#include "lock-daemon.h"
#include "lockd-process-mgr.h"
#include "lockd-window-mgr.h"
#include "starter-util.h"
#include "menu_daemon.h"
#include "hw_key.h"
#include "lockd-bt.h"
#include "dbus-util.h"

static int phone_lock_pid;
static int auto_lock_pid;
static bool lock_state_available = TRUE;

struct lockd_data {
	int lock_app_pid;
	int phone_lock_app_pid;
	int lock_type;	/* -1: None, 0:Normal, 1:Security,  2:Other */
	Eina_Bool is_sensor;	/* EINA_TRUE : can use sensor EINA_FALSE : cannot use */
	Eina_Bool request_recovery;
	Eina_Bool back_to_app;
	lockw_data *lockw;
	GPollFD *gpollfd;
	Eina_Bool is_alarm;	/* EINA_TRUE : can use alarm EINA_FALSE : cannot use */
	alarm_id_t alarm_id;	/* -1 : None, others : set alarm */
	Eina_Bool is_first_boot;	/* EINA_TRUE : first boot  */
	int hall_status;	/* 0: cover is closed, 1:cover is opened */
	E_DBus_Connection *hall_conn;
	E_DBus_Signal_Handler *hall_handler;
	E_DBus_Connection *display_conn;
	E_DBus_Signal_Handler *display_handler;

};



/* Local features*/
#define _FEATURE_LCD_OFF_DBUS	0 //Check to apply lcd off dbus by PM part
#define _FEATURE_SCOVER			0 //Scover view




#define PHLOCK_SOCK_PREFIX "/tmp/phlock"
#define PHLOCK_SOCK_MAXBUFF 65535

#define VOLUNE_APP_CMDLINE "/usr/apps/com.samsung.volume/bin/volume"		 // LOCK_EXIT_CMD
#define PHLOCK_APP_CMDLINE "/usr/apps/org.tizen.lockscreen/bin/lockscreen" //PHLOCK_UNLOCK_CMD
#define MDM_APP_CMDLINE "/usr/bin/mdm-server" //PHLOCK_LAUNCH_CMD
#define VOICE_CALL_APP_CMDLINE "/usr/apps/com.samsung.call/bin/calls" //HOME_RAISE_CMD, LOCK_SHOW_CMD
#define VIDEO_CALL_APP_CMDLINE "/usr/apps/com.samsung.vtmain/bin/vtmain" //HOME_RAISE_CMD, LOCK_SHOW_CMD
#define OMA_DRM_AGENT_CMDLINE "/usr/bin/oma-dm-agent" //PHLOCK_UNLOCK_RESET_CMD

#define PHLOCK_UNLOCK_CMD "unlock"
#define PHLOCK_LAUNCH_CMD "launch_phone_lock"
#define PHLOCK_UNLOCK_RESET_CMD "unlock_reset"
#define HOME_RAISE_CMD "raise_homescreen"
#define LOCK_SHOW_CMD "show_lockscreen"
#define LOCK_LAUNCH_CMD "launch_lockscreen"
#define LOCK_EXIT_CMD "exit_lockscreen"

#define LAUNCH_INTERVAL 100*1000

#define HALL_COVERED_STATUS		0
#define BUS_NAME       "org.tizen.system.deviced"
#define OBJECT_PATH    "/Org/Tizen/System/DeviceD"
#define INTERFACE_NAME BUS_NAME
#define DEVICED_PATH_HALL		OBJECT_PATH"/Hall"
#define DEVICED_INTERFACE_HALL	INTERFACE_NAME".hall"
#define SIGNAL_HALL_STATE	"ChangeState"
#define METHOD_GET_STATUS	"getstatus"
#define DBUS_REPLY_TIMEOUT (120 * 1000)
#define DEVICED_PATH_PMQOS		OBJECT_PATH"/PmQos"
#define DEVICED_INTERFACE_PMQOS	INTERFACE_NAME".PmQos"
#define METHOD_PMQOS_NAME	"LockScreen"
#define REQUEST_PMQOS_DURATION (2 * 1000)

#define DEVICED_PATH_LCD_OFF 	OBJECT_PATH"/Display"
#define DEVICED_INTERFACE_LCD_OFF	INTERFACE_NAME".display"
#define METHOD_LCD_OFF_NAME	"LCDOffByPowerkey"

#define VOLUME_PKG_NAME "org.tizen.volume"
#define WALLPAPER_UI_SERVICE_PKG_NAME "com.samsung.wallpaper-ui-service"
#define SHOW_LOCK "show_lock"
#define ISTRUE "TRUE"
#define SMART_ALERT_INTERVAL 0.5

enum {
	LOCK_TYPE_NONE = -1,
	LOCK_TYPE_NORMAL = 0,
	LOCK_TYPE_SECURITY,
	LOCK_TYPE_AUTO_LOCK,
	LOCK_TYPE_OTHER,
	LOCK_TYPE_MAX
};

static int lockd_launch_app_lockscreen(struct lockd_data *lockd);
static void lockd_unlock_lockscreen(struct lockd_data *lockd);
#if 0
static Eina_Bool lockd_start_sensor(void *data);
static void lockd_stop_sensor(void *data);
#endif

static void _lockd_set_lock_state(int lock_state)
{
	LOCKD_DBG("%s, %d, %d", __func__, __LINE__, lock_state);
	if(lock_state < VCONFKEY_IDLE_UNLOCK || lock_state > VCONFKEY_IDLE_LAUNCHING_LOCK) {
		LOCKD_ERR("Can't set lock_state : %d out of range", lock_state);
	} else {
		vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, lock_state);
	}
}

static void _lockd_play_sound(bool unlock)
{
	int ret = -1, val = 0;
	ret = vconf_get_bool(VCONFKEY_SETAPPL_SOUND_LOCK_BOOL, &val);

	if(ret == 0 && val == 1) {
		feedback_initialize();
		feedback_play_type(FEEDBACK_TYPE_SOUND, unlock ? FEEDBACK_PATTERN_UNLOCK : FEEDBACK_PATTERN_LOCK);
	}
}

int lockd_get_lock_type(void)
{
	int lock_type = 0;
	int ret = 0;

	vconf_get_int(VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, &lock_type);

	if (lock_type == SETTING_SCREEN_LOCK_TYPE_NONE) {
		ret = LOCK_TYPE_NONE;
	} else if (lock_type == SETTING_SCREEN_LOCK_TYPE_PASSWORD ||
		lock_type == SETTING_SCREEN_LOCK_TYPE_SIMPLE_PASSWORD ||
		lock_type == SETTING_SCREEN_LOCK_TYPE_FINGERPRINT ||
		lock_type == SETTING_SCREEN_LOCK_TYPE_FACE_AND_VOICE) {
		ret = LOCK_TYPE_SECURITY;
	} else if (lock_type == SETTING_SCREEN_LOCK_TYPE_SWIPE ||
		lock_type == SETTING_SCREEN_LOCK_TYPE_MOTION) {
		ret = LOCK_TYPE_NORMAL;
	} else if (lock_type == SETTING_SCREEN_LOCK_TYPE_AUTO_LOCK) {
		ret = LOCK_TYPE_AUTO_LOCK;
	} else {
		ret = LOCK_TYPE_OTHER;
	}

	LOCKD_DBG("lockd_get_lock_type ret(%d), lock_type (%d)", ret, lock_type);

	return ret;
}

static int _lockd_append_variant(DBusMessageIter *iter, const char *sig, char *param[])
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

DBusMessage *_lockd_invoke_dbus_method_async(const char *dest, const char *path,
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
		LOCKD_ERR("dbus_bus_get error");
		return NULL;
	}

	msg = dbus_message_new_method_call(dest, path, interface, method);
	if (!msg) {
		LOCKD_ERR("dbus_message_new_method_call(%s:%s-%s)", path, interface, method);
		return NULL;
	}

	dbus_message_iter_init_append(msg, &iter);
	r = _lockd_append_variant(&iter, sig, param);
	if (r < 0) {
		LOCKD_ERR("append_variant error(%d)", r);
		return NULL;
	}

	r = dbus_connection_send(conn, msg, NULL);
	dbus_message_unref(msg);
	if (r != TRUE) {
		LOCKD_ERR("dbus_connection_send error(%s:%s:%s-%s)", dest, path, interface, method);
		return -ECOMM;
	}
	LOCKD_DBG("dbus_connection_send, ret=%d", r);
	return NULL;
}

DBusMessage *_lockd_invoke_dbus_method_sync(const char *dest, const char *path,
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
		LOCKD_ERR("dbus_bus_get error");
		return NULL;
	}

	msg = dbus_message_new_method_call(dest, path, interface, method);
	if (!msg) {
		LOCKD_ERR("dbus_message_new_method_call(%s:%s-%s)", path, interface, method);
		return NULL;
	}

	dbus_message_iter_init_append(msg, &iter);
	r = _lockd_append_variant(&iter, sig, param);
	if (r < 0) {
		LOCKD_ERR("append_variant error(%d)", r);
		return NULL;
	}

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_REPLY_TIMEOUT, &err);
	if (!reply) {
		LOCKD_ERR("dbus_connection_send error(No reply)");
	}

	if (dbus_error_is_set(&err)) {
		LOCKD_ERR("dbus_connection_send error(%s:%s)", err.name, err.message);
		reply = NULL;
	}

	dbus_message_unref(msg);
	dbus_error_free(&err);

	return reply;
}

int lockd_get_hall_status(void)
{
	DBusError err;
	DBusMessage *msg;
	int ret, ret_val;

	LOCKD_DBG("[ == %s == ]", __func__);
	msg = _lockd_invoke_dbus_method_sync(BUS_NAME, DEVICED_PATH_HALL, DEVICED_INTERFACE_HALL,
			METHOD_GET_STATUS, NULL, NULL);
	if (!msg)
		return -EBADMSG;

	dbus_error_init(&err);

	ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &ret_val, DBUS_TYPE_INVALID);
	if (!ret) {
		LOCKD_ERR("no message : [%s:%s]", err.name, err.message);
		ret_val = -EBADMSG;
	}

	dbus_message_unref(msg);
	dbus_error_free(&err);

	LOCKD_DBG("%s-%s : %d", DEVICED_INTERFACE_HALL, METHOD_GET_STATUS, ret_val);
	return ret_val;
}

int lockd_get_lock_state(void)
{
	int idle_lock_state = VCONFKEY_IDLE_UNLOCK;

	if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &idle_lock_state) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY");
		idle_lock_state = VCONFKEY_IDLE_UNLOCK;
	}
	LOCKD_DBG("Get lock state : %d", idle_lock_state);
	return idle_lock_state;
}

static void _lockd_on_changed_receive(void *data, DBusMessage *msg)
{
	DBusError err;
	char *str;
	int response;
	int r;
	int automatic_unlock = 0;

	struct lockd_data *lockd = (struct lockd_data *)data;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	LOCKD_DBG("hall signal is received");

	r = dbus_message_is_signal(msg, DEVICED_INTERFACE_HALL, SIGNAL_HALL_STATE);
	if (!r) {
		LOCKD_ERR("dbus_message_is_signal error");
		return;
	}

	LOCKD_DBG("%s - %s", DEVICED_INTERFACE_HALL, SIGNAL_HALL_STATE);

	dbus_error_init(&err);
	r = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &response, DBUS_TYPE_INVALID);
	if (!r) {
		LOCKD_ERR("dbus_message_get_args error");
		return;
	}

	LOCKD_DBG("receive data : %d", response);
	lockd->hall_status = response;

}

static int _lockd_request_PmQos(int duration)
{
	DBusError err;
	DBusMessage *msg;
	int ret, ret_val;
	char *arr[1];
	char val[32];

	snprintf(val, sizeof(val), "%d", duration);
	arr[0] = val;

	//msg = _lockd_invoke_dbus_method_sync(BUS_NAME, DEVICED_PATH_PMQOS, DEVICED_INTERFACE_PMQOS,
	//		METHOD_PMQOS_NAME, "i", arr);
	msg = _lockd_invoke_dbus_method_async(BUS_NAME, DEVICED_PATH_PMQOS, DEVICED_INTERFACE_PMQOS,
			METHOD_PMQOS_NAME, "i", arr);

	if (!msg)
		return -EBADMSG;

	dbus_error_init(&err);

	ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &ret_val, DBUS_TYPE_INVALID);
	if (!ret) {
		LOCKD_ERR("no message : [%s:%s]", err.name, err.message);
		ret_val = -EBADMSG;
	}

	dbus_message_unref(msg);
	dbus_error_free(&err);

	LOCKD_DBG("%s-%s : %d", DEVICED_INTERFACE_PMQOS, METHOD_PMQOS_NAME, ret_val);
	return ret_val;
}

static void _lockd_delete_alarm(alarm_id_t id, void * user_param)
{
	int ret_val = ALARMMGR_RESULT_SUCCESS;
	LOCKD_DBG("Del alarm_id(%d)", id);
	ret_val = alarmmgr_remove_alarm(id);
	if(ret_val != ALARMMGR_RESULT_SUCCESS) {
		LOCKD_ERR("alarmmgr_enum_alarm_ids() failed");
	}
}

static void _lockd_unset_alarms(void)
{
	LOCKD_DBG("[ == %s == ]", __func__);
	int ret_val = ALARMMGR_RESULT_SUCCESS;
	ret_val = alarmmgr_enum_alarm_ids(_lockd_delete_alarm, NULL);
	if(ret_val != ALARMMGR_RESULT_SUCCESS) {
		LOCKD_ERR("alarmmgr_enum_alarm_ids() failed");
	}
}

static void _lockd_lauch_lockscreen(alarm_id_t alarm_id, void *data)
{
	struct lockd_data *lockd = (struct lockd_data *)data;
	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	int ret_val = ALARMMGR_RESULT_SUCCESS;

	LOCKD_DBG("[ == %s == ], alarm_id(%d)", __func__, alarm_id);

	display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
	lockd_launch_app_lockscreen(lockd);

	if(alarm_id != -1) {
		if(alarm_id != lockd->alarm_id) {
			LOCKD_ERR("alarm ids are different callback->id(%d), lockd->alarm_id(%d)", alarm_id, lockd->alarm_id);
			/* delete all registering alarm*/
			_lockd_unset_alarms();
			lockd->alarm_id = -1;
		} else {
			ret_val = alarmmgr_remove_alarm(alarm_id);
			if(ret_val != ALARMMGR_RESULT_SUCCESS) {
				LOCKD_ERR("alarmmgr_remove_alarm() failed");
				/* delete all registering alarm*/
				_lockd_unset_alarms();
			}
			lockd->alarm_id = -1;
		}
	}
}

static Eina_Bool _lockd_set_alarm(int sec, void *data)
{
	struct lockd_data *lockd = (struct lockd_data *)data;
	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return EINA_FALSE;
	}

	time_t current_time;
	struct tm current_tm;
	alarm_entry_t *alarm_info;
	alarm_id_t alarm_id;
	alarm_date_t alarm_time;
	int ret_val = ALARMMGR_RESULT_SUCCESS;

	/* delete before registering alarm ids */
	if(lockd->alarm_id != -1){
		LOCKD_ERR("ad->alarm_id(%d) deleted", lockd->alarm_id);
		ret_val = alarmmgr_remove_alarm(lockd->alarm_id);
		if(ret_val != ALARMMGR_RESULT_SUCCESS) {
			LOCKD_ERR("alarmmgr_remove_alarm(%d) failed", lockd->alarm_id);
			_lockd_unset_alarms();
		}
		lockd->alarm_id = -1;
	}

	/* set alarm after sec */
	time(&current_time);

	localtime_r(&current_time, &current_tm);

	alarm_info = alarmmgr_create_alarm();
	if(alarm_info == NULL) {
		LOCKD_ERR("alarmmgr_create_alarm() is failed\n");
		return EINA_FALSE;
	}

	alarm_time.year = 0;
	alarm_time.month = 0;
	alarm_time.day = 0;
	alarm_time.hour = current_tm.tm_hour;
	alarm_time.min = current_tm.tm_min;
	alarm_time.sec = current_tm.tm_sec + sec;

	alarmmgr_set_repeat_mode(alarm_info, ALARM_REPEAT_MODE_ONCE, 0);
	alarmmgr_set_time(alarm_info, alarm_time);
	alarmmgr_set_type(alarm_info, ALARM_TYPE_VOLATILE);

	ret_val = alarmmgr_add_alarm_with_localtime(alarm_info, NULL, &alarm_id);
	if(ret_val != ALARMMGR_RESULT_SUCCESS) {
		LOCKD_ERR("alarmmgr_add_alarm_with_localtime() failed");
		alarmmgr_free_alarm(alarm_info) ;
		return EINA_FALSE;
	}

	LOCKD_DBG("alarm id(%d) is set", alarm_id);
	lockd->alarm_id = alarm_id;
	alarmmgr_free_alarm(alarm_info) ;

	return EINA_TRUE;
}


#ifndef FEATURE_LITE
static Eina_Bool _lockd_play_smart_alert_cb(void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	struct lockd_data *lockd = (struct lockd_data *)data;

	feedback_initialize();
	feedback_play_type(FEEDBACK_TYPE_VIBRATION, FEEDBACK_PATTERN_REACTIVE_ALERT);
	//feedback_deinitialize();
	//lockd_stop_sensor(lockd);

	return ECORE_CALLBACK_CANCEL;
}

void _lockd_context_update_cb(int error, context_item_e context, char* context_data, void* user_data, int req_id)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	struct lockd_data *lockd = (struct lockd_data *)user_data;

	if (error == CONTEXT_ERROR_NONE && context == CONTEXT_ITEM_MOVEMENT) {
		int action = -1;
		int ret = 0;
		int calls = 0;
		int messages = 0;

		context_context_data_get_int(context_data, CONTEXT_MOTION_ACTION, &action);

		switch (action) {
		case CONTEXT_MOVEMENT_ACTION:
			ret = vconf_get_int(VCONFKEY_STARTER_MISSED_CALL, &calls);
			if(ret != 0)
				calls = 0;

			ret = vconf_get_int(VCONFKEY_STARTER_UNREAD_MESSAGE, &messages);
			if(ret != 0)
				messages = 0;

			if(calls > 0 || messages > 0) {
				LOCKD_WARN("[ sensor ] SMART ALERT calls = %d messages = %d", calls, messages);
				ecore_timer_add(SMART_ALERT_INTERVAL, _lockd_play_smart_alert_cb, user_data);
			}
			break;
		case CONTEXT_MOVEMENT_NONE:
			break;
		default:
			break;
		}
		context_unset_changed_callback(CONTEXT_ITEM_MOVEMENT);
		free(context_data);
	}
}

static Eina_Bool _lockd_smart_alert_idelr_cb(void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	int val = -1;
	struct lockd_data *lockd = (struct lockd_data *)data;
	if (vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY_PM_STATE");
		return ECORE_CALLBACK_CANCEL;
	}

	if ((val == VCONFKEY_PM_STATE_LCDOFF) || (val == VCONFKEY_PM_STATE_SLEEP)) {
		LOCKD_DBG("LCD OFF ==> smart alert start");

		int ret = 0;
		int is_motion = 0;
		int is_pickup = 0;

		ret = vconf_get_bool(VCONFKEY_SETAPPL_MOTION_ACTIVATION, &is_motion);
		LOCKD_DBG("[ sensor ] ret = %d is_motion = %d", ret, is_motion);
		if(ret == 0 && is_motion == 1) {
			ret = vconf_get_bool(VCONFKEY_SETAPPL_USE_PICK_UP, &is_pickup);
			LOCKD_DBG("[ sensor ] ret = %d is_pickup = %d", ret, is_pickup);
			if(ret == 0 && is_pickup == 1) {
				int ret = 0;
				int calls = 0;
				int messages = 0;
				int req_id, r;

				ret = vconf_get_int(VCONFKEY_STARTER_MISSED_CALL, &calls);
				if(ret != 0)
					calls = 0;

				ret = vconf_get_int(VCONFKEY_STARTER_UNREAD_MESSAGE, &messages);
				if(ret != 0)
					messages = 0;

				if(calls > 0 || messages > 0) {
					r = context_set_changed_callback(CONTEXT_ITEM_MOVEMENT, NULL, _lockd_context_update_cb, lockd, &req_id);
					if (r != CONTEXT_ERROR_NONE) {
						LOCKD_ERR("[ sensor ] context_set_changed_callback fail");
					}
					LOCKD_WARN("[ sensor ] context_set_changed_callback, r=[%d]", r);
				}
			}
		}
	} else {
		LOCKD_DBG("Not LCD OFF ==> cancel smart alert");
	}
	return ECORE_CALLBACK_CANCEL;
}
#endif



static Eina_Bool _lockd_sensor_idelr_cb(void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	int val = -1;
	if (vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY_PM_STATE");
		return ECORE_CALLBACK_CANCEL;
	}

	if ((val == VCONFKEY_PM_STATE_LCDOFF) || (val == VCONFKEY_PM_STATE_SLEEP)) {
		LOCKD_DBG("LCD OFF ==> sensor start");

		int ret = 0;
		int calls = 0;
		int messages = 0;

		ret = vconf_get_int(VCONFKEY_STARTER_MISSED_CALL, &calls);
		if(ret != 0)
			calls = 0;

		ret = vconf_get_int(VCONFKEY_STARTER_UNREAD_MESSAGE, &messages);
		if(ret != 0)
			messages = 0;

		if(calls > 0 || messages > 0) {
			if(lockd_start_sensor(data) == EINA_FALSE) {
				LOCKD_ERR("smart_alert sensor start is failed");
			}
		} else {
			LOCKD_ERR("[ sensor ] sensor is not started calls = %d messages = %d", calls, messages);
		}
	} else {
		LOCKD_DBG("Not LCD OFF ==> cancel sensor start");
	}
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _lockd_alarm_idelr_cb(void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	struct lockd_data *lockd = (struct lockd_data *)data;
	_lockd_unset_alarms();
	lockd->alarm_id = -1;
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _lockd_launch_idelr_cb(void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	struct lockd_data *lockd = (struct lockd_data *)data;
	_lockd_lauch_lockscreen(-1, lockd);
	return ECORE_CALLBACK_CANCEL;
}

static void _lockd_notify_pm_state_cb(keynode_t * node, void *data)
{
	struct lockd_data *lockd = (struct lockd_data *)data;
	int val = -1;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	if (vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY");
		return;
	}

	LOCKD_DBG("[ == %s == ] pm_state(%d)", __func__, val);

	if (val == VCONFKEY_PM_STATE_NORMAL) {
		LOCKD_DBG("LCD ON");
		/* delete all alarm registering */
		LOCKD_DBG("%d", lockd->alarm_id);

		_lockd_unset_alarms();
		lockd->alarm_id = -1;

		if(lockd->lock_type == LOCK_TYPE_NONE) {
			LOCKD_DBG("Lockscreen type is none, unlock");
		}
#ifndef FEATURE_LITE
		context_unset_changed_callback(CONTEXT_ITEM_MOVEMENT);
	} else if (val == VCONFKEY_PM_STATE_LCDOFF) {
		ecore_idler_add(_lockd_smart_alert_idelr_cb, lockd);
#endif
	}
}

static int _show_lock_bg(void)
{
	bundle *b;
	int r = 0;
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	if (menu_daemon_get_volume_pid() <0) {
		LOCKD_DBG("volume is not running");
		return -1;
	}

	b = bundle_create();

	bundle_add(b, SHOW_LOCK, ISTRUE);
	r = aul_launch_app(VOLUME_PKG_NAME, b);
	if(b) {
		bundle_free(b);
	}

	return r;
}

static void _lockd_notify_pm_lcdoff_cb(keynode_t * node, void *data)
{
	struct lockd_data *lockd = (struct lockd_data *)data;
	int val = -1;
	int lcdoff_source = vconf_keynode_get_int(node);
	int accessbility_lock_time = 0;
	int idle_lock_state = VCONFKEY_IDLE_UNLOCK;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	if (lcdoff_source < 0) {
		LOCKD_ERR("Cannot get VCONFKEY, error (%d)", lcdoff_source);
		return;
	}

	LOCKD_DBG("[ %s ] LCD OFF by lcd off source(%d)", __func__, lcdoff_source);

	idle_lock_state = lockd_get_lock_state();

	if(lcdoff_source == VCONFKEY_PM_LCDOFF_BY_POWERKEY) {
		LOCKD_DBG("lcd off by powerkey");
#if (!_FEATURE_LCD_OFF_DBUS) // move to dbus signal handler.
		if (idle_lock_state == VCONFKEY_IDLE_UNLOCK) {
			if (lockd->lock_type == LOCK_TYPE_SECURITY || lockd->lock_type == LOCK_TYPE_NORMAL) {
				display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
				_lockd_request_PmQos(REQUEST_PMQOS_DURATION);
			}
		}
		lockd_launch_app_lockscreen(lockd);
#endif
	} else if (lcdoff_source == 3) {
		LOCKD_DBG("lcd off by proximity sensor..");
		return;
	} else {
		LOCKD_DBG("lcd off by timeout");

		if(idle_lock_state == VCONFKEY_IDLE_LOCK) {
			LOCKD_DBG("VCONFKEY is set(not need to set alarm)");
			return;
		}

		if(lockd->alarm_id != -1) {
			LOCKD_ERR("Alarm is set yet (alarm_id = %d) : do nothing", lockd->alarm_id);
			return;
		}

		if(vconf_get_int(VCONFKEY_SETAPPL_ACCESSIBILITY_LOCK_TIME_INT, &accessbility_lock_time) < 0) {
			LOCKD_ERR("Cannot get VCONFKEY");
			accessbility_lock_time = 0;
		}

		if(accessbility_lock_time == 0) {
			LOCKD_ERR("accessbility_lock_time is 0");
			_lockd_lauch_lockscreen(-1, lockd);
			return;
		} else {
			if(lockd->is_alarm) {
				LOCKD_DBG("set alarm %d sec", accessbility_lock_time);
				if(_lockd_set_alarm(accessbility_lock_time, lockd) != EINA_TRUE) {
					LOCKD_ERR("_lockd_set_alarm() failed");
					_lockd_lauch_lockscreen(-1, lockd);
				}
			}
			else {
				LOCKD_ERR("is_alarm is EINA_FALSE");
				_lockd_lauch_lockscreen(-1, lockd);
			}
		}
	}
}

static void _lockd_notify_time_changed_cb(keynode_t * node, void *data)
{
	int festival_wallpaper = 0;
	LOCKD_DBG("system time chanded!!");

	if(vconf_get_bool(VCONFKEY_LOCKSCREEN_FESTIVAL_WALLPAPER, &festival_wallpaper) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY");
		festival_wallpaper = 0;
	}
	LOCKD_DBG("festival wallpaper : %d", festival_wallpaper);
	if(festival_wallpaper){
		bundle *b;
		b = bundle_create();
		if(b == NULL){
			LOCKD_ERR("bundle create failed.\n");
			return;
		}
		bundle_add(b, "popup_type", "festival");
		bundle_add(b, "festival_type", "festival_create");
		aul_launch_app(WALLPAPER_UI_SERVICE_PKG_NAME, b);
		bundle_free(b);
	}
}

static void _lockd_notify_factory_mode_cb(keynode_t * node, void *data)
{
	int factory_mode = -1;
	LOCKD_DBG("Factory mode Notification!!");

	vconf_get_int(VCONFKEY_TELEPHONY_SIM_FACTORY_MODE, &factory_mode);
	if (factory_mode == VCONFKEY_TELEPHONY_SIM_FACTORYMODE_ON) {
		LOCKD_DBG("Factory mode ON, lock screen can't be launched..!!");
		_lockd_set_lock_state(VCONFKEY_IDLE_UNLOCK);
	}
}

static void
_lockd_notify_lock_state_cb(keynode_t * node, void *data)
{
	LOCKD_DBG("lock state changed!!");

	struct lockd_data *lockd = (struct lockd_data *)data;
	int val = -1;
	int pm_state = -1;
	int is_sync = -1;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	val = lockd_get_lock_state();

	if (val == VCONFKEY_IDLE_UNLOCK) {
		LOCKD_DBG("unlocked..!!");

		/*Phone lock can't be terminated..!! */
		if (phone_lock_pid != 0) {
			LOCKD_ERR("Security lock[pid = %d] is unlocked why???", phone_lock_pid);
			_lockd_set_lock_state(VCONFKEY_IDLE_LOCK);
			return;
		}

		if (vconf_set_bool(VCONFKEY_SAT_NORMAL_PRIORITY_AVAILABLE_BOOL, EINA_FALSE) < 0) {
			LOCKD_ERR("Cannot set VCONFKEY");
		}

		if (lockd->lock_type == LOCK_TYPE_AUTO_LOCK) {
			if (!lockd_get_auto_lock_security()) {
				LOCKD_ERR("Auto lock security is set... why???");
			}
			lockd->lock_app_pid = auto_lock_pid;
			auto_lock_pid = 0;
		}

		if (lockd->lock_app_pid > 0) {

			if (vconf_get_int(VCONFKEY_PM_STATE, &pm_state) < 0) {
				LOCKD_ERR("Cannot get VCONFKEY");
				pm_state = VCONFKEY_PM_STATE_NORMAL;
			}
			LOCKD_DBG("pm_state : [%d]..", pm_state);
			if (pm_state == VCONFKEY_PM_STATE_NORMAL) {
				LOCKD_DBG("terminate lock app..!!");
				lockd_process_mgr_terminate_lock_app(lockd->lock_app_pid, 1);
				if (lockd->lock_type != LOCK_TYPE_NORMAL) {
					_lockd_play_sound(TRUE);
				}
				lockd->lock_app_pid = 0;
			} else {
				vconf_get_int(VCONFKEY_POPSYNC_ACTIVATED_KEY, &is_sync);
				if (is_sync == 1) {
					LOCKD_ERR("POP sync is connected");
					LOCKD_DBG("terminate lock app..!!");
					lockd_process_mgr_terminate_lock_app(lockd->lock_app_pid, 1);
					if (lockd->lock_type != LOCK_TYPE_NORMAL) {
						_lockd_play_sound(TRUE);
					}
					lockd->lock_app_pid = 0;
				} else if (lock_state_available) {
					LOCKD_ERR("can't unlock because of lcd off");
					_lockd_set_lock_state(VCONFKEY_IDLE_LOCK);
				} else {
					LOCKD_ERR("Unlock by exit");
					//we should not re-set lock state because of exit cmd
					lockd_process_mgr_terminate_lock_app(lockd->lock_app_pid, 1);
					lock_state_available = TRUE;
				}
			}
		}
	}else if (val == VCONFKEY_IDLE_LOCK) {
		display_unlock_state(LCD_OFF, PM_SLEEP_MARGIN);
	}
}

static Eina_Bool lockd_set_lock_state_cb(void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	_lockd_set_lock_state(VCONFKEY_IDLE_LOCK);
	return ECORE_CALLBACK_CANCEL;
}

static void
_lockd_notify_phone_lock_verification_cb(keynode_t * node, void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	struct lockd_data *lockd = (struct lockd_data *)data;
	int val = -1;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	if (vconf_get_bool(VCONFKEY_LOCKSCREEN_PHONE_LOCK_VERIFICATION, &val) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY");
		return;
	}

	if (val == TRUE) {
		/* password verified */
		/* lockd_unlock_lockscreen(lockd); */
		lockd_window_mgr_finish_lock(lockd->lockw);
		_lockd_set_lock_state(VCONFKEY_IDLE_UNLOCK);
	}
}

static int lockd_app_dead_cb(int pid, void *data)
{
	LOCKD_DBG("app dead cb call! (pid : %d)", pid);

	struct lockd_data *lockd = (struct lockd_data *)data;

	if (pid == lockd->lock_app_pid && lockd->lock_type == LOCK_TYPE_OTHER) {
		LOCKD_DBG("lock app(pid:%d) is destroyed.", pid);
		lockd_unlock_lockscreen(lockd);
	}
#ifdef FEATURE_LITE
	/* set unlock vconf temporarily in Kiran */
	else if(pid == lockd->lock_app_pid){
		int idle_lock_state = 0;
		LOCKD_DBG("lock app(pid:%d) is destroyed.", pid);
		idle_lock_state = lockd_get_lock_state();
		if(idle_lock_state != VCONFKEY_IDLE_UNLOCK){
			LOCKD_ERR("lock app terminated unexpectedly, and lock state is not unlock");
			lockd_unlock_lockscreen(lockd);
		}

		display_unlock_state(LCD_OFF, PM_SLEEP_MARGIN);
	}
#endif
	menu_daemon_check_dead_signal(pid);

	return 0;
}

static Eina_Bool lockd_app_create_cb(void *data, int type, void *event)
{
	struct lockd_data *lockd = (struct lockd_data *)data;

	if (lockd == NULL) {
		return ECORE_CALLBACK_PASS_ON;
	}
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	if (lockd_window_set_window_effect(lockd->lockw, lockd->lock_app_pid,
				       event) == EINA_TRUE) {
		//FIXME sometimes show cb is not called.
		if(lockd_window_set_window_property(lockd->lockw, lockd->lock_app_pid,
						 event) == EINA_FALSE) {
			LOCKD_ERR("window is not matched..!!");
		}
	}
	return ECORE_CALLBACK_PASS_ON;
}

static Eina_Bool lockd_app_show_cb(void *data, int type, void *event)
{
	struct lockd_data *lockd = (struct lockd_data *)data;

	if (lockd == NULL) {
		return EINA_TRUE;
	}
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	if (lockd_window_set_window_property(lockd->lockw, lockd->lock_app_pid,
					 event)) {
		lockd_window_set_scroll_property(lockd->lockw, lockd->lock_type);
		if (lockd->lock_type > LOCK_TYPE_SECURITY) {
			ecore_idler_add(lockd_set_lock_state_cb, NULL);
		}
	}
	return EINA_FALSE;
}

static Eina_Bool _lockd_play_idelr_cb(void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	_lockd_play_sound(FALSE);
	return ECORE_CALLBACK_CANCEL;
}

static int lockd_launch_app_lockscreen(struct lockd_data *lockd)
{
	LOCKD_DBG("launch app lock screen");

	int call_state = -1, factory_mode = -1, test_mode = -1;
	int r = 0;
	int lcdoff_source = 0;
	int focus_win_pid = 0;
	int pw_type = 0;
	int automatic_unlock = 0;

	//PM LOCK - don't go to sleep
	display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);

	vconf_get_int(VCONFKEY_TELEPHONY_SIM_FACTORY_MODE, &factory_mode);
	if (factory_mode == VCONFKEY_TELEPHONY_SIM_FACTORYMODE_ON) {
		LOCKD_DBG("Factory mode ON, lock screen can't be launched..!!");
		goto launch_out;
	}
	vconf_get_int(VCONFKEY_TESTMODE_SCREEN_LOCK, &test_mode);
	if (test_mode == VCONFKEY_TESTMODE_SCREEN_LOCK_DISABLE) {
		LOCKD_DBG("Test mode ON, lock screen can't be launched..!!");
		goto launch_out;
	}

	if (lockd->is_first_boot == TRUE) {
		if(vconf_get_int(VCONFKEY_PWLOCK_STATE, &pw_type) < 0)
		{
			LOCKD_ERR("vconf_get_int() failed");
			pw_type = VCONFKEY_PWLOCK_RUNNING_UNLOCK;
		}
		if(pw_type == VCONFKEY_PWLOCK_BOOTING_LOCK || pw_type == VCONFKEY_PWLOCK_RUNNING_LOCK) {
			LOCKD_ERR("First boot & pwlock state[%d], lock screen can't be launched..!!", pw_type);
			goto launch_out;
		}
	}

	if (lockd->lock_type == LOCK_TYPE_NONE) {
		LOCKD_DBG("Lock screen type is None..!!");
		goto launch_out;
	}

	/* Get Call state */
	if(vconf_get_int(VCONFKEY_PM_LCDOFF_SOURCE, &lcdoff_source) < 0)
	{
		LOCKD_ERR("Cannot get VCONFKEY");
	}
	focus_win_pid = lockd_window_mgr_get_focus_win_pid();
	vconf_get_int(VCONFKEY_CALL_STATE, &call_state);
	if(lcdoff_source == VCONFKEY_PM_LCDOFF_BY_POWERKEY) {
		if ((call_state != VCONFKEY_CALL_OFF) && (lockd->lock_type != LOCK_TYPE_SECURITY)) {
			if (focus_win_pid >0) {
				if (lockd_process_mgr_check_call(focus_win_pid) == TRUE) {
					LOCKD_DBG("Call is FG => not allow to launch lock screen!!");
					goto launch_out;
				} else {
					LOCKD_DBG("Call is BG => allow to launch lock screen!!");
				}
			}
		}
	} else {
		if ((call_state != VCONFKEY_CALL_OFF) && (lockd->back_to_app == FALSE)) {
			LOCKD_DBG("Current call state(%d) does not allow to launch lock screen.",
			     call_state);
			goto launch_out;
		}
	}

	if ((lockd->lock_type == LOCK_TYPE_NORMAL) || (lockd->lock_type == LOCK_TYPE_OTHER)) {
		if (lockd_process_mgr_check_lock(lockd->lock_app_pid) == TRUE) {
			LOCKD_DBG("Lock Screen App is already running.");
			display_unlock_state(LCD_OFF, PM_SLEEP_MARGIN);
			return 1;
		}

#if _FEATURE_SCOVER
		if (lockd->hall_status == HALL_COVERED_STATUS) {
			vconf_get_bool(VCONFKEY_SETAPPL_ACCESSORY_AUTOMATIC_UNLOCK, &automatic_unlock);
			LOCKD_DBG("hall status : %d, lockd->hall_status : %d, automatic_unlock : %d", HALL_COVERED_STATUS, lockd->hall_status, automatic_unlock);
			if (automatic_unlock == 1) {
				LOCKD_DBG("Don't lauch lockscreen because of non security with scover");
				display_unlock_state(LCD_OFF, PM_SLEEP_MARGIN);
				return 1;
			}
		}
#endif
	}

	switch (lockd->lock_type) {

		case LOCK_TYPE_NORMAL:
			_lockd_set_lock_state(VCONFKEY_IDLE_LAUNCHING_LOCK);

			if (lockd->lock_app_pid > 0) {
				LOCKD_ERR("Lock Screen App is remained, pid[%d].", lockd->lock_app_pid);
				lockd_process_mgr_kill_lock_app(lockd->lock_app_pid);
				lockd->lock_app_pid = 0;
			}

			lockd->lock_app_pid =
			    lockd_process_mgr_start_normal_lock(lockd, lockd_app_dead_cb);
			if (lockd->lock_app_pid < 0) {
				goto launch_out;
			}
			break;

		case LOCK_TYPE_SECURITY:
			vconf_set_bool(VCONFKEY_LOCKSCREEN_PHONE_LOCK_VERIFICATION, FALSE);

			if (phone_lock_pid > 0) {
				LOCKD_DBG("phone lock App is already running.");
				if ((lockd->request_recovery == FALSE) && (lockd->back_to_app == FALSE)) {
					display_unlock_state(LCD_OFF, PM_SLEEP_MARGIN);
					return 1;
				}
			}
			_lockd_set_lock_state(VCONFKEY_IDLE_LAUNCHING_LOCK);

			/* TO DO : Recovery should be checked by EAS interface later */
			/* After getting EAS interface, we should remove lockd->request_recovery */
			if (lockd->request_recovery == TRUE) {
				lockd->phone_lock_app_pid =
				    lockd_process_mgr_start_recovery_lock();
				lockd->request_recovery = FALSE;
			} else if (lockd->back_to_app == TRUE) {
				lockd->phone_lock_app_pid =
				    lockd_process_mgr_start_back_to_app_lock();
				lockd->back_to_app = FALSE;
			} else {
				lockd->phone_lock_app_pid =
				    lockd_process_mgr_start_phone_lock();
			}
			phone_lock_pid = lockd->phone_lock_app_pid;
			LOCKD_DBG("%s, %d, phone_lock_pid = %d", __func__, __LINE__,
				  phone_lock_pid);
			lockd_window_set_phonelock_pid(lockd->lockw, phone_lock_pid);
			if (phone_lock_pid > 0) {
				if(starter_dbus_set_oomadj(phone_lock_pid, OOM_ADJ_VALUE_DEFAULT) < 0){
					LOCKD_ERR("failed to send oom dbus signal");
				}
				if(lockd_process_mgr_check_home(focus_win_pid)) {
					if(vconf_set_bool(VCONFKEY_SAT_NORMAL_PRIORITY_AVAILABLE_BOOL, EINA_TRUE) < 0) {
						LOCKD_ERR("Cannot get VCONFKEY");
					}
				}
			}
			return 1;
			break;

		case LOCK_TYPE_AUTO_LOCK:
			vconf_set_bool(VCONFKEY_LOCKSCREEN_PHONE_LOCK_VERIFICATION, FALSE);

			if (auto_lock_pid > 0) {
				LOCKD_DBG("Auto lock App is already running.");
				if (lockd->back_to_app == FALSE) {
					display_unlock_state(LCD_OFF, PM_SLEEP_MARGIN);
					return 1;
				}
			}
			_lockd_set_lock_state(VCONFKEY_IDLE_LAUNCHING_LOCK);

			if (lockd->back_to_app == TRUE) {
				auto_lock_pid = lockd_process_mgr_start_back_to_app_lock();
				lockd->back_to_app = FALSE;
			} else {
				auto_lock_pid = lockd_process_mgr_start_phone_lock();
			}
			LOCKD_DBG("%s, %d, Auto_lock_pid = %d", __func__, __LINE__, auto_lock_pid);

			if (auto_lock_pid > 0) {
				if(starter_dbus_set_oomadj(auto_lock_pid, OOM_ADJ_VALUE_DEFAULT) < 0){
					LOCKD_ERR("failed to send oom dbus signal");
				}
				if(lockd_process_mgr_check_home(focus_win_pid)) {
					if(vconf_set_bool(VCONFKEY_SAT_NORMAL_PRIORITY_AVAILABLE_BOOL, EINA_TRUE) < 0) {
						LOCKD_ERR("Cannot get VCONFKEY");
					}
				}
			}
			return 1;
			break;

		default:
			if (lockd->lock_app_pid > 0) {
				LOCKD_ERR("Lock Screen App is remained, pid[%d].", lockd->lock_app_pid);
				lockd_process_mgr_kill_lock_app(lockd->lock_app_pid);
				lockd->lock_app_pid = 0;
			}
			if(lcdoff_source == VCONFKEY_PM_LCDOFF_BY_POWERKEY)
				ecore_idler_add(_lockd_play_idelr_cb, NULL);

			lockd->lock_app_pid =
			    lockd_process_mgr_start_lock(lockd, lockd_app_dead_cb,
							 lockd->lock_type);
			if (lockd->lock_app_pid < 0) {
				goto launch_out;
			}
			/* reset window mgr before start win mgr  */
			lockd_window_mgr_finish_lock(lockd->lockw);
			lockd_window_mgr_ready_lock(lockd, lockd->lockw, lockd_app_create_cb,
						    lockd_app_show_cb);
			break;
	}


	if (lockd->lock_app_pid > 0) {
		if(starter_dbus_set_oomadj(lockd->lock_app_pid, OOM_ADJ_VALUE_DEFAULT) < 0){
			LOCKD_ERR("failed to send oom dbus signal");
		}
		if(lockd_process_mgr_check_home(focus_win_pid)) {
			if(vconf_set_bool(VCONFKEY_SAT_NORMAL_PRIORITY_AVAILABLE_BOOL, EINA_TRUE) < 0) {
				LOCKD_ERR("Cannot get VCONFKEY");
			}
		}
	}
	return 1;


 launch_out:
	_lockd_set_lock_state(VCONFKEY_IDLE_UNLOCK);
	display_unlock_state(LCD_OFF, PM_SLEEP_MARGIN);
	return 0;
}

static void lockd_unlock_lockscreen(struct lockd_data *lockd)
{
	LOCKD_DBG("unlock lock screen");
	lockd->lock_app_pid = 0;

	_lockd_set_lock_state(VCONFKEY_IDLE_UNLOCK);
	lockd_window_mgr_finish_lock(lockd->lockw);
}

inline static void lockd_set_sock_option(int fd, int cli)
{
	int size;
	int ret;
	struct timeval tv = { 1, 200 * 1000 };	/* 1.2 sec */

	size = PHLOCK_SOCK_MAXBUFF;
	ret = setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
	if(ret != 0)
		return;
	ret = setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	if(ret != 0)
		return;
	if (cli) {
		ret = setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
		if(ret != 0)
			return;
	}
}

static int lockd_create_sock(void)
{
	struct sockaddr_un saddr;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);	/* support above version 2.6.27 */
	if (fd < 0) {
		if (errno == EINVAL) {
			fd = socket(AF_UNIX, SOCK_STREAM, 0);
			if (fd < 0) {
				LOCKD_ERR
				    ("second chance - socket create error");
				return -1;
			}
		} else {
			LOCKD_ERR("socket error");
			return -1;
		}
	}

	bzero(&saddr, sizeof(saddr));
	saddr.sun_family = AF_UNIX;

	strncpy(saddr.sun_path, PHLOCK_SOCK_PREFIX, sizeof(saddr.sun_path)-1);
	saddr.sun_path[strlen(PHLOCK_SOCK_PREFIX)] = 0;

	unlink(saddr.sun_path);

	if (bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		LOCKD_ERR("bind error");
		close(fd);
		return -1;
	}

	if (chmod(saddr.sun_path, (S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
		/* Flawfinder: ignore */
		LOCKD_ERR("failed to change the socket permission");
		close(fd);
		return -1;
	}

	lockd_set_sock_option(fd, 0);

	if (listen(fd, 10) == -1) {
		LOCKD_ERR("listen error");
		close(fd);
		return -1;
	}

	return fd;
}

static gboolean lockd_glib_check(GSource * src)
{
	GSList *fd_list;
	GPollFD *tmp;

	fd_list = src->poll_fds;
	do {
		tmp = (GPollFD *) fd_list->data;
		if ((tmp->revents & (POLLIN | POLLPRI)))
			return TRUE;
		fd_list = fd_list->next;
	} while (fd_list);

	return FALSE;
}

static char *lockd_read_cmdline_from_proc(int pid)
{
	int memsize = 32;
	char path[32];
	char *cmdline = NULL, *tempptr = NULL;
	FILE *fp = NULL;

	snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

	fp = fopen(path, "r");
	if (fp == NULL) {
		LOCKD_ERR("Cannot open cmdline on pid[%d]", pid);
		return NULL;
	}

	cmdline = malloc(32);
	if (cmdline == NULL) {
		LOCKD_ERR("%s", "Out of memory");
		fclose(fp);
		return NULL;
	}

	bzero(cmdline, memsize);
	if (fgets(cmdline, 32, fp) == NULL) {
		LOCKD_ERR("%s", "Cannot read cmdline");
		free(cmdline);
		fclose(fp);
		return NULL;
	}

	while (cmdline[memsize - 2] != 0) {
		cmdline[memsize - 1] = (char)fgetc(fp);
		tempptr = realloc(cmdline, memsize + 32);
		if (tempptr == NULL) {
			fclose(fp);
			LOCKD_ERR("%s", "Out of memory");
			return NULL;
		}
		cmdline = tempptr;
		bzero(cmdline + memsize, 32);
		fgets(cmdline + memsize, 32, fp);
		memsize += 32;
	}

	if (fp != NULL)
		fclose(fp);
	return cmdline;
}

static int lockd_sock_handler(void *data)
{
	int cl;
	int len;
	int sun_size;
	int clifd = -1;
	char cmd[PHLOCK_SOCK_MAXBUFF];
	char *cmdline = NULL;
	int fd = -1;
	int recovery_state = -1;
	GPollFD *gpollfd;

	struct ucred cr;
	struct sockaddr_un lockd_addr;
	struct lockd_data *lockd = (struct lockd_data *)data;

	if ((lockd == NULL) || (lockd->gpollfd == NULL)) {
		LOCKD_ERR("lockd->gpollfd is NULL");
		return -1;
	}
	gpollfd = (GPollFD *)lockd->gpollfd;
	fd = gpollfd->fd;

	cl = sizeof(cr);
	sun_size = sizeof(struct sockaddr_un);

	if ((clifd =
	     accept(fd, (struct sockaddr *)&lockd_addr,
		    (socklen_t *) & sun_size)) == -1) {
		if (errno != EINTR)
			LOCKD_ERR("accept error");
		return -1;
	}

	if (getsockopt(clifd, SOL_SOCKET, SO_PEERCRED, &cr, (socklen_t *) & cl)
	    < 0) {
		LOCKD_ERR("peer information error");
		close(clifd);
		return -1;
	}
	LOCKD_ERR("Peer's pid=%d, uid=%d, gid=%d\n", cr.pid, cr.uid, cr.gid);

	memset(cmd, 0, PHLOCK_SOCK_MAXBUFF);

	lockd_set_sock_option(clifd, 1);

	/* receive single packet from socket */
	len = recv(clifd, cmd, PHLOCK_SOCK_MAXBUFF, 0);

	if (len < 0) {
		LOCKD_ERR("recv error, read number is less than zero");
		close(clifd);
		return -1;
	}

	cmd[PHLOCK_SOCK_MAXBUFF - 1] = '\0';

	LOCKD_ERR("cmd %s", cmd);

	/* Read command line of the PID from proc fs */
	cmdline = lockd_read_cmdline_from_proc(cr.pid);
	if (cmdline == NULL) {
		/* It's weired. no file in proc file system, */
		LOCKD_ERR("Error on opening /proc/%d/cmdline", cr.pid);
		close(clifd);
		return -1;
	}
	LOCKD_SECURE_ERR("/proc/%d/cmdline : %s", cr.pid, cmdline);

	if (!strncmp(cmd, PHLOCK_UNLOCK_CMD, strlen(cmd))) {
	    if (!strncmp(cmdline, PHLOCK_APP_CMDLINE, strlen(cmdline))) {
			if (lockd->lock_type == LOCK_TYPE_SECURITY) {
				LOCKD_ERR("phone_lock_pid : %d vs cr.pid : %d", phone_lock_pid, cr.pid);
				if (phone_lock_pid == cr.pid) {
					LOCKD_SECURE_ERR("pid [%d] %s is verified, unlock..!!\n", cr.pid,
						  cmdline);
					lockd_process_mgr_terminate_phone_lock(phone_lock_pid);
					phone_lock_pid = 0;
					_lockd_play_sound(TRUE);
					vconf_set_bool(VCONFKEY_LOCKSCREEN_PHONE_LOCK_VERIFICATION, TRUE);
				}
			} else if (lockd->lock_type == LOCK_TYPE_AUTO_LOCK) {
				LOCKD_ERR("auto_lock_pid : %d vs cr.pid : %d", auto_lock_pid, cr.pid);
				if (auto_lock_pid == cr.pid) {
					LOCKD_SECURE_ERR("pid [%d] %s is verified, unlock..!!\n", cr.pid,
						  cmdline);
					lockd_process_mgr_terminate_phone_lock(auto_lock_pid);
					auto_lock_pid = 0;
					_lockd_play_sound(TRUE);
					lockd_window_mgr_finish_lock(lockd->lockw);
					_lockd_set_lock_state(VCONFKEY_IDLE_UNLOCK);
				}
			}
	    }
#if 0
		else if (!strncmp(cmd, LOCK_EXIT_CMD, strlen(cmd))) {
		    LOCKD_ERR("[%s] for exit", LOCK_EXIT_CMD);
			//we should not re-set lock state because of exit cmd for scover view.
			lock_state_available = FALSE;
			_lockd_set_lock_state(VCONFKEY_IDLE_UNLOCK);
	    }
#endif
	} else if (!strncmp(cmd, LOCK_EXIT_CMD, strlen(cmd))) {
			if ((!strncmp(cmdline, PHLOCK_APP_CMDLINE, strlen(cmdline))) ||
				(!strncmp(cmdline, VOLUNE_APP_CMDLINE, strlen(cmdline)))) {
				//we should not re-set lock state because of exit cmd for scover view.
			    LOCKD_ERR("[%s] for exit", LOCK_EXIT_CMD);
				lock_state_available = FALSE;
				_lockd_set_lock_state(VCONFKEY_IDLE_UNLOCK);
			}
    } else if ((!strncmp(cmd, PHLOCK_UNLOCK_RESET_CMD, strlen(cmd)))
		&& (!strncmp(cmdline, OMA_DRM_AGENT_CMDLINE, strlen(cmdline)))) {
		LOCKD_SECURE_ERR("%s request %s \n", cmdline, cmd);
		if(phone_lock_pid > 0) {
			LOCKD_SECURE_ERR("pid [%d] %s is verified, unlock..!!\n", cr.pid,
				  cmdline);
			lockd_process_mgr_terminate_phone_lock(phone_lock_pid);
			phone_lock_pid = 0;
			_lockd_play_sound(TRUE);
			vconf_set_bool(VCONFKEY_LOCKSCREEN_PHONE_LOCK_VERIFICATION, TRUE);
			// reset lockscreen type
			if (vconf_set_int(VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, SETTING_SCREEN_LOCK_TYPE_SWIPE) < 0) {
				LOCKD_ERR("%s set to %d is failed. error \n",
					VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, SETTING_SCREEN_LOCK_TYPE_SWIPE);
			}
		}
	} else if (!strncmp(cmd, PHLOCK_LAUNCH_CMD, strlen(cmd))) {
		if (!strncmp(cmdline, MDM_APP_CMDLINE, strlen(cmdline))) {
			LOCKD_ERR("%s request %s \n", cmdline, cmd);
			if (vconf_get_int(VCONFKEY_EAS_RECOVERY_MODE, &recovery_state) < 0) {
				LOCKD_ERR("Cannot get vconfkey", VCONFKEY_EAS_RECOVERY_MODE);
				lockd->request_recovery = FALSE;
			} else if (recovery_state == 1) {
				LOCKD_ERR("recovery mode : %d \n", recovery_state);
				lockd->request_recovery = TRUE;
			} else {
				lockd->request_recovery = FALSE;
			}
		}
		if (lockd->lock_type == LOCK_TYPE_SECURITY) {
			lockd_launch_app_lockscreen(lockd);
		} else {
			lockd->request_recovery = FALSE;
		}
	} else if (!strncmp(cmd, HOME_RAISE_CMD, strlen(cmd))) {
		if ((!strncmp(cmdline, VOICE_CALL_APP_CMDLINE, strlen(cmdline)))
			|| (!strncmp(cmdline, VIDEO_CALL_APP_CMDLINE, strlen(cmdline)))) {
			LOCKD_SECURE_ERR("%s request %s \n", cmdline, cmd);
			menu_daemon_open_homescreen(NULL);
		}
	} else if (!strncmp(cmd, LOCK_SHOW_CMD, strlen(cmd))) {
		if ((!strncmp(cmdline, VOICE_CALL_APP_CMDLINE, strlen(cmdline)))
			|| (!strncmp(cmdline, VIDEO_CALL_APP_CMDLINE, strlen(cmdline)))) {
			LOCKD_SECURE_ERR("%s request %s \n", cmdline, cmd);
			lockd->back_to_app = TRUE;
		}
		if ((lockd->lock_type == LOCK_TYPE_SECURITY)
			|| ((lockd->lock_type == LOCK_TYPE_AUTO_LOCK) && (lockd_get_auto_lock_security()))){
			lockd_launch_app_lockscreen(lockd);
		} else {
			lockd->back_to_app = FALSE;
		}
	} else if (!strncmp(cmd, LOCK_LAUNCH_CMD, strlen(cmd))) {
		LOCKD_SECURE_ERR("%s request %s \n", cmdline, cmd);
		if (!strncmp(cmdline, VOICE_CALL_APP_CMDLINE, strlen(cmdline))) {
			lockd_launch_app_lockscreen(lockd);
		}
	}

	if(cmdline != NULL) {
		free(cmdline);
		cmdline = NULL;
	}

	close(clifd);
	return 0;
}

static gboolean lockd_glib_handler(gpointer data)
{
	if (lockd_sock_handler(data) < 0) {
		LOCKD_ERR("lockd_sock_handler is failed..!!");
	}
	return TRUE;
}

static gboolean lockd_glib_dispatch(GSource * src, GSourceFunc callback,
				    gpointer data)
{
	callback(data);
	return TRUE;
}

static gboolean lockd_glib_prepare(GSource * src, gint * timeout)
{
	return FALSE;
}

static GSourceFuncs funcs = {
	.prepare = lockd_glib_prepare,
	.check = lockd_glib_check,
	.dispatch = lockd_glib_dispatch,
	.finalize = NULL
};

static int lockd_init_sock(struct lockd_data *lockd)
{
	int fd;
	GPollFD *gpollfd;
	GSource *src;
	int ret;

	fd = lockd_create_sock();
	if (fd < 0) {
		LOCKD_ERR("lock daemon create sock failed..!!");
	}

	src = g_source_new(&funcs, sizeof(GSource));

	gpollfd = (GPollFD *) g_malloc(sizeof(GPollFD));
	gpollfd->events = POLLIN;
	gpollfd->fd = fd;

	lockd->gpollfd = gpollfd;

	g_source_add_poll(src, lockd->gpollfd);
	g_source_set_callback(src, (GSourceFunc) lockd_glib_handler,
			      (gpointer) lockd, NULL);
	g_source_set_priority(src, G_PRIORITY_LOW);

	ret = g_source_attach(src, NULL);
	if (ret == 0)
		return -1;

	g_source_unref(src);

	return 0;
}




static void _lockd_notify_bt_device_connected_status_cb(keynode_t * node, void *data)
{
	int is_connected = 0;
	struct lockd_data *lockd = (struct lockd_data *)data;

	vconf_get_bool(VCONFKEY_SETAPPL_AUTO_LOCK_DEVICE_STATUS_BOOL , &is_connected);
	LOCKD_WARN("BT connect status is changed to [%d]", is_connected);

	if (is_connected) {
		if (lockd_start_bt_monitor() < 0) {
			LOCKD_ERR("Fail starter BT monitor");
		}
	} else {
		lockd_stop_bt_monitor();
		lockd_change_security_auto_lock(is_connected);
	}
}


static void _lockd_notify_lock_type_cb(keynode_t * node, void *data)
{
	int lock_type = -1;
	struct lockd_data *lockd = (struct lockd_data *)data;
	LOCKD_DBG("lock type is changed..!!");

	lock_type = lockd_get_lock_type();

	if ((lockd->lock_type != LOCK_TYPE_AUTO_LOCK) && (lock_type == LOCK_TYPE_AUTO_LOCK)) {

		int is_connected = 0;

		LOCKD_WARN("Auto Lock is set ON..!!");

		vconf_get_bool(VCONFKEY_SETAPPL_AUTO_LOCK_DEVICE_STATUS_BOOL , &is_connected);
		LOCKD_WARN("BT connect status is [%d]", is_connected);

		if (is_connected) {
			if (lockd_start_bt_monitor() < 0) {
				LOCKD_ERR("Fail starter BT monitor");
			}
		} else {
			lockd_change_security_auto_lock(is_connected);
		}

		if (vconf_notify_key_changed
			(VCONFKEY_SETAPPL_AUTO_LOCK_DEVICE_STATUS_BOOL, _lockd_notify_bt_device_connected_status_cb, lockd) != 0) {
			LOCKD_ERR("Fail vconf_notify_key_changed : VCONFKEY");
		}
	} else if ((lockd->lock_type == LOCK_TYPE_AUTO_LOCK) && (lock_type != LOCK_TYPE_AUTO_LOCK)) {

		LOCKD_WARN("Auto Lock is set OFF..!!");

		lockd_stop_bt_monitor();

		if (vconf_ignore_key_changed
		    (VCONFKEY_SETAPPL_AUTO_LOCK_DEVICE_STATUS_BOOL, _lockd_notify_bt_device_connected_status_cb) != 0) {
			LOCKD_ERR("Fail to unregister");
		}
	}
	lockd->lock_type = lock_type;
}

static void lockd_init_vconf(struct lockd_data *lockd)
{
	int val = -1;

	if (vconf_notify_key_changed
	    (VCONFKEY_PM_STATE, _lockd_notify_pm_state_cb, lockd) != 0) {
		LOCKD_ERR("Fail vconf_notify_key_changed : VCONFKEY");
	}

	if (vconf_notify_key_changed
	    (VCONFKEY_PM_LCDOFF_SOURCE, _lockd_notify_pm_lcdoff_cb, lockd) != 0) {
		LOCKD_ERR("Fail vconf_notify_key_changed : VCONFKEY");
	}

	if (vconf_notify_key_changed
	    (VCONFKEY_TELEPHONY_SIM_FACTORY_MODE, _lockd_notify_factory_mode_cb, NULL) != 0) {
		LOCKD_ERR("Fail vconf_notify_key_changed : VCONFKEY");
	}
#if 0
	if (vconf_notify_key_changed
		(VCONFKEY_SYSTEM_TIME_CHANGED, _lockd_notify_time_changed_cb, NULL) != 0) {
		LOCKD_ERR("Fail vconf_notify_key_changed : VCONFKEY");
	}
#endif
	if (vconf_notify_key_changed
	    (VCONFKEY_SETAPPL_SCREEN_LOCK_TYPE_INT, _lockd_notify_lock_type_cb, lockd) != 0) {
		LOCKD_ERR("Fail vconf_notify_key_changed : VCONFKEY");
	}

	if (vconf_notify_key_changed
	    (VCONFKEY_LOCKSCREEN_PHONE_LOCK_VERIFICATION,
	     _lockd_notify_phone_lock_verification_cb, lockd) != 0) {
		if (vconf_get_bool(VCONFKEY_LOCKSCREEN_PHONE_LOCK_VERIFICATION, &val) < 0) {
			LOCKD_ERR
			    ("Cannot get VCONFKEY");
			vconf_set_bool(VCONFKEY_LOCKSCREEN_PHONE_LOCK_VERIFICATION, 0);
			if (vconf_notify_key_changed
			    (VCONFKEY_LOCKSCREEN_PHONE_LOCK_VERIFICATION,
			     _lockd_notify_phone_lock_verification_cb,
			     lockd) != 0) {
				LOCKD_ERR
				    ("Fail vconf_notify_key_changed " );
			}
		} else {
			LOCKD_ERR
			    ("Fail vconf_notify_key_changed " );
		}
	}

	if (vconf_notify_key_changed
	    (VCONFKEY_IDLE_LOCK_STATE,
	     _lockd_notify_lock_state_cb,
	     lockd) != 0) {
		LOCKD_ERR
		    ("[Error] vconf notify : lock state");
	}
}


static Eina_Bool lockd_init_alarm(void *data)
{
	struct lockd_data *lockd = (struct lockd_data *)data;

	if(!lockd) {
		LOCKD_ERR("parameter is NULL");
		return EINA_FALSE;
	}

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	int ret_val = 0;
	/* alarm id initialize */
	lockd->alarm_id = -1;

	g_type_init();
	ret_val = alarmmgr_init(PACKAGE_NAME);
	if(ret_val != ALARMMGR_RESULT_SUCCESS) {
		LOCKD_ERR("alarmmgr_init() failed(%d)", ret_val);
		return EINA_FALSE;
	}
	ret_val = alarmmgr_set_cb((alarm_cb_t)_lockd_lauch_lockscreen, lockd);
	if(ret_val != ALARMMGR_RESULT_SUCCESS) {
		LOCKD_ERR("alarmmgr_init() failed");
		return EINA_FALSE;
	}

	LOCKD_DBG("alarm init success");
	return EINA_TRUE;
}

static int lockd_init_hall_signal(void *data)
{
	E_DBus_Connection *conn;
	E_DBus_Signal_Handler *handler;

	struct lockd_data *lockd = (struct lockd_data *)data;
	if(!lockd) {
		LOCKD_ERR("parameter is NULL");
		return EINA_FALSE;
	}

	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (conn == NULL) {
		LOCKD_ERR("e_dbus_bus_get error");
		return -1;
	}
	lockd->hall_conn = conn;

#if 0
	int r;
	r = e_dbus_request_name(conn, BUS_NAME, 0, NULL, NULL);
	if (!r) {
		LOCKD_ERR("e_dbus_request_name erro");
		return -1;
	}
#endif

	handler = e_dbus_signal_handler_add(conn, NULL, DEVICED_PATH_HALL,
								DEVICED_INTERFACE_HALL, SIGNAL_HALL_STATE,
								_lockd_on_changed_receive, lockd);
	if (handler == NULL) {
		LOCKD_ERR("e_dbus_signal_handler_add error");
		return -1;
	}
	lockd->hall_handler = handler;
	/* S-Cover Lock : Signal <!-- END --> */
}



static Eina_Bool _lockd_launch_lockscreen_by_pwr_key(void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	int idle_lock_state = VCONFKEY_IDLE_UNLOCK;
	struct lockd_data *lockd = (struct lockd_data *)data;

	LOCKD_DBG("lcd off by powerkey");

	if (idle_lock_state == VCONFKEY_IDLE_UNLOCK) {
		if (lockd->lock_type == LOCK_TYPE_SECURITY || lockd->lock_type == LOCK_TYPE_NORMAL) {
			display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
			_lockd_request_PmQos(REQUEST_PMQOS_DURATION);
		}
	}
	lockd_launch_app_lockscreen(lockd);
	return ECORE_CALLBACK_CANCEL;
}


static void _lockd_receive_lcd_off(void *data, DBusMessage *msg)
{
	DBusError err;
	int r;
	int idle_lock_state = VCONFKEY_IDLE_UNLOCK;

	struct lockd_data *lockd = (struct lockd_data *)data;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	LOCKD_DBG("lcd off signal is received");

	r = dbus_message_is_signal(msg, DEVICED_INTERFACE_LCD_OFF, METHOD_LCD_OFF_NAME);
	if (!r) {
		LOCKD_ERR("dbus_message_is_signal error");
		return;
	}

	LOCKD_DBG("%s - %s", DEVICED_INTERFACE_LCD_OFF, METHOD_LCD_OFF_NAME);

#if 0 //need to check if lockscreen is displayed before lcd off.
	idle_lock_state = lockd_get_lock_state();

	LOCKD_DBG("lcd off by powerkey");

	if (idle_lock_state == VCONFKEY_IDLE_UNLOCK) {
		if (lockd->lock_type == LOCK_TYPE_SECURITY || lockd->lock_type == LOCK_TYPE_NORMAL) {
			_lockd_request_PmQos(REQUEST_PMQOS_DURATION);
			display_lock_state(LCD_OFF, STAY_CUR_STATE, 0);
		}
	}
	lockd_launch_app_lockscreen(lockd);
#else
	ecore_idler_add(_lockd_launch_lockscreen_by_pwr_key, lockd);
#endif
}


static int lockd_init_lcd_off_signal(void *data)
{
	E_DBus_Connection *conn;
	E_DBus_Signal_Handler *handler;

	struct lockd_data *lockd = (struct lockd_data *)data;
	if(!lockd) {
		LOCKD_ERR("parameter is NULL");
		return EINA_FALSE;
	}

	conn = e_dbus_bus_get(DBUS_BUS_SYSTEM);
	if (conn == NULL) {
		LOCKD_ERR("e_dbus_bus_get error");
		return -1;
	}
	lockd->display_conn = conn;

	handler = e_dbus_signal_handler_add(conn, NULL, DEVICED_PATH_LCD_OFF,
								DEVICED_INTERFACE_LCD_OFF, METHOD_LCD_OFF_NAME,
								_lockd_receive_lcd_off, lockd);
	if (handler == NULL) {
		LOCKD_ERR("e_dbus_signal_handler_add error");
		return -1;
	}
	lockd->display_handler = handler;
}

void lockd_check_vconf_festival_wallpaper(void *data){
	struct lockd_data *lockd = (struct lockd_data *)data;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}
	int festival_wallpaper = 0;

	if(vconf_get_bool(VCONFKEY_LOCKSCREEN_FESTIVAL_WALLPAPER, &festival_wallpaper) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY");
		festival_wallpaper = 0;
	}
	LOCKD_DBG("festival wallpaper : %d", festival_wallpaper);
	if(festival_wallpaper){
		bundle *b;
		b = bundle_create();
		if(b == NULL){
			LOCKD_ERR("bundle create failed.\n");
			return;
		}
		bundle_add(b, "popup_type", "festival");
		bundle_add(b, "festival_type", "festival_create");
		aul_launch_app(WALLPAPER_UI_SERVICE_PKG_NAME, b);
		bundle_free(b);
	}

}

static void lockd_start_lock_daemon(void *data)
{
	struct lockd_data *lockd = NULL;
	int r = 0;

	lockd = (struct lockd_data *)data;

	if (!lockd) {
		return;
	}

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	/*init and check hall status*/
	e_dbus_init();


#if _FEATURE_SCOVER
	lockd->hall_status = -1;

	lockd->hall_status = lockd_get_hall_status();
	LOCKD_DBG(" >>> hall_status : %d", lockd->hall_status);

	if (lockd_init_hall_signal(lockd) < 0) {
		LOCKD_ERR("Hall signal can't be used.");
		lockd->hall_status = -1;
	}
#endif

#if _FEATURE_LCD_OFF_DBUS
	if (lockd_init_lcd_off_signal(lockd) < 0) {
		LOCKD_ERR("LCD off signal can't be used.");
		lockd->hall_status = -1;
	}
#endif

	/*init sensor for smart alert*/
	//lockd->is_sensor = lockd_init_sensor(lockd);

	/* register vconf notification */
	lockd_init_vconf(lockd);
	lockd_process_mgr_init();
	/* init alarm manager */
	lockd_check_vconf_festival_wallpaper(lockd);

	lockd->is_alarm = lockd_init_alarm(lockd);

//	lockd_set_festival_wallpaper_alarm(lockd);

	/* Initialize socket */
	r = lockd_init_sock(lockd);
	if (r < 0) {
		LOCKD_ERR("lockd init socket failed: %d", r);
	}

	/* Create internal 1x1 window */
	lockd->lockw = lockd_window_init();

	aul_listen_app_dead_signal(lockd_app_dead_cb, data);

#if 0 // for booting time
	if(feedback_initialize() != FEEDBACK_ERROR_NONE) {
		LOCKD_ERR("[CAUTION][ERROR] feedback_initialize() is failed..!!");
	}
#endif
}

int start_lock_daemon(int launch_lock, int is_first_boot)
{
	struct lockd_data *lockd = NULL;
	int recovery_state = -1;
	int first_boot = 0;
	int ret = 0;

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	lockd = (struct lockd_data *)malloc(sizeof(struct lockd_data));
	memset(lockd, 0x0, sizeof(struct lockd_data));
	lockd_start_lock_daemon(lockd);

	lockd->lock_type = lockd_get_lock_type();
	lockd->back_to_app = FALSE;

	if (lockd->lock_type == LOCK_TYPE_NONE)
		return -1;
	if (lockd->lock_type == LOCK_TYPE_SECURITY) {
		if (vconf_get_int(VCONFKEY_EAS_RECOVERY_MODE, &recovery_state) < 0) {
			LOCKD_ERR("Cannot get vconfkey" );
			lockd->request_recovery = FALSE;
		} else if (recovery_state == 1) {
			LOCKD_DBG("recovery mode : %d \n", recovery_state);
			lockd->request_recovery = TRUE;
		} else {
			lockd->request_recovery = FALSE;
		}
	} else {
		lockd->request_recovery = FALSE;
	}
	if (lockd->lock_type == LOCK_TYPE_AUTO_LOCK) {
		int is_connected = 0;

		vconf_get_bool(VCONFKEY_SETAPPL_AUTO_LOCK_DEVICE_STATUS_BOOL , &is_connected);
		LOCKD_WARN("BT connect status is [%d]", is_connected);

		if (is_connected) {
			if (lockd_start_bt_monitor() < 0) {
				LOCKD_ERR("Fail starter BT monitor");
			}
		} else {
			lockd_change_security_auto_lock(is_connected);
		}

		if (vconf_notify_key_changed
			(VCONFKEY_SETAPPL_AUTO_LOCK_DEVICE_STATUS_BOOL, _lockd_notify_bt_device_connected_status_cb, lockd) != 0) {
			LOCKD_ERR("Fail vconf_notify_key_changed : VCONFKEY");
		}
	}

	vconf_set_bool(VCONFKEY_SAT_NORMAL_PRIORITY_AVAILABLE_BOOL, EINA_TRUE);

	if (launch_lock == FALSE) {
		LOCKD_ERR("Don't launch lockscreen..");
		return 0;
	}

	lockd->is_first_boot = is_first_boot;
	if (lockd->is_first_boot == TRUE){
		LOCKD_ERR("first_boot : %d \n", lockd->is_first_boot);
		return 0;
	}

	ret = lockd_launch_app_lockscreen(lockd);

	if(feedback_initialize() != FEEDBACK_ERROR_NONE) {
		LOCKD_ERR("[CAUTION][ERROR] feedback_initialize() is failed..!!");
	}

	return ret;
}
