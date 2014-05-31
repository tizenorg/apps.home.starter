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

#include <Elementary.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <poll.h>
#include <errno.h>
#include <string.h>

#include <vconf.h>
#include <signal.h>
#include <app.h>
#include <alarm.h>
#include <feedback.h>

#include "starter.h"
#include "starter-util.h"
#include "lockd-debug.h"

#include "util.h"

static int __alarm_delete_cb(alarm_id_t id, void * user_param)
{
	int ret = ALARMMGR_RESULT_SUCCESS;

	ret = alarmmgr_remove_alarm(id);
	if(ret != ALARMMGR_RESULT_SUCCESS) {
		_ERR("alarmmgr_enum_alarm_ids() failed");
	}

	return 0;
}

static void _alarm_unset(void *data)
{
	int ret = ALARMMGR_RESULT_SUCCESS;

	struct appdata *ad = data;
	ret_if(ad == NULL);

	if(ad->alarm_id != -1){
		_DBG("try to delete alarm_id(%d)", ad->alarm_id);
		ret = alarmmgr_remove_alarm(ad->alarm_id);
		if(ret != ALARMMGR_RESULT_SUCCESS) {
			ret = alarmmgr_enum_alarm_ids(__alarm_delete_cb, NULL);
			if(ret != ALARMMGR_RESULT_SUCCESS) {
				_ERR("alarmmgr_enum_alarm_ids() failed");
			}
		}
		ad->alarm_id = -1;
	}
}

static int _alarm_set(void *data)
{
	int ret = ALARMMGR_RESULT_SUCCESS;
	time_t current_time;
	struct tm current_tm;
	alarm_entry_t *alarm_info = NULL;
	alarm_id_t alarm_id;
	alarm_date_t alarm_time;
	struct appdata *ad = data;
	retv_if(ad == NULL, -1);

	/* delete before registering alarm ids */
	_alarm_unset(ad);

	time(&current_time);

	/* alarm revision */
	current_time += 3600;	// +1 hour

	localtime_r(&current_time, &current_tm);

	alarm_info = alarmmgr_create_alarm();
	if(alarm_info == NULL) {
		_ERR("alarmmgr_create_alarm() is failed\n");
		return -1;
	}

	alarm_time.year = current_tm.tm_year;
	alarm_time.month = current_tm.tm_mon;
	alarm_time.day = current_tm.tm_mday;
	alarm_time.hour = current_tm.tm_hour;
	alarm_time.min = 0;
	alarm_time.sec = 0;

	//alarmmgr_set_repeat_mode(alarm_info, ALARM_REPEAT_MODE_ONCE, 0);
	ret = alarmmgr_set_repeat_mode(alarm_info, ALARM_REPEAT_MODE_REPEAT, 60*60);
	if(ret != ALARMMGR_RESULT_SUCCESS) {
		_ERR("alarmmgr_set_repeat_mode() failed");
		alarmmgr_free_alarm(alarm_info) ;
		return -1;
	}
	alarmmgr_set_time(alarm_info, alarm_time);
	alarmmgr_set_type(alarm_info, ALARM_TYPE_VOLATILE);

	ret = alarmmgr_add_alarm_with_localtime(alarm_info, NULL, &alarm_id);
	if(ret != ALARMMGR_RESULT_SUCCESS) {
		_ERR("alarmmgr_add_alarm_with_localtime() failed");
		alarmmgr_free_alarm(alarm_info) ;
		return -1;
	}

	ad->alarm_id = alarm_id;
	alarmmgr_free_alarm(alarm_info);

	return 0;
}

static int __alarm_cb(alarm_id_t alarm_id, void *data)
{
	_DBG("hourly_alert alarm callback called");

	feedback_initialize();
	feedback_play(FEEDBACK_PATTERN_HOURLY_ALERT);
	feedback_deinitialize();

	return 0;
}

