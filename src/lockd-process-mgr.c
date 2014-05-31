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

#include <vconf.h>
#include <vconf-keys.h>

#include <aul.h>
#include <pkgmgr-info.h>

#include "lockd-debug.h"
#include "lockd-process-mgr.h"
#include "starter-vconf.h"

#define LOCKD_DEFAULT_PKG_NAME "org.tizen.lockscreen"
#define LOCKD_DRAG_LOCKSCREEN "org.tizen.draglock"
#define LOCKD_PHONE_LOCK_PKG_NAME LOCKD_DEFAULT_PKG_NAME
#define RETRY_MAXCOUNT 30
#define RELAUNCH_INTERVAL 100*1000

#define LOCKD_VOICE_CALL_PKG_NAME "org.tizen.call"
#define LOCKD_VIDEO_CALL_PKG_NAME "org.tizen.vtmain"

static char *default_lockscreen_pkg = NULL;

static bool _lockd_exist_package(char *pkgid)
{
	int ret = 0;
	pkgmgrinfo_appinfo_h handle = NULL;

	ret = pkgmgrinfo_appinfo_get_appinfo(pkgid, &handle);
	if (PMINFO_R_OK != ret || NULL == handle) {
		LOCKD_SECURE_DBG("%s doesn't exist in this binary", pkgid);
		return false;
	}

	pkgmgrinfo_appinfo_destroy_appinfo(handle);

	return true;
}

void lockd_process_mgr_init(void)
{
	default_lockscreen_pkg = vconf_get_str(VCONF_PRIVATE_LOCKSCREEN_DEFAULT_PKGNAME);
	LOCKD_SECURE_DBG("default lock screen pkg name is %s", default_lockscreen_pkg);
	if (default_lockscreen_pkg == NULL) {
		default_lockscreen_pkg = LOCKD_DEFAULT_PKG_NAME;
	}
	if (!_lockd_exist_package(default_lockscreen_pkg)) {
		LOCKD_SECURE_ERR("%s is not exist, default lock screen pkg name is set to %s", default_lockscreen_pkg, LOCKD_DRAG_LOCKSCREEN);
		default_lockscreen_pkg = LOCKD_DRAG_LOCKSCREEN;
		if (vconf_set_str(VCONFKEY_SETAPPL_3RD_LOCK_PKG_NAME_STR, default_lockscreen_pkg) != 0) {
			LOCKD_SECURE_ERR("vconf key [%s] set [%s] is failed", VCONFKEY_SETAPPL_3RD_LOCK_PKG_NAME_STR, default_lockscreen_pkg);
		}
	}
}

static char *_lockd_process_mgr_get_pkgname(int lock_type)
{
	char *pkgname = NULL;

	if (lock_type > 1) {
		pkgname = vconf_get_str(VCONFKEY_SETAPPL_3RD_LOCK_PKG_NAME_STR);
		LOCKD_SECURE_DBG("pkg name is %s", pkgname);
		if (pkgname == NULL) {
			pkgname = default_lockscreen_pkg;
		} else if (_lockd_exist_package(pkgname) == FALSE) {
			pkgname = default_lockscreen_pkg;
		}
	} else {
		pkgname = default_lockscreen_pkg;
	}
	return pkgname;
}

int lockd_process_mgr_restart_lock(int lock_type)
{
	char *lock_app_path = NULL;
	int pid;
	int i;

	LOCKD_DBG("%s, %d", __func__, __LINE__);
	lock_app_path = _lockd_process_mgr_get_pkgname(lock_type);

	for (i=0; i<RETRY_MAXCOUNT; i++)
	{
		pid = aul_launch_app(lock_app_path, NULL);
		LOCKD_DBG("aul_launch_app(%s), pid = %d", lock_app_path, pid);
		if (pid == AUL_R_ETIMEOUT) {
			LOCKD_DBG("Relaunch lock application [%d]times", i);
			usleep(RELAUNCH_INTERVAL);
		} else {
			return pid;
		}
	}

	LOCKD_SECURE_DBG("Reset : aul_launch_app(%s, NULL), pid = %d", lock_app_path,
		  pid);
	return pid;
}

