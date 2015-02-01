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
#include <glib.h>

#include <vconf.h>
#include <signal.h>

#include <dd-deviced.h>

#include "starter_w.h"
#include "starter-util.h"
#include "x11.h"
#include "lockd-debug.h"
#include "hw_key_w.h"
#include "util.h"

int errno;

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "com.samsung.starter"
#endif

#define DEFAULT_THEME 				"tizen"

#define W_LOCKSCREEN_PKGNAME 		"com.samsung.w-lockscreen"
#define REMOTE_LOCK_PKGNAME			"com.samsung.wfmw-remote-lock"

#ifdef FEATURE_TIZENW2
#define SETUP_WIZARD_PKGNAME 		"com.samsung.b2-setup-wizard"
#else
#define SETUP_WIZARD_PKGNAME 		"com.samsung.b2-setup-wizard"
#endif
#define PWLOCK_PKGNAME 				"com.samsung.b2-pwlock"

#define NOTIFICATION_APP_PKGNAME 	"com.samsung.idle-noti-drawer"

#define FACTORY_TDF_NOTIFIER_PATH			"/csa/factory/cblkftdf"

#define VCONFKEY_BT_CONNECTED 		"memory/private/sap/conn_type"

#define VCONFKEY_REMOTE_LOCK_ISLOCKED		"db/private/com.samsung.wfmw/is_locked"

static struct appdata *g_app_data = NULL;

void *starter_get_app_data(void){
	return g_app_data;
}


static void _signal_handler(int signum, siginfo_t *info, void *unused)
{
    _DBG("_signal_handler : Terminated...");
    elm_exit();
}

int w_open_app(char *pkgname)
{
	int r = AUL_R_OK;

	_SECURE_D("w_open_app:[%s]", pkgname);

	r = aul_open_app(pkgname);

	if (r < 0) {
		_ERR("open app failed [%s] ret=[%d]", pkgname, r);
	}

	return r;
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

#define RETRY_CNT 5
static Eina_Bool _w_retry_idler_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	_DBG("%s, %d", __func__, __LINE__);

	ad->retry_cnt++;
	ad->launcher_pid = w_launch_app(ad->home_pkgname, NULL);

	if (ad->launcher_pid > 0) {
		if (-1 == deviced_conf_set_mempolicy_bypid(ad->launcher_pid, OOM_IGNORE)) {
			_ERR("Cannot set the memory policy for Homescreen(%d)", ad->launcher_pid);
		} else {
			_ERR("Set the memory policy for Homescreen(%d)", ad->launcher_pid);
		}
	}
	else{
		if(ad->retry_cnt >= RETRY_CNT){
			ad->retry_cnt = 0;
			return ECORE_CALLBACK_CANCEL;
		}
		else{
			return ECORE_CALLBACK_RENEW;
		}
	}
	ad->retry_cnt = 0;
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _w_retry_idler_first_launch_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;
	bundle *b;
	b = bundle_create();
	if (!b) {
		_E("Cannot create bundle");
		return;
	}

	_DBG("%s, %d", __func__, __LINE__);

	bundle_add(b, "home_op", "first_boot");

	ad->retry_cnt++;
	ad->launcher_pid = w_launch_app(ad->home_pkgname, b);

	if (ad->launcher_pid > 0) {
		if (-1 == deviced_conf_set_mempolicy_bypid(ad->launcher_pid, OOM_IGNORE)) {
			_ERR("Cannot set the memory policy for Homescreen(%d)", ad->launcher_pid);
		} else {
			_ERR("Set the memory policy for Homescreen(%d)", ad->launcher_pid);
		}
	}
	else{
		if(ad->retry_cnt >= RETRY_CNT){
			ad->retry_cnt = 0;
			bundle_free(b);
			return ECORE_CALLBACK_CANCEL;
		}
		else{
			bundle_free(b);
			return ECORE_CALLBACK_RENEW;
		}
	}
	ad->retry_cnt = 0;
	bundle_free(b);
	return ECORE_CALLBACK_CANCEL;
}

