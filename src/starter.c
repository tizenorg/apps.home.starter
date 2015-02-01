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

#include <aul.h>
#include <vconf.h>
#include <signal.h>
//#include <bincfg.h>

#include "starter.h"
#include "starter-util.h"
#include "x11.h"
#include "lock-daemon.h"
#include "lockd-debug.h"
#include "menu_daemon.h"

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "com.samsung.starter"
#endif

#define DEFAULT_THEME "tizen"
#define PWLOCK_PATH "/usr/apps/com.samsung.pwlock/bin/pwlock"
#define PWLOCK_PKG_NAME "com.samsung.pwlock"
#define PWLOCK_LITE_PKG_NAME "com.samsung.pwlock-lite"

#define DATA_UNENCRYPTED "unencrypted"
#define DATA_MOUNTED "mounted"
#define SD_DATA_ENCRYPTED "encrypted"
#define SD_CRYPT_META_FILE ".MetaEcfsFile"
#define MMC_MOUNT_POINT	"/opt/storage/sdcard"

#ifdef FEATURE_LITE
#define _FIRST_HOME 1
#else
#define _FIRST_HOME 1
#endif

static int _check_encrypt_sdcard()
{
	int ret = 0;
	struct stat src_stat;
	const char * cryptTempFile = SD_CRYPT_META_FILE;
	char *mMetaDataFile;

	mMetaDataFile = malloc(strlen (MMC_MOUNT_POINT) + strlen (cryptTempFile) +2);
	if (mMetaDataFile)
	{
		sprintf (mMetaDataFile, "%s%s%s", MMC_MOUNT_POINT, "/", cryptTempFile);
		if (lstat (mMetaDataFile, &src_stat) < 0)
			if (errno == ENOENT)
				ret = -1;
		free(mMetaDataFile);
	}
	_DBG("check sd card ecryption : %d", ret);
	return ret;
}

static void lock_menu_screen(void)
{
	vconf_set_int(VCONFKEY_STARTER_SEQUENCE, 0);
}

static void unlock_menu_screen(void)
{
	int r;
	int show_menu;

	show_menu = 0;
	r = vconf_get_int(VCONFKEY_STARTER_SEQUENCE, &show_menu);
	if (r || !show_menu) {
		vconf_set_int(VCONFKEY_STARTER_SEQUENCE, 1);
	}
}

static void _set_elm_theme(void)
{
	char *vstr;
	char *theme;
	Elm_Theme *th = NULL;
	vstr = vconf_get_str(VCONFKEY_SETAPPL_WIDGET_THEME_STR);
	if (vstr == NULL)
		theme = DEFAULT_THEME;
	else
		theme = vstr;

	th = elm_theme_new();
	_DBG("theme vconf[%s]\n set[%s]\n", vstr, theme);
	elm_theme_set(th, theme);

	if (vstr)
		free(vstr);
}

#if 0
static void _set_elm_entry(void)
{
	int v;
	int r;

	r = vconf_get_bool(VCONFKEY_SETAPPL_AUTOCAPITAL_ALLOW_BOOL, &v);
	if (!r) {
		prop_int_set("ENLIGHTENMENT_AUTOCAPITAL_ALLOW", v);
		_DBG("vconf autocatipal[%d]", v);
	}

	r = vconf_get_bool(VCONFKEY_SETAPPL_AUTOPERIOD_ALLOW_BOOL, &v);
	if (!r) {
		prop_int_set("ENLIGHTENMENT_AUTOPERIOD_ALLOW", v);
		_DBG("vconf autoperiod[%d]", v);
	}
}
#endif

static int _launch_pwlock(void)
{
	int r;
	//int i = 0;

	_DBG("%s", __func__);

#ifdef FEATURE_LITE
	r = aul_launch_app(PWLOCK_LITE_PKG_NAME, NULL);
	if (r < 0) {
		_ERR("PWLock launch error: error(%d)", r);
		if (r == AUL_R_ETIMEOUT) {
			_DBG("Launch pwlock is failed for AUL_R_ETIMEOUT, again launch pwlock-lite");
			r = aul_launch_app(PWLOCK_LITE_PKG_NAME, NULL);
			if (r < 0) {
				_ERR("2'nd PWLock launch error: error(%d)", r);
				return -1;
			} else {
				_DBG("Launch pwlock-lite");
				return r;
			}
		} else {
			return -1;
		}
	} else {
		_DBG("Launch pwlock-lite");
		return r;
	}
#else
	r = aul_launch_app(PWLOCK_LITE_PKG_NAME, NULL);
	if (r < 0) {
		_ERR("PWLock launch error: error(%d)", r);
		if (r == AUL_R_ETIMEOUT) {
			_DBG("Launch pwlock is failed for AUL_R_ETIMEOUT, again launch pwlock");
			r = aul_launch_app(PWLOCK_LITE_PKG_NAME, NULL);
			if (r < 0) {
				_ERR("2'nd PWLock launch error: error(%d)", r);
				return -1;
			} else {
				_DBG("Launch pwlock");
				return r;
			}
		} else {
			return -1;
		}
	} else {
		_DBG("Launch pwlock");
		return r;
	}
#endif
#if 0
 retry_con:
	r = aul_launch_app("com.samsung.pwlock", NULL);
	if (r < 0) {
		_ERR("PWLock launch error: error(%d)", r);
		if (r == AUL_R_ETIMEOUT) {
			i++;
			_DBG("Launching pwlock is failed [%d]times for AUL_R_ETIMEOUT ", i);
			goto retry_con;
		} else {
			return -1;
		}
	} else {
		_DBG("Launch pwlock");
		return 0;
	}
#endif
}

