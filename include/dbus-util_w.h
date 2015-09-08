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

#ifndef __DBUS_H__
#define __DBUS_H__

#include <E_DBus.h>

int request_Poweroff(void);
int request_dbus_cpu_booster(void);
int init_dbus_ALPM_signal(void *data);
int init_dbus_COOL_DOWN_MODE_signal(void *data);
int get_dbus_cool_down_mode(void *data);
int init_dbus_NIKE_RUNNING_STATUS_signal(void *data);
int init_dbus_ALPM_clock_state_signal(void *data);
void starter_dbus_alpm_clock_signal_send(void *data);
DBusConnection *starter_dbus_connection_get(void);


#endif //__DBUS_H__