int
lockd_process_mgr_start_lock(void *data, int (*dead_cb) (int, void *),
			     int lock_type)
{
	char *lock_app_path = NULL;
	int pid;
	int i;

	LOCKD_DBG("%s, %d", __func__, __LINE__);
	lock_app_path = _lockd_process_mgr_get_pkgname(lock_type);

	for (i=0; i<RETRY_MAXCOUNT; i++)
	{
		pid = aul_launch_app(lock_app_path, NULL);

		LOCKD_SECURE_DBG("aul_launch_app(%s), pid = %d", lock_app_path, pid);

		if ((pid == AUL_R_ECOMM) || (pid == AUL_R_ETERMINATING)) {
			LOCKD_DBG("Relaunch lock application [%d]times", i);
			usleep(RELAUNCH_INTERVAL);
		} else if (pid == AUL_R_ERROR) {
			LOCKD_SECURE_ERR("launch[%s] is failed, launch default lock screen", lock_app_path);
			pid = aul_launch_app(default_lockscreen_pkg, NULL);
			if (pid == AUL_R_ERROR) {
				LOCKD_SECURE_ERR("launch[%s] is failed, launch drag lock screen", default_lockscreen_pkg);
				pid = aul_launch_app(LOCKD_DRAG_LOCKSCREEN, NULL);
				if (pid >0) {
					if (vconf_set_str(VCONFKEY_SETAPPL_3RD_LOCK_PKG_NAME_STR, LOCKD_DRAG_LOCKSCREEN) != 0) {
						LOCKD_SECURE_ERR("vconf key [%s] set [%s] is failed", VCONFKEY_SETAPPL_3RD_LOCK_PKG_NAME_STR, LOCKD_DRAG_LOCKSCREEN);
					}
					return pid;
				}
			} else {
				return pid;
			}
		} else {
			return pid;
		}
	}
	LOCKD_DBG("Relaunch lock application failed..!!");
	return pid;
}

int lockd_process_mgr_start_normal_lock(void *data, int (*dead_cb) (int, void *))
{
	int pid = 0;

	int i;
	for (i=0; i<RETRY_MAXCOUNT; i++)
	{
		pid = aul_launch_app(default_lockscreen_pkg, NULL);

		LOCKD_SECURE_DBG("aul_launch_app(%s), pid = %d", default_lockscreen_pkg, pid);

		if ((pid == AUL_R_ECOMM) || (pid == AUL_R_ETERMINATING)) {
			LOCKD_DBG("Relaunch lock application [%d]times", i);
			usleep(RELAUNCH_INTERVAL);
		} else if (pid == AUL_R_ERROR) {
			LOCKD_SECURE_DBG("launch[%s] is failed, launch default lock screen", default_lockscreen_pkg);
#if 0
			pid = aul_launch_app(LOCKD_DRAG_LOCKSCREEN, NULL);
			if (pid >0) {
				if (vconf_set_str(VCONFKEY_SETAPPL_3RD_LOCK_PKG_NAME_STR, LOCKD_DRAG_LOCKSCREEN) != 0) {
					LOCKD_SECURE_ERR("vconf key [%s] set [%s] is failed", VCONFKEY_SETAPPL_3RD_LOCK_PKG_NAME_STR, LOCKD_DRAG_LOCKSCREEN);
				}
				return pid;
			}
#endif
		} else {
			return pid;
		}
	}
	LOCKD_DBG("Relaunch lock application failed..!!");
	return pid;
}