static void _signal_handler(int signum, siginfo_t *info, void *unused)
{
    _DBG("_signal_handler : Terminated...");
    elm_exit();
}

#if 0
static void _heynoti_event_power_off(void *data)
{
    _DBG("_heynoti_event_power_off : Terminated...");
    elm_exit();
}
#endif

static void _poweroff_control_cb(keynode_t *in_key, void *data)
{
	int val;
	if (vconf_get_int(VCONFKEY_SYSMAN_POWER_OFF_STATUS, &val) == 0 &&
		(val == VCONFKEY_SYSMAN_POWER_OFF_DIRECT || val == VCONFKEY_SYSMAN_POWER_OFF_RESTART)) {
	    _DBG("_poweroff_control_cb : Terminated...");
	    elm_exit();
	}
}

#define APP_ID_SPLIT_LAUNCHER "com.samsung.split-launcher"
static Eina_Bool _fini_boot(void *data)
{
	_DBG("%s %d\n", __func__, __LINE__);

	int multiwindow_enabled = 0;
	int val = 0;

	if (vconf_set_int(VCONFKEY_BOOT_ANIMATION_FINISHED, 1) != 0) {
		_ERR("Failed to set boot animation finished set");
	}

	if (vconf_get_bool(VCONFKEY_QUICKSETTING_MULTIWINDOW_ENABLED, &multiwindow_enabled) < 0) {
		_ERR("Cannot get VCONFKEY");
		multiwindow_enabled = 0;
	}

	if (vconf_get_bool(VCONFKEY_SETAPPL_ACCESSIBILITY_TTS, &val) < 0) {
		_DBG("Cannot get VCONFKEY");
		val = 0;
	}

	if ((val == 1) || (multiwindow_enabled == 0)) {
		_DBG("TTS : %d, Multiwindow enabled : %d", val, multiwindow_enabled);
		return ECORE_CALLBACK_CANCEL;
	}

	_DBG("Launch the split-launcher");

	int ret = aul_launch_app(APP_ID_SPLIT_LAUNCHER, NULL);
	if (0 > ret) _ERR("cannot launch the split-launcher (%d)", ret);

	return ECORE_CALLBACK_CANCEL;
}


static Eina_Bool _init_idle(void *data)
{
	_DBG("%s %d\n", __func__, __LINE__);
	int pwlock_pid = 0;
#ifdef FEATURE_SDK
	_fini_boot(NULL);
#else
	if ((pwlock_pid = _launch_pwlock()) < 0) {
		_ERR("launch pwlock error");
	}
	else{
		lockd_process_mgr_set_pwlock_priority(pwlock_pid);
	}
	_fini_boot(NULL);
#endif
	return ECORE_CALLBACK_CANCEL;
}

static void _lock_state_cb(keynode_t * node, void *data)
{
	_DBG("%s %d\n", __func__, __LINE__);
#if 0
	if (_launch_pwlock() < 0) {
		_ERR("launch pwlock error");
	}
	menu_daemon_init(NULL);
#else
	_fini_boot(NULL);
#endif
	if (vconf_ignore_key_changed(VCONFKEY_IDLE_LOCK_STATE,
	     _lock_state_cb) != 0) {
		LOCKD_DBG("Fail to unregister");
	}
}