static int _w_app_dead_cb(int pid, void *data)
{
	_DBG("app dead cb call! (pid : %d)", pid);

	struct appdata *ad = (struct appdata *)data;

	if (pid == ad->launcher_pid) {
		_ERR("w-launcher-app (pid:%d) is destroyed.", pid);
		ad->launcher_pid = w_launch_app(ad->home_pkgname, NULL);
		if (ad->launcher_pid > 0) {
			if (-1 == deviced_conf_set_mempolicy_bypid(ad->launcher_pid, OOM_IGNORE)) {
				_ERR("Cannot set the memory policy for Homescreen(%d)", ad->launcher_pid);
			} else {
				_ERR("Set the memory policy for Homescreen(%d)", ad->launcher_pid);
			}
		}
		else{
			_ERR("Launch Home failed.");
			ecore_idler_add(_w_retry_idler_cb, ad);
		}
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

	ad->launcher_pid = w_launch_app(ad->home_pkgname, NULL);

	if (ad->launcher_pid > 0) {
		if (-1 == deviced_conf_set_mempolicy_bypid(ad->launcher_pid, OOM_IGNORE)) {
			_ERR("Cannot set the memory policy for Homescreen(%d)", ad->launcher_pid);
		} else {
			_ERR("Set the memory policy for Homescreen(%d)", ad->launcher_pid);
		}
	}
	else{
		_ERR("Launch Home failed.");
		ecore_idler_add(_w_retry_idler_cb, ad);
	}

	return ECORE_CALLBACK_CANCEL;
}


static Eina_Bool _w_starter_lockscreen_idler_cb(void *data)
{
	struct appdata *ad = (struct appdata *)data;

	_DBG("%s, %d", __func__, __LINE__);

	w_launch_app(W_LOCKSCREEN_PKGNAME, NULL);

	if (ad->first_boot == FALSE)
		ecore_idler_add(_w_starter_idler_cb, ad);

	return ECORE_CALLBACK_CANCEL;
}


#define TEMP_VCONFKEY_LOCK_TYPE "db/setting/lock_type"
static void _w_BT_changed_cb(keynode_t* node, void *data)
{
	int bt_state = -1;
	int lock_type = -1;
	int test_mode = -1;
	struct appdata *ad = (struct appdata *)data;

	_DBG("%s, %d", __func__, __LINE__);

	if (node) {
		bt_state = vconf_keynode_get_int(node);
	} else {
		if (vconf_get_int(VCONFKEY_BT_CONNECTED, &bt_state) < 0) {
			_ERR("Failed to get %s", VCONFKEY_BT_CONNECTED);
			return;
		}
	}
	_DBG("WMS key value:[%d], previous state:[%d]", bt_state, ad->bt_connected);

	vconf_get_int(TEMP_VCONFKEY_LOCK_TYPE, &lock_type);
	vconf_get_int(VCONFKEY_TESTMODE_SCREEN_LOCK, &test_mode);

	if ((lock_type != 1) || (test_mode == VCONFKEY_TESTMODE_SCREEN_LOCK_DISABLE)) {
		ad->bt_connected = bt_state;
		return;
	}

	if (bt_state == FALSE) {
		if (ad->bt_connected == TRUE) {
			_ERR("BT connect state is changed from [%d] to [%d]", ad->bt_connected, bt_state);
			w_launch_app(W_LOCKSCREEN_PKGNAME, NULL);
		}
	}
	ad->bt_connected = bt_state;

	return;
}

static void _w_power_off_cb(keynode_t* node, void *data)
{
	int val = VCONFKEY_SYSMAN_POWER_OFF_NONE;

	vconf_get_int(VCONFKEY_SYSMAN_POWER_OFF_STATUS, &val);

	if (val > VCONFKEY_SYSMAN_POWER_OFF_POPUP) {
		_ERR("power off status : %d", val);
		if (vconf_ignore_key_changed(VCONFKEY_BT_CONNECTED, _w_BT_changed_cb) < 0)
			_ERR("Failed to ignore the callback for [%s]", VCONFKEY_BT_CONNECTED);
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


static void _launch_home_cb(keynode_t* node, void *data)
{
	int seq;
	struct appdata *ad = (struct appdata *)data;
	bundle *b = NULL;

	if (node) {
		seq = vconf_keynode_get_int(node);
	} else {
		if (vconf_get_int(VCONFKEY_STARTER_SEQUENCE, &seq) < 0) {
			_E("Failed to get sequence info");
			return;
		}
	}

	b = bundle_create();
	if (!b) {
		_E("Cannot create bundle");
		return;
	}
	bundle_add(b, "home_op", "first_boot");

	_DBG("_launch_home_cb, seq=%d", seq);
	if (seq == 1) {
		ad->launcher_pid = w_launch_app(ad->home_pkgname, b);
		if (ad->launcher_pid > 0) {
			if (-1 == deviced_conf_set_mempolicy_bypid(ad->launcher_pid, OOM_IGNORE)) {
				_ERR("Cannot set the memory policy for Homescreen(%d)", ad->launcher_pid);
			} else {
				_ERR("Set the memory policy for Homescreen(%d)", ad->launcher_pid);
			}
		} else{
			_ERR("Launch Home failed.");
			ecore_idler_add(_w_retry_idler_first_launch_cb, ad);
		}
	}

	create_key_window(ad->home_pkgname, ad);
	bundle_free(b);
}


static void _init(struct appdata *ad)
{
	int r;
	struct sigaction act;
	char *file = NULL;
	int first = -1;
	int lock_type = -1;
	int bt_state = -1;
	int test_mode = -1;
	int remote_lock = 0;

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
	g_app_data = &*ad;

	ad->retry_cnt = 0;
	ad->nike_running_status = 0;
	ad->lcd_status = 0;

	gettimeofday(&ad->tv_start, NULL);
	aul_launch_init(NULL,NULL);

	aul_listen_app_dead_signal(_w_app_dead_cb, ad);

	if (vconf_notify_key_changed(VCONFKEY_LANGSET, _w_lang_changed_cb, NULL) < 0)
		_ERR("Failed to add the callback for [%s]", VCONFKEY_LANGSET);

	ad->home_pkgname = vconf_get_str("file/private/homescreen/pkgname");
	if (!ad->home_pkgname) {
		ad->home_pkgname = W_HOME_PKGNAME;
	}
	_ERR("Home pkg name is [%s]", ad->home_pkgname);

#ifdef TARGET
	if (bincfg_is_factory_binary() == BIN_TYPE_FACTORY) {
		gchar *contents;
		gsize length;
		GError *error = NULL;
		gboolean result = FALSE;

		_ERR("Factory binary..!!");

		if (g_file_get_contents(FACTORY_TDF_NOTIFIER_PATH, &contents, &length, &error)) {
			gchar *found = NULL;
			_ERR("Read %d bytes from filesystem", length);
			if ((found = g_strstr_len(contents, strlen(contents), "ON"))) {
				// Launch TDF Notifier
				char *argv[3];
				argv[0] = "/usr/bin/testmode";
				argv[1] = "*#833#";
				argv[2] = NULL;

				execv("/usr/bin/testmode", argv);

				g_free(contents);
				return;
			}
			g_free(contents);
		} else {
			_ERR("read failed [%d]: [%s]", error->code, error->message);
			g_error_free(error);
			error = NULL;
		}
		ad->launcher_pid = w_launch_app(ad->home_pkgname, NULL);
		if(ad->launcher_pid < 0){
			_ERR("Launch Home failed.");
			ecore_idler_add(_w_retry_idler_cb, ad);
		}
		create_key_window_factory_mode(ad->home_pkgname, ad);
	} else {
#endif
		if (vconf_notify_key_changed(VCONFKEY_BT_CONNECTED, _w_BT_changed_cb, ad) < 0) {
			_ERR("Failed to add the callback for %s changed", VCONFKEY_BT_CONNECTED);
		}
		if (vconf_get_int(VCONFKEY_BT_CONNECTED, &bt_state) < 0) {
			_ERR("Failed to get [%s]", VCONFKEY_BT_CONNECTED);
		} else {
			ad->bt_connected = bt_state;
			_DBG("ad->bt_connected : [%d]", ad->bt_connected);
		}

#ifdef TELEPHONY_DISABLE //B2
		if (_w_check_first_boot() == TRUE) {
			w_launch_app(SETUP_WIZARD_PKGNAME, NULL);
			ecore_idler_add(_w_starter_idler_cb, ad);
		} else {
			_DBG("Not first booting, launch [%s]..!!", ad->home_pkgname);

			vconf_get_int(VCONFKEY_TESTMODE_SCREEN_LOCK, &test_mode);
			vconf_get_int(TEMP_VCONFKEY_LOCK_TYPE, &lock_type);

			if ((bt_state == FALSE) && (lock_type == 1) && (test_mode != VCONFKEY_TESTMODE_SCREEN_LOCK_DISABLE))  {
				_ERR("BT disconneted and privacy lock is set");
				w_launch_app(W_LOCKSCREEN_PKGNAME, NULL);
				ecore_idler_add(_w_starter_idler_cb, ad);
			} else {
				ad->launcher_pid = w_launch_app(ad->home_pkgname, NULL);
				if (ad->launcher_pid > 0) {
					if (-1 == deviced_conf_set_mempolicy_bypid(ad->launcher_pid, OOM_IGNORE)) {
						_ERR("Cannot set the memory policy for Homescreen(%d)", ad->launcher_pid);
					} else {
						_ERR("Set the memory policy for Homescreen(%d)", ad->launcher_pid);
					}
				}
			}
		}
#else //B2-3G //TELEPHONY_DISABLE 

#if 0 // To do not display home before setupwizard

		w_launch_app(PWLOCK_PKGNAME, NULL);
		vconf_get_int(VCONFKEY_TESTMODE_SCREEN_LOCK, &test_mode);
		vconf_get_int(TEMP_VCONFKEY_LOCK_TYPE, &lock_type);

		if ((wms_state == FALSE) && (lock_type == 1) && (test_mode != VCONFKEY_TESTMODE_SCREEN_LOCK_DISABLE))  {
			_ERR("BT disconneted and privacy lock is set");
			ecore_idler_add(_w_starter_lockscreen_idler_cb, ad);
		} else {
			ad->launcher_pid = w_launch_app(ad->home_pkgname, NULL);
			if (ad->launcher_pid > 0) {
				if (-1 == deviced_conf_set_mempolicy_bypid(ad->launcher_pid, OOM_IGNORE)) {
					_ERR("Cannot set the memory policy for Homescreen(%d)", ad->launcher_pid);
				} else {
					_ERR("Set the memory policy for Homescreen(%d)", ad->launcher_pid);
				}
			}
		}

#else //B3

		vconf_get_int(VCONFKEY_TESTMODE_SCREEN_LOCK, &test_mode);
		vconf_get_int(TEMP_VCONFKEY_LOCK_TYPE, &lock_type);

		if (_w_check_first_boot() == TRUE) {

			//First boot : launch pwlock > set seq > home
			ad->first_boot = TRUE;

			vconf_set_int(VCONFKEY_STARTER_SEQUENCE, 0);

			if (vconf_notify_key_changed(VCONFKEY_STARTER_SEQUENCE, _launch_home_cb, ad) < 0)
				_ERR("Failed to add the callback for show event");
#ifdef MODEM_ALWAYS_OFF
			w_launch_app(SETUP_WIZARD_PKGNAME, NULL);
#else
			w_launch_app(PWLOCK_PKGNAME, NULL);
#endif
			if ((bt_state == FALSE) && (lock_type == 1) && (test_mode != VCONFKEY_TESTMODE_SCREEN_LOCK_DISABLE))  {
				_ERR("BT disconneted and privacy lock is set");
				ecore_idler_add(_w_starter_lockscreen_idler_cb, ad);
			}
		}else {

			// Not first boot : launch home > pwlock
			ad->first_boot = FALSE;

			if ((bt_state == FALSE) && (lock_type == 1) && (test_mode != VCONFKEY_TESTMODE_SCREEN_LOCK_DISABLE))  {
				_ERR("BT disconneted and privacy lock is set");
				ecore_idler_add(_w_starter_lockscreen_idler_cb, ad);
			} else {
				ad->launcher_pid = w_launch_app(ad->home_pkgname, NULL);
				if (ad->launcher_pid > 0) {
					if (-1 == deviced_conf_set_mempolicy_bypid(ad->launcher_pid, OOM_IGNORE)) {
						_ERR("Cannot set the memory policy for Homescreen(%d)", ad->launcher_pid);
					} else {
						_ERR("Set the memory policy for Homescreen(%d)", ad->launcher_pid);
					}
				}
				else{
					_ERR("Launch Home failed.");
					ecore_idler_add(_w_retry_idler_cb, ad);
				}
			}
			create_key_window(ad->home_pkgname, ad);
#ifndef MODEM_ALWAYS_OFF
			w_launch_app(PWLOCK_PKGNAME, NULL);
#endif
		}
#endif

#endif //TELEPHONY_DISABLE
#ifdef TARGET
	}
#endif

	/* Check remote-lock state */
	if(vconf_get_bool(VCONFKEY_REMOTE_LOCK_ISLOCKED, &remote_lock) < 0){
		_E("failed to get %s", VCONFKEY_REMOTE_LOCK_ISLOCKED);
	}

	if(remote_lock == true){
		w_launch_app(REMOTE_LOCK_PKGNAME, NULL);
	}

//	create_key_window(ad->home_pkgname, ad);
	init_hourly_alert(ad);
	get_dbus_cool_down_mode(ad);
	init_dbus_COOL_DOWN_MODE_signal(ad);
	starter_dbus_connection_get();
	init_dbus_lcd_on_off_signal(ad);
	init_clock_mgr(ad);

	// THIS ROUTINE IS FOR SAT.
	vconf_set_int(VCONFKEY_IDLE_SCREEN_LAUNCHED, VCONFKEY_IDLE_SCREEN_LAUNCHED_TRUE);

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
	fini_clock_mgr();

	gettimeofday(&tv, NULL);
	timersub(&tv, &ad->tv_start, &res);
	_DBG("Total time: %d.%06d sec\n", (int)res.tv_sec, (int)res.tv_usec);
}

int main(int argc, char *argv[])
{
	struct appdata ad;

//	WRITE_FILE_LOG("%s", "Main function is started in starter");
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
