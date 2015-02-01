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

#include <ail.h>
#include <aul.h>
#include <app.h>
#include <db-util.h>
#include <Elementary.h>
#include <errno.h>
#include <fcntl.h>
#include <pkgmgr-info.h>
#include <stdio.h>
#include <dd-deviced.h>
#include <syspopup_caller.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <vconf.h>
//#include <bincfg.h>

#include "hw_key.h"
#include "util.h"
//#include "xmonitor.h"
#include "dbus-util.h"

int errno;


#define QUERY_UPDATE_NAME "UPDATE app_info SET name='%s' where package='%s';"
#define RELAUNCH_INTERVAL 100*1000
#define RETRY_MAXCOUNT 30
#if !defined(VCONFKEY_SVOICE_PACKAGE_NAME)
#define VCONFKEY_SVOICE_PACKAGE_NAME "db/svoice/package_name"
#endif
#define HOME_TERMINATED "home_terminated"
#define ISTRUE "TRUE"

#define RELAUNCH_TASKMGR 0

// Define prototype of the "hidden API of AUL"
//extern int aul_listen_app_dead_signal(int (*func)(int signal, void *data), void *data);



static struct info {
	pid_t home_pid;
	pid_t tray_pid;
	pid_t taskmgr_pid;
	pid_t volume_pid;
	int safe_mode;
	int cradle_status;
	int pm_key_ignore;
	int power_off;
	char *svoice_pkg_name;
	char *taskmgr_pkg_name;
} s_info = {
	.home_pid = (pid_t)-1,
	.tray_pid = (pid_t)-1,
	.taskmgr_pid = (pid_t)-1,
	.volume_pid = (pid_t)-1,
	.safe_mode = -1,
	.cradle_status = -1,
	.pm_key_ignore = -1,
	.power_off = 0,
	.svoice_pkg_name = NULL,
	.taskmgr_pkg_name = NULL,
};



inline char *menu_daemon_get_selected_pkgname(void)
{
	char *pkgname = NULL;

	pkgname = vconf_get_str(VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME);
	retv_if(NULL == pkgname, NULL);

	return pkgname;
}

char *menu_daemon_get_taskmgr_pkgname(void)
{
	return s_info.taskmgr_pkg_name;
}


#define VCONFKEY_IDLE_SCREEN_SAFEMODE "memory/idle-screen/safemode"
int menu_daemon_is_safe_mode(void)
{
	if (-1 == s_info.safe_mode) {
		if (vconf_get_int(VCONFKEY_IDLE_SCREEN_SAFEMODE, &s_info.safe_mode) < 0) {
			_E("Failed to get safemode info");
			s_info.safe_mode = 0;
		}
	}

	return s_info.safe_mode;
}



#define VCONFKEY_SYSMAN_CRADLE_STATUS "memory/sysman/cradle_status"
int menu_daemon_get_cradle_status(void)
{
	if (-1 == s_info.cradle_status) {
		if (vconf_get_int(VCONFKEY_SYSMAN_CRADLE_STATUS, &s_info.cradle_status) < 0) {
			_E("Failed to get cradle status");
			s_info.cradle_status = 0;
		}
	}

	return s_info.cradle_status;
}



#define RETRY_COUNT 5
int menu_daemon_open_app(const char *pkgname)
{
	register int i;
	int r = AUL_R_OK;
	for (i = 0; i < RETRY_COUNT; i ++) {
		_D("pkgname: %s", pkgname);
		r = aul_open_app(pkgname);
		if (0 <= r) return r;
		else {
			_D("aul_open_app error(%d)", r);
			_F("cannot open an app(%s) by %d", pkgname, r);
		}
		usleep(500000);
	}

	return r;
}



