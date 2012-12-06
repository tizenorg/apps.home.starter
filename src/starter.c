 /*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.tizenopensource.org/license
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
#include <heynoti.h>
#include <signal.h>


#include "starter.h"
#include "starter-util.h"
#include "x11.h"
#include "lock-daemon.h"
#include "lockd-debug.h"
#include "menu_daemon.h"

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "org.tizen.starter"
#endif

#define DEFAULT_THEME "tizen"
#define PWLOCK_PATH "/usr/apps/org.tizen.pwlock/bin/pwlock"

#define DATA_ENCRYPTED "encrypted"
#define DATA_MOUNTED "mounted"

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

#if 1
	r = aul_launch_app("org.tizen.pwlock", NULL);
	if (r < 0) {
		_ERR("PWLock launch error: error(%d)", r);
		if (r == AUL_R_ETIMEOUT) {
			_DBG("Launch pwlock is failed for AUL_R_ETIMEOUT, again launch pwlock");
			r = aul_launch_app("org.tizen.pwlock", NULL);
			if (r < 0) {
				_ERR("2'nd PWLock launch error: error(%d)", r);
				return -1;
			} else {
				_DBG("Launch pwlock");
				return 0;
			}
		} else {
			return -1;
		}
	} else {
		_DBG("Launch pwlock");
		return 0;
	}
#else
 retry_con:
	r = aul_launch_app("org.tizen.pwlock", NULL);
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

static void _heynoti_event_power_off(void *data)
{
    _DBG("_heynoti_event_power_off : Terminated...");
    elm_exit();
}

static Eina_Bool _init_idle(void *data)
{
	_DBG("%s %d\n", __func__, __LINE__);
	if (_launch_pwlock() < 0) {
		_ERR("launch pwlock error");
	}
	menu_daemon_init(NULL);

	return ECORE_CALLBACK_CANCEL;
}

static void _lock_state_cb(keynode_t * node, void *data)
{
	_DBG("%s %d\n", __func__, __LINE__);
	WRITE_FILE_LOG("%s", "Lock state is changed!");

	if (_launch_pwlock() < 0) {
		_ERR("launch pwlock error");
	}
	menu_daemon_init(NULL);
	if (vconf_ignore_key_changed(VCONFKEY_IDLE_LOCK_STATE,
	     _lock_state_cb) != 0) {
		LOCKD_DBG("Fail to unregister");
	}
}

static void _data_encryption_cb(keynode_t * node, void *data)
{
	int r;
	char *file = NULL;

	_DBG("%s %d\n", __func__, __LINE__);

	file = vconf_get_str(VCONFKEY_ODE_CRYPTO_STATE);
	if (file != NULL) {
		_DBG("get %s : %s\n", VCONFKEY_ODE_CRYPTO_STATE, file);
		if (!strcmp(file, DATA_MOUNTED)) {
			start_lock_daemon(FALSE);
			menu_daemon_init(NULL);
		}
		free(file);
	}
}

static void _init(struct appdata *ad)
{
	int r;
	struct sigaction act;
	char *file = NULL;

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

	lock_menu_screen();
	_set_elm_theme();

	_DBG("%s %d\n", __func__, __LINE__);

	/* Check data encrption */
	file = vconf_get_str(VCONFKEY_ODE_CRYPTO_STATE);
	if (file != NULL) {
		_DBG("get %s : %s\n", VCONFKEY_ODE_CRYPTO_STATE, file);
		if (!strcmp(file, DATA_ENCRYPTED)) {
			if (vconf_notify_key_changed(VCONFKEY_ODE_CRYPTO_STATE,
			     _data_encryption_cb, NULL) != 0) {
				_ERR("[Error] vconf notify changed : %s", VCONFKEY_ODE_CRYPTO_STATE);
			} else {
				_DBG("waiting mount..!!");
				if (_launch_pwlock() < 0) {
					_ERR("launch pwlock error");
				}
				free(file);
				return;
			}
		}
		free(file);
	}

	/* change the launching order of pwlock and lock mgr in booting time */
	/* TODO: menu screen is showed before phone lock is showed in booting time */
	/* FIXME Here lock daemon start..!! */
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

	WRITE_FILE_LOG("%s", "Main function is started in starter");
#if 0
	set_window_scale();	/* not in loop */
#endif
    int heyfd = heynoti_init();
	if (heyfd < 0) {
		_ERR("Failed to heynoti_init[%d]", heyfd);
	}

	int ret = heynoti_subscribe(heyfd, "power_off_start", _heynoti_event_power_off, NULL);
	if (ret < 0) {
		_ERR("Failed to heynoti_subscribe[%d]", ret);
	}
	ret = heynoti_attach_handler(heyfd);
	if (ret < 0) {
		_ERR("Failed to heynoti_attach_handler[%d]", ret);
	}

	elm_init(argc, argv);

	_init(&ad);

	elm_run();

	_fini(&ad);

	elm_shutdown();

	return 0;
}
