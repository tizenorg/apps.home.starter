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

#include "starter.h"
#include "starter-util.h"
#include "x11.h"
#include "lockd-debug.h"
#include "hw_key.h"
#include "util.h"

int errno;

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "org.tizen.starter"
#endif

#define DEFAULT_THEME "tizen"

#define W_LAUNCHER_PKGNAME "org.tizen.w-launcher-app"
#define W_LOCKSCREEN_PKGNAME "org.tizen.w-lockscreen"

#ifdef FEATURE_TIZENW2
#define SETUP_WIZARD_PKGNAME "org.tizen.b2-setup-wizard"
#else
#define SETUP_WIZARD_PKGNAME "org.tizen.b2-setup-wizard"
#endif


static void _signal_handler(int signum, siginfo_t *info, void *unused)
{
    _DBG("_signal_handler : Terminated...");
    elm_exit();
}

int w_launch_app(char *pkgname, bundle *b)
{
	int r = AUL_R_OK;

	_SECURE_D("w_launch_app:[%s]", pkgname);

	r = aul_launch_app(pkgname, b);

	if (r < 0) {
		_ERR("launch failed [%s] ret=[%d]", pkgname, r);
	}

	return r;
}

static int _w_app_dead_cb(int pid, void *data)
{
	_DBG("app dead cb call! (pid : %d)", pid);

	struct appdata *ad = (struct appdata *)data;

	if (pid == ad->launcher_pid) {
		_ERR("w-launcher-app (pid:%d) is destroyed.", pid);
		ad->launcher_pid = w_launch_app(W_LAUNCHER_PKGNAME, NULL);
	}

	return 0;
}

static int _w_check_first_boot(void)
{
	int is_first = 0;
	int ret = 0;

#if 1 // NOT YET define vconfkey from setting  "VCONFKEY_SETUP_WIZARD_FIRST_BOOT"
	ret = vconf_get_bool(VCONFKEY_SETUP_WIZARD_FIRST_BOOT, &is_first);
	if (ret < 0){
		_ERR("can't get vconfkey value of [%s], ret=[%d]", VCONFKEY_SETUP_WIZARD_FIRST_BOOT, ret);
		is_first = 0;
	} else if (is_first == 1) {
		_ERR("[%s] value is [%d], first booting..!!", VCONFKEY_SETUP_WIZARD_FIRST_BOOT, is_first);
	}
#endif

	return is_first;
}

static Eina_Bool _w_starter_idler_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	_DBG("%s, %d", __func__, __LINE__);

	ad->launcher_pid = w_launch_app(W_LAUNCHER_PKGNAME, NULL);

	return ECORE_CALLBACK_CANCEL;
}


#define TEMP_VCONFKEY_LOCK_TYPE "db/setting/lock_type"
static void _w_wms_changed_cb(keynode_t* node, void *data)
{
	int wms_state = -1;
	int lock_type = -1;
	int test_mode = -1;
	struct appdata *ad = (struct appdata *)data;

	_DBG("%s, %d", __func__, __LINE__);

	if (node) {
		wms_state = vconf_keynode_get_bool(node);
	} else {
		if (vconf_get_bool(VCONFKEY_WMS_WMANAGER_CONNECTED, &wms_state) < 0) {
			_ERR("Failed to get %s", VCONFKEY_WMS_WMANAGER_CONNECTED);
			return;
		}
	}
	_DBG("WMS key value:[%d], previous state:[%d]", wms_state, ad->wms_connected);

	vconf_get_int(TEMP_VCONFKEY_LOCK_TYPE, &lock_type);
	vconf_get_int(VCONFKEY_TESTMODE_SCREEN_LOCK, &test_mode);

	if ((lock_type != 1) || (test_mode == VCONFKEY_TESTMODE_SCREEN_LOCK_DISABLE)) {
		ad->wms_connected = wms_state;
		return;
	}

	if (wms_state == FALSE) {
		if (ad->wms_connected == TRUE) {
			_ERR("WMS connect state is changed from [%d] to [%d]", ad->wms_connected, wms_state);
			w_launch_app(W_LOCKSCREEN_PKGNAME, NULL);
		}
	}
	ad->wms_connected = wms_state;

	return;
}

static void _w_power_off_cb(keynode_t* node, void *data)
{
	int val = VCONFKEY_SYSMAN_POWER_OFF_NONE;

	vconf_get_int(VCONFKEY_SYSMAN_POWER_OFF_STATUS, &val);

	if (val > VCONFKEY_SYSMAN_POWER_OFF_POPUP) {
		_ERR("power off status : %d", val);
		if (vconf_ignore_key_changed(VCONFKEY_WMS_WMANAGER_CONNECTED, _w_wms_changed_cb) < 0)
			_ERR("Failed to ignore the callback for [%s]", VCONFKEY_WMS_WMANAGER_CONNECTED);
		exit(0);
	}
}

