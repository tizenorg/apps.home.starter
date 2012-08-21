/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
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
#include <stdio.h>
#include <stdlib.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <errno.h>

#include "lockd-debug.h"
#include "lock-daemon.h"
#include "lockd-process-mgr.h"
#include "lockd-window-mgr.h"

struct lockd_data {
	int lock_app_pid;
	lockw_data *lockw;
};

#define LAUNCH_INTERVAL 100*1000

static void lockd_launch_lockscreen(struct lockd_data *lockd);
static void lockd_launch_app_lockscreen(struct lockd_data *lockd);

static void lockd_unlock_lockscreen(struct lockd_data *lockd);

static void _lockd_notify_pm_state_cb(keynode_t * node, void *data)
{
	LOCKD_DBG("PM state Notification!!");

	struct lockd_data *lockd = (struct lockd_data *)data;
	int val = -1;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	if (vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY_PM_STATE");
		return;
	}

	if (val == VCONFKEY_PM_STATE_LCDOFF) {
		lockd_launch_app_lockscreen(lockd);
	}
}

static void
_lockd_notify_lock_state_cb(keynode_t * node, void *data)
{
	LOCKD_DBG("lock state changed!!");

	struct lockd_data *lockd = (struct lockd_data *)data;
	int val = -1;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY_IDLE_LOCK_STATE");
		return;
	}

	if (val == VCONFKEY_IDLE_UNLOCK) {
		LOCKD_DBG("unlocked..!!");
		if (lockd->lock_app_pid != 0) {
			LOCKD_DBG("terminate lock app..!!");
			lockd_process_mgr_terminate_lock_app(lockd->lock_app_pid, 1);
		}
	}
}

static int lockd_app_dead_cb(int pid, void *data)
{
	LOCKD_DBG("app dead cb call! (pid : %d)", pid);

	struct lockd_data *lockd = (struct lockd_data *)data;

	if (pid == lockd->lock_app_pid ) {
		LOCKD_DBG("lock app(pid:%d) is destroyed.", pid);
		lockd_unlock_lockscreen(lockd);
	}
	return 0;
}

static Eina_Bool lockd_app_create_cb(void *data, int type, void *event)
{
	struct lockd_data *lockd = (struct lockd_data *)data;

	if (lockd == NULL) {
		return EINA_TRUE;
	}
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	lockd_window_set_window_effect(lockd->lockw, lockd->lock_app_pid,
				       event);
	lockd_window_set_window_property(lockd->lockw, lockd->lock_app_pid,
					 event);
	return EINA_FALSE;
}

static Eina_Bool lockd_app_show_cb(void *data, int type, void *event)
{
	struct lockd_data *lockd = (struct lockd_data *)data;

	if (lockd == NULL) {
		return EINA_TRUE;
	}
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	lockd_window_set_window_property(lockd->lockw, lockd->lock_app_pid,
					 event);

	return EINA_FALSE;
}

static void lockd_launch_app_lockscreen(struct lockd_data *lockd)
{
	LOCKD_DBG("launch app lock screen");

	int call_state = -1, phlock_state = -1;
	int r = 0;

	if (lockd_process_mgr_check_lock(lockd->lock_app_pid) == TRUE) {
		LOCKD_DBG("Lock Screen App is already running.");
		r = lockd_process_mgr_restart_lock();
		if (r < 0) {
			LOCKD_DBG("Restarting Lock Screen App is fail [%d].", r);
			usleep(LAUNCH_INTERVAL);
		} else {
			LOCKD_DBG("Restarting Lock Screen App, pid[%d].", r);
			return;
		}
	}

	vconf_get_int(VCONFKEY_CALL_STATE, &call_state);
	if (call_state != VCONFKEY_CALL_OFF) {
		LOCKD_DBG
		    ("Current call state(%d) does not allow to launch lock screen.",
		     call_state);
		return;
	}

	lockd->lock_app_pid =
	    lockd_process_mgr_start_lock(lockd, lockd_app_dead_cb);
	if (lockd->lock_app_pid < 0)
		return;

	vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_LOCK);
	lockd_window_mgr_ready_lock(lockd, lockd->lockw, lockd_app_create_cb,
				    lockd_app_show_cb);
}

static void lockd_launch_lockscreen(struct lockd_data *lockd)
{
	LOCKD_DBG("launch lock screen");

	int call_state = -1;
	int r = 0;

	if (lockd_process_mgr_check_lock(lockd->lock_app_pid) == TRUE) {
		LOCKD_DBG("Lock Screen App is already running.");
		r = lockd_process_mgr_restart_lock();
		if (r < 0) {
			LOCKD_DBG("Restarting Lock Screen App is fail [%d].", r);
		} else {
			LOCKD_DBG("Restarting Lock Screen App, pid[%d].", r);
			return;
		}
	}

	vconf_get_int(VCONFKEY_CALL_STATE, &call_state);
	if (call_state != VCONFKEY_CALL_OFF) {
		LOCKD_DBG
		    ("Current call state(%d) does not allow to launch lock screen.",
		     call_state);
		return;
	}

	lockd_window_mgr_ready_lock(lockd, lockd->lockw, lockd_app_create_cb,
				    lockd_app_show_cb);

	lockd->lock_app_pid =
	    lockd_process_mgr_start_lock(lockd, lockd_app_dead_cb);

	vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_LOCK);
}

static void lockd_unlock_lockscreen(struct lockd_data *lockd)
{
	LOCKD_DBG("unlock lock screen");
	lockd->lock_app_pid = 0;

	lockd_window_mgr_finish_lock(lockd->lockw);

	vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_UNLOCK);
}

static void lockd_init_vconf(struct lockd_data *lockd)
{
	int val = -1;

	if (vconf_notify_key_changed
	    (VCONFKEY_PM_STATE, _lockd_notify_pm_state_cb, lockd) != 0) {
		LOCKD_ERR("Fail vconf_notify_key_changed : VCONFKEY_PM_STATE");
	}

	if (vconf_notify_key_changed
	    (VCONFKEY_IDLE_LOCK_STATE,
	     _lockd_notify_lock_state_cb,
	     lockd) != 0) {
		LOCKD_ERR
		    ("[Error] vconf notify : lock state");
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

	lockd_init_vconf(lockd);

	lockd->lockw = lockd_window_init();

	LOCKD_DBG("%s, %d", __func__, __LINE__);
}

int start_lock_daemon()
{
	struct lockd_data *lockd = NULL;
	int val = -1;

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	lockd = (struct lockd_data *)malloc(sizeof(struct lockd_data));
	memset(lockd, 0x0, sizeof(struct lockd_data));
	lockd_start_lock_daemon(lockd);

	return 0;
}