static Eina_Bool _init_lock_lite(void *data)
{
#ifdef FEATURE_SDK
	if (start_lock_daemon_lite(TRUE, FALSE) == 1) {
		if (vconf_notify_key_changed(VCONFKEY_IDLE_LOCK_STATE,
					_lock_state_cb, NULL) != 0) {
			_ERR("[Error] vconf notify : lock state");
			ecore_timer_add(0.5, _fini_boot, NULL);
		}
	} else{
		_init_idle(NULL);
	}
#else
	char *file = NULL;
	int pwlock_pid = 0;

	_DBG("%s %d\n", __func__, __LINE__);

	/* Check SD card encription */
	file = vconf_get_str(VCONFKEY_SDE_CRYPTO_STATE);
	if (file && !strncmp(SD_DATA_ENCRYPTED, file, strlen(file)) && _check_encrypt_sdcard() == 0) {
		_ERR("SD card is encripted");
		if (start_lock_daemon_lite(FALSE, FALSE) == 1) {
			if ((pwlock_pid = _launch_pwlock()) < 0) {
				_ERR("launch pwlock error");
			}
			else{
				lockd_process_mgr_set_pwlock_priority(pwlock_pid);
			}
			ecore_timer_add(0.5, _fini_boot, NULL);
		} else {
			_init_idle(NULL);
		}
	} else {
		if (start_lock_daemon_lite(TRUE, FALSE) == 1) {
			if ((pwlock_pid = _launch_pwlock()) < 0) {
				_ERR("launch pwlock error");
			}
			else{
				lockd_process_mgr_set_pwlock_priority(pwlock_pid);
			}
			if (vconf_notify_key_changed(VCONFKEY_IDLE_LOCK_STATE,
						_lock_state_cb, NULL) != 0) {
				_ERR("[Error] vconf notify : lock state");
				ecore_timer_add(0.5, _fini_boot, NULL);
			}
		} else {
			_init_idle(NULL);
		}
	}
	free(file);
#endif
	return ECORE_CALLBACK_CANCEL;
}

static Eina_Bool _init_lock(void *data)
{
	char *file = NULL;

	_DBG("%s %d\n", __func__, __LINE__);

	/* Check SD card encription */
	file = vconf_get_str(VCONFKEY_SDE_CRYPTO_STATE);
	if (file && !strncmp(SD_DATA_ENCRYPTED, file, strlen(file)) && _check_encrypt_sdcard() == 0) {
		_ERR("SD card is encripted");
		if (start_lock_daemon(FALSE, FALSE) == 1) {
			if (_launch_pwlock() < 0) {
				_ERR("launch pwlock error");
			}
			ecore_timer_add(0.5, _fini_boot, NULL);
		} else {
			_init_idle(NULL);
		}
	} else {
		if (start_lock_daemon(TRUE, FALSE) == 1) {
			if (_launch_pwlock() < 0) {
				_ERR("launch pwlock error");
			}
			if (vconf_notify_key_changed(VCONFKEY_IDLE_LOCK_STATE,
						_lock_state_cb, NULL) != 0) {
				_ERR("[Error] vconf notify : lock state");
				ecore_timer_add(0.5, _fini_boot, NULL);
			}
		} else {
			_init_idle(NULL);
		}
	}
	free(file);
	return ECORE_CALLBACK_CANCEL;
}

static void _data_encryption_cb(keynode_t * node, void *data)
{
	char *file = NULL;

	_DBG("%s %d\n", __func__, __LINE__);

	file = vconf_get_str(VCONFKEY_ODE_CRYPTO_STATE);
	if (file != NULL) {
		_DBG("get VCONFKEY : %s\n",  file);
		if (!strcmp(file, DATA_MOUNTED)) {
#ifdef FEATURE_LITE
			start_lock_daemon_lite(FALSE, FALSE);
#else
			start_lock_daemon(FALSE, FALSE);
#endif
			menu_daemon_init(NULL);
		}
		free(file);
	}
}

static Eina_Bool _start_sequence_cb(void *data)
{
	_DBG("%s, %d", __func__, __LINE__);

	unlock_menu_screen();

	return ECORE_CALLBACK_CANCEL;
}

