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



#include <ail.h>
#include <aul.h>
#include <db-util.h>
#include <Elementary.h>
#include <fcntl.h>
#include <stdio.h>
#include <sysman.h>
#include <syspopup_caller.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <vconf.h>
#include <errno.h>

#include "hw_key.h"
#include "pkg_event.h"
#include "util.h"
#include "xmonitor.h"


int errno;


#define QUERY_UPDATE_NAME "UPDATE app_info SET name='%s' where package='%s';"
#define SAT_DESKTOP_FILE "/opt/share/applications/org.tizen.sat-ui.desktop"
#define RELAUNCH_INTERVAL 100*1000
#define RETRY_MAXCOUNT 30

static struct info {
	pid_t home_pid;
	pid_t volume_pid;
	int power_off;
} s_info = {
	.home_pid = -1,
	.volume_pid = -1,
	.power_off = 0,
};



bool menu_daemon_is_homescreen(pid_t pid)
{
	if (s_info.home_pid == pid) return true;
	return false;
}



static inline char *_get_selected_pkgname(void)
{
	char *pkgname;

	pkgname = vconf_get_str(VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME);
	if (!pkgname) {
		_E("Cannot get pkgname from vconf.");

		pkgname = strdup(HOME_SCREEN_PKG_NAME);
		if (!pkgname) {
			_E("strdup error for pkgname, %s", strerror(errno));
			return NULL;
		}
	}

	return pkgname;
}



static inline void _open_homescreen(const char *pkgname)
{
	int ret;
	char *homescreen = (char *) pkgname;

	system("echo -e '[${_G}menu-daemon launches home-screen${C_}]' > /dev/kmsg");
	ret = aul_open_app(homescreen);
	_D("can%s launch %s now. (%d)", ret < 0 ? "not" : "", homescreen, ret);
	if (ret < 0 && strcmp(homescreen, HOME_SCREEN_PKG_NAME)) {
		_E("cannot launch package %s", homescreen);

		if (-1 == ret) {
			ret = aul_open_app(HOME_SCREEN_PKG_NAME);
			if (ret < 0) {
				_E("Failed to open a default home, %s(err:%d)", HOME_SCREEN_PKG_NAME, ret);
			}
		}
	}

	s_info.home_pid = ret;
	if (ret > 0) {
		if (-1 == sysconf_set_mempolicy_bypid(ret, OOM_IGNORE)) {
			_E("Cannot set the memory policy for Home-screen(%d)", ret);
		} else {
			_E("Set the memory policy for Home-screen(%d)", ret);
		}
	}
}



static void _show_cb(keynode_t* node, void *data)
{
	int seq;
	char *pkgname;

	_D("[MENU_DAEMON] _show_cb is invoked");

	pkgname = _get_selected_pkgname();
	if (!pkgname)
		return;

	if (node) {
		seq = vconf_keynode_get_int(node);
	} else {
		if (vconf_get_int(VCONFKEY_STARTER_SEQUENCE, &seq) < 0) {
			_E("Failed to get sequence info");
			free(pkgname);
			return;
		}
	}

	switch (seq) {
		case 0:
			if (s_info.home_pid > 0) {
				int pid;
				_D("pid[%d] is terminated.", s_info.home_pid);

				pid = s_info.home_pid;
				s_info.home_pid = -1;

				if (aul_terminate_pid(pid) != AUL_R_OK)
					_E("Failed to terminate %d", s_info.home_pid);
			}
			break;
		case 1:
			_open_homescreen(pkgname);
			break;
		default:
			_E("False sequence [%d]", seq);
			break;
	}

	free(pkgname);
	return;

}



static void _pkg_changed(keynode_t* node, void *data)
{
	char *pkgname;
	int seq;

	if (vconf_get_int(VCONFKEY_STARTER_SEQUENCE, &seq) < 0) {
		_E("Do nothing, there is no sequence info yet");
		return;
	}

	if (seq < 1) {
		_E("Sequence is not ready yet, do nothing");
		return;
	}

	_D("_pkg_changed is invoked");

	pkgname = _get_selected_pkgname();
	if (!pkgname)
		return;

	_D("pkg_name : %s", pkgname);

	if (s_info.home_pid > 0) {
		char old_pkgname[256];

		if (aul_app_get_pkgname_bypid(s_info.home_pid, old_pkgname, sizeof(old_pkgname)) == AUL_R_OK) {
			if (!strcmp(pkgname, old_pkgname)) {
				_D("Package is changed but same package is selected");
				free(pkgname);
				return;
			}
		}

		if (aul_terminate_pid(s_info.home_pid) != AUL_R_OK)
			_D("Failed to terminate pid %d", s_info.home_pid);
	} else {
		_open_homescreen(pkgname);
	}

	free(pkgname);
	return;
}

static void _launch_volume(void)
{
	int pid;
	int i;
	_D("_launch_volume");

	for (i=0; i<RETRY_MAXCOUNT; i++)
	{
		pid = syspopup_launch("volume", NULL);

		_D("syspopup_launch(volume), pid = %d", pid);

		if (pid <0) {
			_D("syspopup_launch(volume)is failed [%d]times", i);
			usleep(RELAUNCH_INTERVAL);
		} else {
			s_info.volume_pid = pid;
			return;
		}
	}
}

int menu_daemon_check_dead_signal(int pid)
{
	char *pkgname;

	if (s_info.power_off) {
		_D("Power off. ignore dead cb\n");
		return 0;
	}

	_D("Process %d is termianted", pid);

	if (pid < 0)
		return 0;

	pkgname = _get_selected_pkgname();
	if (!pkgname)
		return 0;

	if (pid == s_info.home_pid) {
		_D("pkg_name : %s", pkgname);
		_open_homescreen(pkgname);
	} else if (pid == s_info.volume_pid) {
		_launch_volume();
	} else {
		_D("Unknown process, ignore it (pid %d, home pid %d)",
				pid, s_info.home_pid);
	}

	free(pkgname);

	return 0;
}



void menu_daemon_init(void *data)
{
	_D( "[MENU_DAEMON]menu_daemon_init is invoked");

	aul_launch_init(NULL,NULL);

	create_key_window();
	if (xmonitor_init() < 0) _E("cannot init xmonitor");

	pkg_event_init();
	_launch_volume();

	if (unlink(SAT_DESKTOP_FILE) != 0)
		_E("cannot remove sat-ui desktop.");

	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME, _pkg_changed, NULL) < 0)
		_E("Failed to add callback for package change event");

	if (vconf_notify_key_changed(VCONFKEY_STARTER_SEQUENCE, _show_cb, NULL) < 0)
		_E("Failed to add callback for show event");

	_pkg_changed(NULL, NULL);
	vconf_set_int(VCONFKEY_IDLE_SCREEN_LAUNCHED, VCONFKEY_IDLE_SCREEN_LAUNCHED_TRUE);
}



void menu_daemon_fini(void)
{
	vconf_ignore_key_changed(VCONFKEY_STARTER_SEQUENCE, _show_cb);
	vconf_ignore_key_changed(VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME, _pkg_changed);

	xmonitor_fini();
	pkg_event_fini();
	destroy_key_window();
}
