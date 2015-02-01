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

#ifndef __DIRECT_ACCESS_H__
#define __DIRECT_ACCESS_H__

#include <E_DBus.h>

DBusMessage *invoke_dbus_method_sync(const char *dest, const char *path,
		const char *interface, const char *method,
		const char *sig, char *param[]);

DBusMessage *invoke_dbus_method(const char *dest, const char *path,
		const char *interface, const char *method,
		const char *sig, char *param[]);

int launch_direct_access(int access_val);

#endif //__DIRECT_ACCESS_H__