static void _init(struct appdata *ad)
{
	int r;
	struct sigaction act;
	char *file = NULL;
	int first = -1;
	int pwlock_pid = 0;

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

#ifdef FEATURE_SDK
	_DBG("Emulator booting sequence");
	lock_menu_screen();

	//_FIRST_HOME
	_DBG("First home");
#ifdef FEATURE_LITE
	_init_lock_lite(NULL);
#else
	_init_lock(NULL);
#endif
	menu_daemon_init(NULL);
	ecore_idler_add(_start_sequence_cb, NULL);

#else	/* Target binary */
/*	if (bincfg_is_factory_binary() == 1) {
		_DBG("Factory binary..!!");
		_set_elm_theme();
		unlock_menu_screen();
		menu_daemon_init(NULL);
	} else {
*/		_DBG("%s %d\n", __func__, __LINE__);
		lock_menu_screen();
		_set_elm_theme();

		/* Check data encrption */
		file = vconf_get_str(VCONFKEY_ODE_CRYPTO_STATE);
		if (file != NULL) {
			_DBG("get VCONFKEY : %s\n",  file);
			if (strncmp(DATA_UNENCRYPTED, file, strlen(file))) {
				if (vconf_notify_key_changed(VCONFKEY_ODE_CRYPTO_STATE,
							_data_encryption_cb, NULL) != 0) {
					_ERR("[Error] vconf notify changed is failed: %s", VCONFKEY_ODE_CRYPTO_STATE);
				} else {
					_DBG("waiting mount..!!");
					if ((pwlock_pid = _launch_pwlock()) < 0) {
						_ERR("launch pwlock error");
					}
					else{
						lockd_process_mgr_set_pwlock_priority(pwlock_pid);
					}
					free(file);
					return;
				}
			}
			free(file);
		}

#if 0
		/* Check SD card encription */
		file = vconf_get_str(VCONFKEY_SDE_CRYPTO_STATE);
		if (file && !strncmp(SD_DATA_ENCRYPTED, file, strlen(file)) && _check_encrypt_sdcard() == 0) {
		} else {
		}
#endif
		/* change the launching order of pwlock and lock mgr in booting time */
		/* TODO: menu screen is showed before phone lock is showed in booting time */
		/* FIXME Here lock daemon start..!! */
#if 0
		r = start_lock_daemon(TRUE);
		if (r == 1) {
			if (vconf_notify_key_changed(VCONFKEY_IDLE_LOCK_STATE,
						_lock_state_cb, NULL) != 0) {
				_ERR("[Error] vconf notify : lock state");
				ecore_timer_add(1.5, _init_idle, NULL);
			}
		} else {
			if (_launch_pwlock() < 0) {
				_ERR("launch pwlock error");
			}
			menu_daemon_init(NULL);
		}
#else

		r = vconf_get_bool(VCONFKEY_PWLOCK_FIRST_BOOT, &first);
		_ERR("vconf get First boot result:%d, get_value:%d", r, first);
		if (r == 0 && first == 0){
			_DBG("Not first booting time");

#if (!_FIRST_HOME)
			_DBG("Not First home");
			unlock_menu_screen();
			menu_daemon_init(NULL);
#ifdef FEATURE_LITE
			_init_lock_lite(NULL);
#else
			_init_lock(NULL);
#endif
#else //_FIRST_HOME
			_DBG("First home");
#ifdef FEATURE_LITE
			_init_lock_lite(NULL);
#else
			_init_lock(NULL);
#endif
			menu_daemon_init(NULL);
			ecore_idler_add(_start_sequence_cb, NULL);
#endif
		} else {
			_ERR("First booting time");
			menu_daemon_init(NULL);
#ifdef FEATURE_LITE
			r = start_lock_daemon_lite(TRUE, TRUE);
#else
			r = start_lock_daemon(TRUE, TRUE);
#endif
			_DBG("start_lock_daemon ret:%d", r);
			if ((pwlock_pid = _launch_pwlock()) < 0) {
				_ERR("launch pwlock error");
				if (vconf_set_int(VCONFKEY_BOOT_ANIMATION_FINISHED, 1) != 0) {
					_ERR("Failed to set boot animation finished set");
				}
				unlock_menu_screen();
			} else {
				lockd_process_mgr_set_pwlock_priority(pwlock_pid);
				ecore_timer_add(1, _fini_boot, NULL);
			}
		}
#endif
//	}
#endif  /*end of sdk feature */
}

static void _fini(struct appdata *ad)
{
	struct timeval tv, res;

	if (ad == NULL) {
		fprintf(stderr, "Invalid argument: appdata is NULL\n");
		return;
	}

	unlock_menu_screen();
	menu_daemon_fini();

	gettimeofday(&tv, NULL);
	timersub(&tv, &ad->tv_start, &res);
	_DBG("Total time: %d.%06d sec\n", (int)res.tv_sec, (int)res.tv_usec);
}

int main(int argc, char *argv[])
{
	struct appdata ad;

	_DBG("starter is launched..!!");
#if 0
	set_window_scale();	/* not in loop */
#endif

#if 0
    int heyfd = heynoti_init();
	if (heyfd < 0) {
		_ERR("Failed to heynoti_init[%d]", heyfd);
		return -1;
	}

	int ret = heynoti_subscribe(heyfd, "power_off_start", _heynoti_event_power_off, NULL);
	if (ret < 0) {
		_ERR("Failed to heynoti_subscribe[%d]", ret);
	}
	ret = heynoti_attach_handler(heyfd);
	if (ret < 0) {
		_ERR("Failed to heynoti_attach_handler[%d]", ret);
	}
#else
	if (vconf_notify_key_changed(VCONFKEY_SYSMAN_POWER_OFF_STATUS, (void *)_poweroff_control_cb, NULL) < 0) {
		_ERR("Vconf notify key chaneged failed: VCONFKEY_SYSMAN_POWER_OFF_STATUS");
	}
#endif

	elm_init(argc, argv);

	_init(&ad);

	elm_run();

	_fini(&ad);

	elm_shutdown();

	return 0;
}
