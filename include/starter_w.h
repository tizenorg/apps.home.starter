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

#ifndef __STARTER_W_H__
#define __STARTER_W_H__

#include <sys/time.h>
#include <aul.h>
#include <alarm.h>

struct appdata {
	struct timeval tv_start;	/* start time */
	int launcher_pid;
	alarm_id_t alarm_id;	/* -1 : None, others : set alarm */
	int bt_connected;
	char *home_pkgname;
	int first_boot;
	int cool_down_mode;
	int wake_up_setting;
	int pid_ALPM_clock;
	int ambient_mode;
	int retry_cnt;
	int nike_running_status;
	int ALPM_clock_state;
	int reserved_apps_local_port_id;
	Eina_List *reserved_apps_list;
	int lcd_status;
	char *reserved_popup_app_id;
};

typedef enum {
	STARTER_RESERVED_APPS_SHEALTH = 0,
	STARTER_RESERVED_APPS_NIKE = 1,
	STARTER_RESERVED_APPS_HERE = 2,
	STARTER_RESERVED_APPS_MAX = 3,
} starter_reservd_apps_type;

#define W_HOME_PKGNAME "org.tizen.w-home"
#define W_LAUNCHER_PKGNAME "com.samsung.w-launcher-app"


int w_open_app(char *pkgname);
int w_launch_app(char *pkgname, bundle *b);

#endif				/* __STARTER_H__ */
