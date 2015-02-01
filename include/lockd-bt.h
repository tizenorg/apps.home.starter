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

#ifndef __LOCKD_BT_H__
#define __LOCKD_BT_H__

int lockd_start_bt_monitor(void);

void lockd_stop_bt_monitor(void);

void lockd_change_security_auto_lock(int is_connected);

int lockd_get_auto_lock_security(void);

#endif				/* __LOCKD_BT_H__ */
