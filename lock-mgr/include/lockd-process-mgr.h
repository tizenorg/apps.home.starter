/*
 *  starter
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Seungtaek Chung <seungtaek.chung@samsung.com>, Mi-Ju Lee <miju52.lee@samsung.com>, Xi Zhichan <zhichan.xi@samsung.com>
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
 *
 */

#ifndef __LOCKD_PROCESS_MGR_H__
#define __LOCKD_PROCESS_MGR_H__

int lockd_process_mgr_start_lock(void *data, int (*dead_cb) (int, void *),
				 int phone_lock_state);

void lockd_process_mgr_restart_lock(int phone_lock_state);

int lockd_process_mgr_start_phone_lock(void);

void lockd_process_mgr_terminate_phone_lock(int phone_lock_pid);

int lockd_process_mgr_check_lock(int pid);

#endif				/* __LOCKD_PROCESS_MGR_H__ */