int menu_daemon_launch_app(const char *pkgname, bundle *b)
{
	register int i;
	int r = AUL_R_OK;
	pkgmgrinfo_appinfo_filter_h handle = NULL;
	bool enabled = false;

	if (PMINFO_R_OK != pkgmgrinfo_appinfo_get_appinfo(pkgname, &handle)) {
		_E("Cannot get the pkginfo");
		return -77;
	}
	if (PMINFO_R_OK != pkgmgrinfo_appinfo_is_enabled(handle, &enabled)) {
		_E("Cannot get if app is enabled or not");
		pkgmgrinfo_appinfo_destroy_appinfo(handle);
		return -77;
	}

	if (enabled) {
		for (i = 0; i < RETRY_COUNT; i ++) {
			r = aul_launch_app(pkgname, b);
			if (0 <= r){
				pkgmgrinfo_appinfo_destroy_appinfo(handle);
				return r;
			}
			else {
				_D("aul_launch_app error(%d)", r);
				_F("cannot launch an app(%s) by %d", pkgname, r);
			}
			usleep(500000);
		}
	}
	else {
		_SECURE_E("%s is disabled, so can't launch this", pkgname);
		r = -77;
	}

	pkgmgrinfo_appinfo_destroy_appinfo(handle);
	return r;
}



void menu_daemon_launch_app_tray(void)
{
	bundle *kb = NULL;
	char *pkgname = NULL;

	_D("launch App-tray");

	pkgname = menu_daemon_get_selected_pkgname();
	if (!pkgname)
		return;
	_SECURE_D("home pkg_name : %s", pkgname);

	//Check preloaded Homescreen
	if (!strncmp(pkgname, EASY_HOME_PKG_NAME, strlen(pkgname))) {
		pkgname = EASY_APPS_PKG_NAME;
	} else if (!strncmp(pkgname, HOMESCREEN_PKG_NAME, strlen(pkgname))) {
		pkgname = HOMESCREEN_PKG_NAME;
	}
	_SECURE_D("apps pkg_name : %s", pkgname);


	kb = bundle_create();
	bundle_add(kb, "HIDE_LAUNCH", "0");
	bundle_add(kb, "SHOW_APPS", "TRUE");

	s_info.tray_pid = menu_daemon_launch_app(pkgname, kb);
	if (s_info.tray_pid < 0) {
		_SECURE_E("Failed to reset %s", pkgname);
	} else if (s_info.tray_pid > 0) {
		if(starter_dbus_set_oomadj(s_info.tray_pid, OOM_ADJ_VALUE_HOMESCREEN) < 0){
			_E("failed to send oom dbus signal");
		}
	}

	bundle_free(kb);
}



bool menu_daemon_is_homescreen(pid_t pid)
{
	if (s_info.home_pid == pid) return true;
	return false;
}




static inline void _hide_launch_task_mgr(void)
{
	bundle *kb = NULL;

	_D("Hide to launch Taskmgr");
	kb = bundle_create();
	bundle_add(kb, "HIDE_LAUNCH", "1");

	s_info.taskmgr_pid = menu_daemon_launch_app(menu_daemon_get_taskmgr_pkgname(), kb);
	if (s_info.taskmgr_pid < 0) {
		_SECURE_E("Failed to reset %s", APP_TRAY_PKG_NAME);
	} else if (s_info.taskmgr_pid > 0) {
		if(starter_dbus_set_oomadj(s_info.taskmgr_pid, OOM_ADJ_VALUE_DEFAULT) < 0){
			_E("failed to send oom dbus signal");
		}
	}

	bundle_free(kb);
}



static bool _exist_package(char *pkgid)
{
	int ret = 0;
	pkgmgrinfo_appinfo_h handle = NULL;

	ret = pkgmgrinfo_appinfo_get_appinfo(pkgid, &handle);
	if (PMINFO_R_OK != ret || NULL == handle) {
		_SECURE_D("%s doesn't exist in this binary", pkgid);
		return false;
	}

	pkgmgrinfo_appinfo_destroy_appinfo(handle);

	return true;
}



