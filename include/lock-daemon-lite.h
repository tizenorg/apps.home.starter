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

#ifndef __LOCK_DAEMON_LITE_H__
#define __LOCK_DAEMON_LITE_H__

int start_lock_daemon_lite(int launch_lock, int is_first_boot);
int lockd_get_lock_type(void);
int lockd_get_hall_status(void);
int lockd_get_lock_state(void);

#endif				/* __LOCK_DAEMON_LITE_H__ */
