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

#include "starter.h"
#include "x11.h"
#include "lock-daemon.h"
#include "lockd-debug.h"

#ifndef PACKAGE_NAME
#define PACKAGE_NAME "org.tizen.starter"
#endif

#define PWLOCK_PKG "org.tizen.pwlock"
#define VCONFKEY_START "memory/startapps/sequence"
#define PWLOCK_FIRST_BOOT "db/setting/pwlock_boot"
#define DEFAULT_THEME "tizen"
#define PWLOCK_PATH "/opt/apps/org.tizen.pwlock/bin/pwlock"
#define HIB_CAPTURING "/opt/etc/.hib_capturing"

static void lock_menu_screen(void)
{
	vconf_set_int(VCONFKEY_START, 0);
}

static void unlock_menu_screen(void)
{
	int r;
	int show_menu;

	show_menu = 0;
	r = vconf_get_int(VCONFKEY_START, &show_menu);
	if (r || !show_menu) {
		vconf_set_int(VCONFKEY_START, 1);
	}
}

static void _set_elm_theme(void)
{
	char *vstr;
	char *theme;
	vstr = vconf_get_str(VCONFKEY_SETAPPL_WIDGET_THEME_STR);
	if (vstr == NULL)
		theme = DEFAULT_THEME;
	else
		theme = vstr;

	_DBG("theme vconf[%s]\n set[%s]\n", vstr, theme);
	elm_theme_all_set(theme);

	if (vstr)
		free(vstr);
}

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

static int _launch_pwlock(void)
{
	int r;

	_DBG("%s", __func__);

	r = aul_launch_app(PWLOCK_PKG, NULL);
	if (r < 0) {
		_ERR("PWLock launch error: error(%d)", r);
		if (r == AUL_R_ETIMEOUT) {
			_DBG("Launch pwlock is failed for AUL_R_ETIMEOUT, again launch pwlock");
			r = aul_launch_app(PWLOCK_PKG, NULL);
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
}

static void hib_leave(void *data)
{
	struct appdata *ad = data;
	if (ad == NULL) {
		fprintf(stderr, "Invalid argument: appdata is NULL\n");
		return;
	}

	_DBG("%s", __func__);
	_set_elm_theme();
	start_lock_daemon();
	if (_launch_pwlock() < 0) {
		_ERR("launch pwlock error");
	}
}

static int add_noti(struct appdata *ad)
{
	int fd;
	int r;
	_DBG("%s %d\n", __func__, __LINE__);

	if (ad == NULL) {
		fprintf(stderr, "Invalid argument: appdata is NULL\n");
		return -1;
	}

	fd = heynoti_init();
	if (fd == -1) {
		_ERR("Noti init error");
		return -1;
	}
	ad->noti = fd;

	r = heynoti_subscribe(fd, "HIBERNATION_PRELEAVE", hib_leave, ad);
	if (r == -1) {
		_ERR("Noti subs error");
		return -1;
	}

	r = heynoti_attach_handler(fd);
	if (r == -1) {
		_ERR("Noti attach error");
		return -1;
	}

	_DBG("Waiting for hib leave");
	_DBG("%s %d\n", __func__, __LINE__);

	return 0;
}

static int _init(struct appdata *ad)
{
	int fd;
	int r;

	memset(ad, 0, sizeof(struct appdata));

	ad->noti = -1;
	gettimeofday(&ad->tv_start, NULL);

	lock_menu_screen();
	_set_elm_theme();
	_set_elm_entry();

	_DBG("%s %d\n", __func__, __LINE__);
	fd = open(HIB_CAPTURING, O_RDONLY);
	_DBG("fd = %d\n", fd);
	if (fd == -1) {
		_DBG("fd = %d\n", fd);
		start_lock_daemon();
		if (_launch_pwlock() < 0) {
			_ERR("launch pwlock error");
		}
		r = 0;
	} else {
		close(fd);
		r = add_noti(ad);
	}

	if (vconf_set_int("memory/hibernation/starter_ready", 1))
		_ERR("vconf_set_int FAIL");
	else
		_ERR("vconf_set_int OK\n");

	return r;
}

static void _fini(struct appdata *ad)
{
	struct timeval tv, res;

	if (ad == NULL) {
		fprintf(stderr, "Invalid argument: appdata is NULL\n");
		return;
	}
	if (ad->noti != -1)
		heynoti_close(ad->noti);

	unlock_menu_screen();

	gettimeofday(&tv, NULL);
	timersub(&tv, &ad->tv_start, &res);
	_DBG("Total time: %d.%06d sec\n", (int)res.tv_sec, (int)res.tv_usec);
}

int main(int argc, char *argv[])
{
	struct appdata ad;

	set_window_scale();

	elm_init(argc, argv);

	_init(&ad);

	elm_run();

	_fini(&ad);

	elm_shutdown();

	return 0;
}