int lockd_process_mgr_start_phone_lock(void)
{
	int pid = 0;
	bundle *b = NULL;

	LOCKD_DBG("%s, %d", __func__, __LINE__);
	b = bundle_create();

	bundle_add(b, "lock_type", "phone_lock");

	pid = aul_launch_app(LOCKD_PHONE_LOCK_PKG_NAME, b);
	LOCKD_SECURE_DBG("aul_launch_app(%s, b), pid = %d", LOCKD_PHONE_LOCK_PKG_NAME,
		  pid);
	if (b)
		bundle_free(b);

	return pid;
}

int lockd_process_mgr_start_recovery_lock(void)
{
	int pid = 0;
	bundle *b = NULL;

	LOCKD_DBG("%s, %d", __func__, __LINE__);
	b = bundle_create();

	bundle_add(b, "lock_type", "recovery_lock");

	pid = aul_launch_app(LOCKD_PHONE_LOCK_PKG_NAME, b);
	LOCKD_SECURE_DBG("aul_launch_app(%s, b), pid = %d", LOCKD_PHONE_LOCK_PKG_NAME,
		  pid);
	if (b)
		bundle_free(b);

	return pid;
}

int lockd_process_mgr_start_back_to_app_lock(void)
{
	int pid = 0;
	bundle *b = NULL;

	LOCKD_DBG("%s, %d", __func__, __LINE__);
	b = bundle_create();

	bundle_add(b, "lock_type", "back_to_call");

	pid = aul_launch_app(LOCKD_PHONE_LOCK_PKG_NAME, b);
	LOCKD_SECURE_DBG("aul_launch_app(%s, b), pid = %d", LOCKD_PHONE_LOCK_PKG_NAME,
		  pid);
	if (b)
		bundle_free(b);

	return pid;
}

void
lockd_process_mgr_terminate_lock_app(int lock_app_pid, int state)
{
	LOCKD_DBG
	    ("lockd_process_mgr_terminate_lock_app,  state:%d\n",
	     state);

	if (state == 1) {
		if (lock_app_pid != 0) {
			LOCKD_DBG("Terminate Lock app(pid : %d)", lock_app_pid);
			aul_terminate_pid(lock_app_pid);
		}
	}
}

void lockd_process_mgr_terminate_phone_lock(int phone_lock_pid)
{
	LOCKD_DBG("Terminate Phone Lock(pid : %d)", phone_lock_pid);
	aul_terminate_pid(phone_lock_pid);
}

void lockd_process_mgr_kill_lock_app(int lock_app_pid)
{
	LOCKD_DBG ("lockd_process_mgr_kill_lock_app [pid:%d]..", lock_app_pid);
	aul_kill_pid(lock_app_pid);
}

int lockd_process_mgr_check_lock(int pid)
{
	char buf[128];
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	/* Check pid is invalid. */
	if (aul_app_get_pkgname_bypid(pid, buf, sizeof(buf)) < 0) {
		LOCKD_DBG("no such pkg by pid %d\n", pid);
	} else {
		LOCKD_SECURE_DBG("app pkgname = %s, pid = %d\n", buf, pid);
		if (aul_app_is_running(buf) == TRUE) {
			LOCKD_DBG("%s [pid = %d] is running\n", buf, pid);
			return TRUE;
		} else {
			LOCKD_SECURE_DBG("[pid = %d] is exist but %s is not running\n",
				  pid, buf);
		}
	}
	return FALSE;
}

int lockd_process_mgr_check_call(int pid)
{
	char buf[128];
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	/* Check pid is invalid. */
	if (aul_app_get_pkgname_bypid(pid, buf, sizeof(buf)) < 0) {
		LOCKD_DBG("no such pkg by pid %d\n", pid);
	} else {
		LOCKD_SECURE_DBG("app pkgname = %s, pid = %d\n", buf, pid);
		if ((!strncmp(buf, LOCKD_VOICE_CALL_PKG_NAME, strlen(buf)))
		    || (!strncmp(buf, LOCKD_VIDEO_CALL_PKG_NAME, strlen(buf)))) {
		    return TRUE;
		}
	}
	return FALSE;
}