static void _check_home_screen_package(void)
{
	char *pkgname = NULL;
	pkgname = menu_daemon_get_selected_pkgname();
	if (pkgname) {
		_SECURE_D("Selected Home-screen : %s", pkgname);
		bool is_existed = _exist_package(pkgname);
		free(pkgname);
		if (true == is_existed) {
			return;
		}
	}

#if 0 /* Homescreen pkg is changed */
	if (_exist_package(CLUSTER_HOME_PKG_NAME)) {
		if (0 != vconf_set_str(VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME, CLUSTER_HOME_PKG_NAME)) {
			_SECURE_E("Cannot set value(%s) into key(%s)", VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME, CLUSTER_HOME_PKG_NAME);
		} else return;
	}
#else
	if (_exist_package(HOMESCREEN_PKG_NAME)) {
		if (0 != vconf_set_str(VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME, HOMESCREEN_PKG_NAME)) {
			_SECURE_E("Cannot set value(%s) into key(%s)", VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME, HOMESCREEN_PKG_NAME);
		} else return;
	}
#endif

	if (_exist_package(MENU_SCREEN_PKG_NAME)) {
		if (0 != vconf_set_str(VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME, MENU_SCREEN_PKG_NAME)) {
			_SECURE_E("Cannot set value(%s) into key(%s)", VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME, MENU_SCREEN_PKG_NAME);
		} else return;
	}
	_E("Invalid Homescreen..!!");
}



#define SERVICE_OPERATION_MAIN_KEY "__APP_SVC_OP_TYPE__"
#define SERVICE_OPERATION_MAIN_VALUE "http://tizen.org/appcontrol/operation/main"
inline int menu_daemon_open_homescreen(const char *pkgname)
{
	char *homescreen = NULL;
	char *tmp = NULL;

//	if (bincfg_is_factory_binary() != 1) {
		if (menu_daemon_is_safe_mode()) {
			homescreen = HOMESCREEN_PKG_NAME;
		} else {
			homescreen = (char *) pkgname;
		}
/*	} else {
		homescreen = (char *) pkgname;
	}
*/
	if (!pkgname) {
		tmp = menu_daemon_get_selected_pkgname();
		retv_if(NULL == tmp, -1);
		homescreen = tmp;
	}

	int ret = -1;

	//Check preloaded Homescreen
	if (!strncmp(homescreen, HOMESCREEN_PKG_NAME, strlen(homescreen))) {
		_SECURE_D("Launch %s", homescreen);
		bundle *b = NULL;
		b = bundle_create();
		if (!b) {
			_E("Failed to create bundle");
			ret = menu_daemon_open_app(homescreen);
			while (0 > ret) {
				_E("Failed to open a default home, %s", homescreen);
				ret = menu_daemon_open_app(homescreen);
			}
		} else {
			bundle_add(b, SERVICE_OPERATION_MAIN_KEY, SERVICE_OPERATION_MAIN_VALUE);
			ret = menu_daemon_launch_app(homescreen, b);
			while (0 > ret) {
				_E("Failed to launch a default home, %s", homescreen);
				ret = menu_daemon_launch_app(homescreen, b);
			}
			bundle_free(b);
		}
	} else {
		ret = menu_daemon_open_app(homescreen);
		while (0 > ret) {
			_E("Failed to open a default home, %s", homescreen);
			ret = menu_daemon_open_app(homescreen);
		}
	}
#if RELAUNCH_TASKMGR
	if (s_info.taskmgr_pid < 0) {
		_hide_launch_task_mgr();
	}
#endif
	s_info.home_pid = ret;
	if (ret > 0) {
		if(starter_dbus_set_oomadj(ret, OOM_ADJ_VALUE_HOMESCREEN) < 0){
			_E("failed to send oom dbus signal");
		}
	}

	if (tmp) free(tmp);
	return ret;
}


#if 0
inline int menu_daemon_get_pm_key_ignore(int ignore_key)
{
	return s_info.pm_key_ignore & ignore_key;
}


inline void menu_daemon_set_pm_key_ignore(int ignore_key, int value)
{
	if (value) {
		s_info.pm_key_ignore |= ignore_key;
	} else {
		s_info.pm_key_ignore &= ~ignore_key;
	}

	if (vconf_set_int(VCONFKEY_PM_KEY_IGNORE, s_info.pm_key_ignore) < 0) {
		_E("Can't set %s", VCONFKEY_PM_KEY_IGNORE);
	}
}
#endif


