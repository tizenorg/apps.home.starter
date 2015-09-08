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

#include <bundle.h>
#include <sys/types.h>
#include <stdbool.h>

extern void menu_daemon_init(void *data);
extern void menu_daemon_fini(void);

extern int menu_daemon_open_app(const char *pkgname);
extern int menu_daemon_launch_app(const char *pkgname, bundle *b);
extern void menu_daemon_launch_app_tray(void);

extern bool menu_daemon_is_homescreen(pid_t pid);
extern int menu_daemon_is_safe_mode(void);
extern int menu_daemon_get_cradle_status(void);
extern const char *menu_daemon_get_svoice_pkg_name(void);

extern int menu_daemon_check_dead_signal(int pid);

extern char *menu_daemon_get_selected_pkgname(void);
extern int menu_daemon_open_homescreen(const char *pkgname);

#if 0
extern int menu_daemon_get_pm_key_ignore(int ignore_key);
extern void menu_daemon_set_pm_key_ignore(int ignore_key, int value);
#endif

extern int menu_daemon_get_volume_pid(void);

extern int menu_daemon_launch_search(void);


// End of a file
