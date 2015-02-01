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

#ifndef __DBUS_UTIL_H__
#define __DBUS_UTIL_H__

#include <E_DBus.h>

#define OOM_ADJ_VALUE_HOMESCREEN 0

#ifdef FEATURE_LITE
#define OOM_ADJ_VALUE_DEFAULT   200
#else
#define OOM_ADJ_VALUE_DEFAULT   0
#endif

void starter_dbus_home_raise_signal_send(void);
int starter_dbus_set_oomadj(int pid, int oom_adj_value);

#endif //__DBUS_UTIL_H__