static int _alarm_init(void *data)
{
	int ret = 0;

	struct appdata *ad = data;
	retv_if(ad == NULL, -1);

	g_type_init();
	ret = alarmmgr_init("starter");
	retv_if(ret<0, -1);

	ret = alarmmgr_set_cb(__alarm_cb, data);
	retv_if(ret<0, -1);

	ad->alarm_id = -1;

	return 0;
}

static void _alarm_fini(void *data)
{
	_alarm_unset(data);
	alarmmgr_fini();
}

static Eina_Bool _register_hourly_alert_alarm(struct appdata *ad)
{
	int ret = 0;

	if(!ad) {
		_ERR("parameter is NULL");
		return EINA_FALSE;
	}

	//alarmmgr_fini();

	ret = _alarm_init(ad);
	if(ret<0) {
		_ERR("_alarm_init() failed");
		return EINA_FALSE;
	}

	_alarm_set(ad);

	return EINA_TRUE;

}

static int _unregister_hourly_alert_alarm(struct appdata *ad)
{
	_alarm_fini(ad);

	return 0;
}


static void _hourly_alert_changed_cb(keynode_t* node, void *data)
{
	int hourly_alert = -1;
	struct appdata *ad = (struct appdata *)data;

	_DBG("%s, %d", __func__, __LINE__);

	if (node) {
		hourly_alert = vconf_keynode_get_bool(node);
	} else {
		if (vconf_get_int(VCONFKEY_SETAPPL_HOURLY_ALERT_BOOL, &hourly_alert) < 0) {
			_ERR("Failed to get %s", VCONFKEY_SETAPPL_HOURLY_ALERT_BOOL);
			return;
		}
	}

	if (hourly_alert == TRUE) {
		_ERR("hourly_alert is set");
		_register_hourly_alert_alarm(ad);
	} else {
		_ERR("hourly_alert is unset");
		_unregister_hourly_alert_alarm(ad);
	}

}

static void _hourly_system_time_changed_cb(keynode_t *node, void *data)
{
	struct appdata *ad = data;
	ret_if(ad == NULL);

	_DBG("%s, %d", __func__, __LINE__);

	/* unset existing alarms and set new alarm */
	_alarm_set(ad);
}

void init_hourly_alert(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	int hourly_alert = -1;
	int ret = 0;

	ret = vconf_get_bool(VCONFKEY_SETAPPL_HOURLY_ALERT_BOOL, &hourly_alert);
	if (ret < 0){
		_ERR("can't get vconfkey value of [%s], ret=[%d]", VCONFKEY_SETAPPL_HOURLY_ALERT_BOOL, ret);
		hourly_alert = FALSE;
	} else if (hourly_alert == TRUE) {
		_DBG("[%s] value is [%d], hourly_alert is set..!!", VCONFKEY_SETAPPL_HOURLY_ALERT_BOOL, hourly_alert);
		if (_register_hourly_alert_alarm(ad) == EINA_FALSE) {
			_ERR("_register_hourly_alert_alarm is failed..!!");
		}
	}

	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_HOURLY_ALERT_BOOL, _hourly_alert_changed_cb, ad) < 0) {
		_ERR("Failed to add the callback for %s changed", VCONFKEY_SETAPPL_HOURLY_ALERT_BOOL);
	}
	/* for time revision */
	if (vconf_notify_key_changed(VCONFKEY_SYSTEM_TIME_CHANGED, _hourly_system_time_changed_cb, ad) < 0) {
		_ERR("Failed to add the callback for %s changed", VCONFKEY_SYSTEM_TIME_CHANGED);
	}
}

void fini_hourly_alert(void *data)
{
	int ret = 0;
	struct appdata *ad = data;
	ret_if(ad == NULL);

	//_unregister_hourly_alert_alarm(data);

	ret = vconf_ignore_key_changed(VCONFKEY_SETAPPL_HOURLY_ALERT_BOOL, _hourly_alert_changed_cb);
	if(ret != 0) {
		_E("vconf_ignore failed");
	}
}