static void _show_cb(keynode_t* node, void *data)
{
	int seq;

	_D("[MENU_DAEMON] _show_cb is invoked");

	if (node) {
		seq = vconf_keynode_get_int(node);
	} else {
		if (vconf_get_int(VCONFKEY_STARTER_SEQUENCE, &seq) < 0) {
			_E("Failed to get sequence info");
			return;
		}
	}

	switch (seq) {
		case 0:
			if (s_info.home_pid > 0) {
				int pid;
				_D("pid[%d] is terminated.", s_info.home_pid);
				pid = s_info.home_pid;
				s_info.home_pid = -1; /* to freeze the dead_cb */
				if (aul_terminate_pid(pid) != AUL_R_OK)
					_E("Failed to terminate %d", s_info.home_pid);

				_D("pid[%d] is terminated.", s_info.taskmgr_pid);
				pid = s_info.taskmgr_pid;
				s_info.taskmgr_pid = -1; /* to freeze the dead_cb */
				if (pid > 0 && aul_terminate_pid(pid) != AUL_R_OK)
					_E("Failed to terminate %d", s_info.taskmgr_pid);
			}
			break;
		case 1:
			menu_daemon_open_homescreen(NULL);
			break;
		default:
			_E("False sequence [%d]", seq);
			break;
	}

	return;
}



static void _font_cb(keynode_t* node, void *data)
{
	_D("Font is changed");

	if (AUL_R_OK != aul_terminate_pid(s_info.taskmgr_pid))
		_E("Cannot terminate Taskmgr");
}



static void _cradle_status_cb(keynode_t* node, void *data)
{
	if (vconf_get_int(VCONFKEY_SYSMAN_CRADLE_STATUS, &s_info.cradle_status) < 0) {
		_E("Failed to get cradle status");
		s_info.cradle_status = 0;
	}

	_D("Cradle status is changed to [%d]", s_info.cradle_status);
}


#if 0
static void _pm_key_ignore_cb(keynode_t* node, void *data)
{
	if (vconf_get_int(VCONFKEY_PM_KEY_IGNORE, &s_info.pm_key_ignore) < 0) {
		_E("Failed to get pm key ignore");
		s_info.pm_key_ignore= -1;
	}

	_D("pm key ignore is changed to [%d]", s_info.pm_key_ignore);
}
#endif


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

	_check_home_screen_package();

	pkgname = menu_daemon_get_selected_pkgname();
	if (!pkgname)
		return;

	_SECURE_D("pkg_name : %s", pkgname);

	if (s_info.home_pid > 0) {
		char old_pkgname[256];

		if (aul_app_get_pkgname_bypid(s_info.home_pid, old_pkgname, sizeof(old_pkgname)) == AUL_R_OK) {
			if (!strcmp(pkgname, old_pkgname)) {
				_D("Package is changed but same package is selected");
				free(pkgname);
				return;
			}
		}

		if (AUL_R_OK != aul_terminate_pid(s_info.home_pid))
			_D("Failed to terminate pid %d", s_info.home_pid);
	} else {
		/* If there is no running home */
		menu_daemon_open_homescreen(pkgname);
	}

	/* NOTE: Dead callback will catch the termination of a current menuscreen 
	 * menu_daemon_open_homescreen(pkgname);
	 */
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
			if(starter_dbus_set_oomadj(s_info.volume_pid, OOM_ADJ_VALUE_DEFAULT) < 0){
				_E("failed to send oom dbus signal");
			}
			return;
		}
	}
}


