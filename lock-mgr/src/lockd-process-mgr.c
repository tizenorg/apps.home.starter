/*
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd All Rights Reserved 
 *
 * This file is part of <starter>
 * Written by <Seungtaek Chung> <seungtaek.chung@samsung.com>, <Mi-Ju Lee> <miju52.lee@samsung.com>, <Xi Zhichan> <zhichan.xi@samsung.com>
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of SAMSUNG ELECTRONICS ("Confidential Information").
 * You shall not disclose such Confidential Information and shall use it only in accordance
 * with the terms of the license agreement you entered into with SAMSUNG ELECTRONICS.
 * SAMSUNG make no representations or warranties about the suitability of the software,
 * either express or implied, including but not limited to the implied warranties of merchantability,
 * fitness for a particular purpose, or non-infringement.
 * SAMSUNG shall not be liable for any damages suffered by licensee as a result of using,
 * modifying or distributing this software or its derivatives.
 *
 */

#include <vconf.h>
#include <vconf-keys.h>

#include <aul.h>

#include "lockd-debug.h"
#include "lockd-process-mgr.h"

#define LOCKD_PHONE_LOCK_PKG_NAME "org.tizen.phone-lock"

int lockd_process_mgr_start_phone_lock(void)
{
	int pid = 0;
	bundle *b = NULL;

	b = bundle_create();

	bundle_add(b, "pwlock_type", "running_lock");
	bundle_add(b, "window_type", "alpha");

	pid = aul_launch_app(LOCKD_PHONE_LOCK_PKG_NAME, b);
	LOCKD_DBG("aul_launch_app(%s, b), pid = %d", LOCKD_PHONE_LOCK_PKG_NAME,
		  pid);
	if (b)
		bundle_free(b);

	return pid;
}

void lockd_process_mgr_terminate_phone_lock(int phone_lock_pid)
{
	LOCKD_DBG("Terminate Phone Lock(pid : %d)", phone_lock_pid);
	aul_terminate_pid(phone_lock_pid);
}

int lockd_process_mgr_check_lock(int pid)
{
	char buf[128];
	LOCKD_DBG("%s, %d", __func__, __LINE__);
	if (aul_app_get_pkgname_bypid(pid, buf, sizeof(buf)) < 0) {
		LOCKD_DBG("no such pkg by pid %d\n", pid);
	} else {
		LOCKD_DBG("lock screen pkgname = %s, pid = %d\n", buf, pid);
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