static void _w_lang_changed_cb(keynode_t* node, void *data)
{
	char *locale = NULL;
	_DBG("%s, %d", __func__, __LINE__);

	locale = vconf_get_str(VCONFKEY_LANGSET);

	if (locale != NULL) {
		elm_language_set(locale);
	}
}

//TO DO. later remove this definition after applying DB structure
#define LAUNCHER_XML_PATH "/opt/usr/share/w-launcher"
static void _init(struct appdata *ad)
{
	int r;
	struct sigaction act;
	char *file = NULL;
	int first = -1;
	int lock_type = -1;
	int wms_state = -1;
	int test_mode = -1;

	memset(&act,0x00,sizeof(struct sigaction));
	act.sa_sigaction = _signal_handler;
	act.sa_flags = SA_SIGINFO;

	int ret = sigemptyset(&act.sa_mask);
	if (ret < 0) {
		_ERR("Failed to sigemptyset[%s]", strerror(errno));
	}
	ret = sigaddset(&act.sa_mask, SIGTERM);
	if (ret < 0) {
		_ERR("Failed to sigaddset[%s]", strerror(errno));
	}
	ret = sigaction(SIGTERM, &act, NULL);
	if (ret < 0) {
		_ERR("Failed to sigaction[%s]", strerror(errno));
	}

	memset(ad, 0, sizeof(struct appdata));

	gettimeofday(&ad->tv_start, NULL);
	aul_launch_init(NULL,NULL);

	aul_listen_app_dead_signal(_w_app_dead_cb, ad);

	//TO DO. later remove this after applying DB structure
	if (chmod(LAUNCHER_XML_PATH, 0777) < 0) {
		_ERR("chmod: %s\n", strerror(errno));
	}

	if (vconf_notify_key_changed(VCONFKEY_LANGSET, _w_lang_changed_cb, NULL) < 0) {
		_ERR("Failed to add the callback for [%s]", VCONFKEY_LANGSET);
	}

	if (vconf_notify_key_changed(VCONFKEY_WMS_WMANAGER_CONNECTED, _w_wms_changed_cb, ad) < 0) {
		_ERR("Failed to add the callback for %s changed", VCONFKEY_WMS_WMANAGER_CONNECTED);
	}
	if (vconf_get_bool(VCONFKEY_WMS_WMANAGER_CONNECTED, &wms_state) < 0) {
		_ERR("Failed to get [%s]", VCONFKEY_WMS_WMANAGER_CONNECTED);
	} else {
		ad->wms_connected = wms_state;
		_DBG("ad->wms_connected : [%d]", ad->wms_connected);
	}

	if (_w_check_first_boot() == TRUE) {
		w_launch_app(SETUP_WIZARD_PKGNAME, NULL);
		ecore_idler_add(_w_starter_idler_cb, ad);
	} else {
		_DBG("Not first booting, launch [%s]..!!", W_LAUNCHER_PKGNAME);

		vconf_get_int(VCONFKEY_TESTMODE_SCREEN_LOCK, &test_mode);
		vconf_get_int(TEMP_VCONFKEY_LOCK_TYPE, &lock_type);

		if ((wms_state == FALSE) && (lock_type == 1) && (test_mode != VCONFKEY_TESTMODE_SCREEN_LOCK_DISABLE))  {
			_ERR("BT disconneted and privacy lock is set");
			w_launch_app(W_LOCKSCREEN_PKGNAME, NULL);
			ecore_idler_add(_w_starter_idler_cb, ad);
		} else {
			ad->launcher_pid = w_launch_app(W_LAUNCHER_PKGNAME, NULL);
		}
	}

	create_key_window();
	init_hourly_alert(ad);

	if (vconf_notify_key_changed(VCONFKEY_SYSMAN_POWER_OFF_STATUS, _w_power_off_cb, NULL) < 0)
		_ERR("Failed to add the callback for [%s]", VCONFKEY_SYSMAN_POWER_OFF_STATUS);

	return;
}

static void _fini(struct appdata *ad)
{
	struct timeval tv, res;

	if (ad == NULL) {
		fprintf(stderr, "Invalid argument: appdata is NULL\n");
		return;
	}

	destroy_key_window();
	fini_hourly_alert(ad);

	gettimeofday(&tv, NULL);
	timersub(&tv, &ad->tv_start, &res);
	_DBG("Total time: %d.%06d sec\n", (int)res.tv_sec, (int)res.tv_usec);
}

int main(int argc, char *argv[])
{
	struct appdata ad;

	WRITE_FILE_LOG("%s", "Main function is started in starter");
	_DBG("starter is launched..!!");
#if 0
	set_window_scale();	/* not in loop */
#endif

	elm_init(argc, argv);

	_init(&ad);

	elm_run();

	_fini(&ad);

	elm_shutdown();

	return 0;
}