static void _menu_daemon_launch_homescreen(const char *pkgname)
{
	char *homescreen = NULL;
	char *tmp = NULL;
	bundle *b;

//	if (bincfg_is_factory_binary() != 1) {
		if (menu_daemon_is_safe_mode()) {
			homescreen = HOMESCREEN_PKG_NAME;
		} else {
			homescreen = (char *) pkgname;
		}
/*	} else {
		homescreen = (char *) pkgname;
	}
*/
	if (!pkgname) {
		tmp = menu_daemon_get_selected_pkgname();
		ret_if(NULL == tmp);
		homescreen = tmp;
	}

	int ret = -1;
	b = bundle_create();
	bundle_add(b, HOME_TERMINATED, ISTRUE);
	ret = aul_launch_app(homescreen, b);
	while (0 > ret) {
		_E("Failed to open a default home, %s", homescreen);
		ret = aul_launch_app(homescreen, b);
	}

	if(b) {
		bundle_free(b);
	}
#if RELAUNCH_TASKMGR
	if (s_info.taskmgr_pid < 0) {
		_hide_launch_task_mgr();
	}
#endif
	s_info.home_pid = ret;
	if (ret > 0) {
		if(starter_dbus_set_oomadj(ret, OOM_ADJ_VALUE_HOMESCREEN) < 0){
			_E("failed to send oom dbus signal");
		}
	}

	if (tmp) free(tmp);
}


int menu_daemon_check_dead_signal(int pid)
{
	char *pkgname;

	_D("Process %d is termianted", pid);

	if (pid < 0) return 0;
	if (s_info.power_off) return 0;

	pkgname = menu_daemon_get_selected_pkgname();
	if (!pkgname)
		return 0;

	if (pid == s_info.home_pid) {
		/* Relaunch */
		_SECURE_D("pkg_name : %s", pkgname);
		//menu_daemon_open_homescreen(pkgname);
		_menu_daemon_launch_homescreen(pkgname);
	}
#if RELAUNCH_TASKMGR
	else if (pid == s_info.taskmgr_pid) {
		_hide_launch_task_mgr();
	}
#endif
	else if (pid == s_info.volume_pid) {
		/* Relaunch */
		_launch_volume();
	} else {
		_D("Unknown process, ignore it (dead pid %d, home pid %d, taskmgr pid %d)",
				pid, s_info.home_pid, s_info.taskmgr_pid);
	}

	free(pkgname);

	return 0;
}


int menu_daemon_get_volume_pid(void)
{
	return s_info.volume_pid;
}


static void _svoice_pkg_cb(keynode_t* node, void *data)
{
	if (s_info.svoice_pkg_name) free(s_info.svoice_pkg_name);

	s_info.svoice_pkg_name = vconf_get_str(VCONFKEY_SVOICE_PACKAGE_NAME);
	ret_if(NULL == s_info.svoice_pkg_name);

	_SECURE_D("svoice pkg name is changed to [%s]", s_info.svoice_pkg_name);
}


#define SERVICE_OPERATION_POPUP_SEARCH "http://samsung.com/appcontrol/operation/search"
#define SEARCH_PKG_NAME "com.samsung.sfinder"
int menu_daemon_launch_search(void)
{
	app_control_h app_control;
	int ret = APP_CONTROL_ERROR_NONE;

	app_control_create(&app_control);
	app_control_set_operation(app_control, APP_CONTROL_OPERATION_DEFAULT);
	app_control_set_app_id(app_control, SEARCH_PKG_NAME);

	ret = app_control_send_launch_request(app_control, NULL, NULL);

	if(ret != APP_CONTROL_ERROR_NONE) {
		_E("Cannot launch search!! err[%d]", ret);
	}

	app_control_destroy(app_control);
	return ret;
}


const char *menu_daemon_get_svoice_pkg_name(void)
{
	return s_info.svoice_pkg_name;
}



static void _power_off_cb(keynode_t* node, void *data)
{
	int val = VCONFKEY_SYSMAN_POWER_OFF_NONE;
	ret_if(vconf_get_int(VCONFKEY_SYSMAN_POWER_OFF_STATUS, &val) != 0);

	if (val == VCONFKEY_SYSMAN_POWER_OFF_DIRECT || val == VCONFKEY_SYSMAN_POWER_OFF_RESTART) {
		s_info.power_off = 1;
	} else s_info.power_off = 0;

	_D("power off status : %d", s_info.power_off);
}

static Eina_Bool _launch_volume_idler_cb(void *data)
{
	_D("%s, %d", __func__, __LINE__);
	_launch_volume();

	return ECORE_CALLBACK_CANCEL;
}

