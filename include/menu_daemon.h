 /*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://floralicense.org/license/
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

extern bool menu_daemon_is_homescreen(pid_t pid);

extern int menu_daemon_check_dead_signal(int pid);

extern char *menu_daemon_get_selected_pkgname(void);
extern void menu_daemon_open_homescreen(const char *pkgname);

// End of a file
