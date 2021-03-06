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
#include "x11.h"
#include "lock-daemon.h"
#include "lockd-debug.h"

#define DEFAULT_THEME "tizen"

#define HIB_CAPTURING "/opt/etc/.hib_capturing"
#define STR_STARTER_READY "/tmp/hibernation/starter_ready"

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
static int _launch_pwlock(void)
{
	int r;

	_DBG("%s", __func__);

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

static int _init(struct appdata *ad)
{
	int fd;
	int r;
	int fd1;

	struct sigaction act;
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

	ad->noti = -1;
	gettimeofday(&ad->tv_start, NULL);

	lock_menu_screen();
	_set_elm_theme();

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

	fd1 = open(STR_STARTER_READY, O_RDWR | O_CREAT, S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH);
	if (fd1 > 0) {
		_DBG("Hibernation ready.\n");
		close(fd1);
	}

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