void menu_daemon_init(void *data)
{
	bool is_exist = false;
	_D( "[MENU_DAEMON]menu_daemon_init is invoked");

	aul_launch_init(NULL,NULL);

	_check_home_screen_package();

	create_key_window();
	//if (xmonitor_init() < 0) _E("cannot init xmonitor");

//	_launch_volume();

	is_exist = _exist_package(TASKMGR_PKG_NAME);
	if(is_exist){
		s_info.taskmgr_pkg_name = TASKMGR_PKG_NAME;
	}
	else{
		s_info.taskmgr_pkg_name = DEFAULT_TASKMGR_PKG_NAME; /* rsa task manager */
	}

	if (vconf_notify_key_changed(VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME, _pkg_changed, NULL) < 0)
		_E("Failed to add the callback for package change event");

	if (vconf_notify_key_changed(VCONFKEY_STARTER_SEQUENCE, _show_cb, NULL) < 0)
		_E("Failed to add the callback for show event");

	if (vconf_notify_key_changed("db/setting/accessibility/font_name", _font_cb, NULL) < 0)
		_E("Failed to add the callback for font event");

	if (vconf_notify_key_changed(VCONFKEY_SYSMAN_CRADLE_STATUS, _cradle_status_cb, NULL) < 0)
		_E("Failed to add the callback for cradle status");

	_D("Cradle status : %d", menu_daemon_get_cradle_status());

#if 0
	if (vconf_notify_key_changed(VCONFKEY_PM_KEY_IGNORE, _pm_key_ignore_cb, NULL) < 0)
		_E("Failed to add the callback for pm key ignore");
	_pm_key_ignore_cb(NULL, NULL);
#endif

	_pkg_changed(NULL, NULL);

	ecore_idler_add(_launch_volume_idler_cb, NULL);

	if (vconf_notify_key_changed(VCONFKEY_SVOICE_PACKAGE_NAME, _svoice_pkg_cb, NULL) < 0)
		_E("Failed to add the callback for svoice pkg");

	_svoice_pkg_cb(NULL, NULL);

	if (vconf_notify_key_changed(VCONFKEY_SYSMAN_POWER_OFF_STATUS, _power_off_cb, NULL) < 0)
		_E("Failed to add the callback for power-off");

	// THIS ROUTINE IS FOR SAT.
	vconf_set_int(VCONFKEY_IDLE_SCREEN_LAUNCHED, VCONFKEY_IDLE_SCREEN_LAUNCHED_TRUE);
}



void menu_daemon_fini(void)
{
	if (vconf_ignore_key_changed(VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME, _pkg_changed) < 0)
		_E("Failed to ignore the callback for package change event");

	if (vconf_ignore_key_changed(VCONFKEY_STARTER_SEQUENCE, _show_cb) < 0)
		_E("Failed to ignore the callback for show event");

	if (vconf_ignore_key_changed("db/setting/accessibility/font_name", _font_cb) < 0)
		_E("Failed to ignore the callback for font event");

	if (vconf_ignore_key_changed(VCONFKEY_SYSMAN_CRADLE_STATUS, _cradle_status_cb) < 0)
		_E("Failed to ignore the callback for cradle status");

#if 0
	if (vconf_ignore_key_changed(VCONFKEY_PM_KEY_IGNORE, _pm_key_ignore_cb) < 0)
		_E("Failed to ignore the callback for pm key ignore");
#endif

	if (vconf_ignore_key_changed(VCONFKEY_SVOICE_PACKAGE_NAME, _svoice_pkg_cb) < 0)
		_E("Failed to ignore the callback for svoice pkg");
	if (s_info.svoice_pkg_name) free(s_info.svoice_pkg_name);

	if (vconf_ignore_key_changed(VCONFKEY_SYSMAN_POWER_OFF_STATUS, _power_off_cb) < 0)
		_E("Failed to ignore the callback for power-off");

	//xmonitor_fini();
	destroy_key_window();
}



// End of a file
