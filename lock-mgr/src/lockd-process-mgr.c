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

#include <vconf.h>
#include <vconf-keys.h>

#include <aul.h>

#include "lockd-debug.h"
#include "lockd-process-mgr.h"
#include "starter-vconf.h"

#define LOCKD_DEFAULT_PKG_NAME "org.tizen.draglock"
#define LOCKD_DEFAULT_LOCKSCREEN "org.tizen.draglock"
#define RETRY_MAXCOUNT 30
#define RELAUNCH_INTERVAL 100*1000

static char *_lockd_process_mgr_get_pkgname(void)
{
	char *pkgname = NULL;

	pkgname = vconf_get_str(VCONF_PRIVATE_LOCKSCREEN_PKGNAME);

	LOCKD_DBG("pkg name is %s", pkgname);

	if (pkgname == NULL) {
		return LOCKD_DEFAULT_PKG_NAME;
	}

	return pkgname;
}

int lockd_process_mgr_restart_lock(void)
{
	char *lock_app_path = NULL;
	int pid;
	bundle *b = NULL;

	lock_app_path = _lockd_process_mgr_get_pkgname();

	b = bundle_create();

	bundle_add(b, "mode", "normal");

	pid = aul_launch_app(lock_app_path, b);

	LOCKD_DBG("Reset : aul_launch_app(%s, NULL), pid = %d", lock_app_path,
		  pid);

	if (b)
		bundle_free(b);

	return pid;
}

int
lockd_process_mgr_start_lock(void *data, int (*dead_cb) (int, void *))
{
	char *lock_app_path = NULL;
	int pid;
	bundle *b = NULL;

	lock_app_path = _lockd_process_mgr_get_pkgname();

	b = bundle_create();

	bundle_add(b, "mode", "normal");

	int i;
	for (i=0; i<RETRY_MAXCOUNT; i++)
	{
		pid = aul_launch_app(lock_app_path, b);

		LOCKD_DBG("aul_launch_app(%s, NULL), pid = %d", lock_app_path, pid);

		if (pid == AUL_R_ECOMM) {
			LOCKD_DBG("Relaunch lock application [%d]times", i);
			usleep(RELAUNCH_INTERVAL);
		} else if (pid == AUL_R_ERROR) {
			LOCKD_DBG("launch[%s] is failed, launch default lock screen", lock_app_path);
			pid = aul_launch_app(LOCKD_DEFAULT_LOCKSCREEN, b);
			if (pid >0) {
				aul_listen_app_dead_signal(dead_cb, data);
				if (b)
					bundle_free(b);
				return pid;
			}
		} else {
			/* set listen and dead signal */
			aul_listen_app_dead_signal(dead_cb, data);
			if (b)
				bundle_free(b);
			return pid;
		}
	}
	LOCKD_DBG("Relaunch lock application failed..!!");
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

int lockd_process_mgr_check_lock(int pid)
{
	char buf[128];
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	if (aul_app_get_pkgname_bypid(pid, buf, sizeof(buf)) < 0) {
		LOCKD_DBG("no such pkg by pid %d\n", pid);
	} else {
		LOCKD_DBG("app pkgname = %s, pid = %d\n", buf, pid);
		if (aul_app_is_running(buf) == TRUE) {
			LOCKD_DBG("%s [pid = %d] is running\n", buf, pid);
			return TRUE;
		} else {
			LOCKD_DBG("[pid = %d] is exist but %s is not running\n",
				  pid, buf);
		}
	}
	return FALSE;
}
