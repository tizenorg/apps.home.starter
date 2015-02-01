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

#ifndef __LOCKD_PROCESS_MGR_H__
#define __LOCKD_PROCESS_MGR_H__

void lockd_process_mgr_init(void);

int lockd_process_mgr_start_lock(void *data, int (*dead_cb) (int, void *),
				 int lock_type);

int lockd_process_mgr_restart_lock(int lock_type);

int lockd_process_mgr_start_recovery_lock(void);

int lockd_process_mgr_start_back_to_app_lock(void);

int lockd_process_mgr_start_ready_lock(void);

int lockd_process_mgr_start_phone_lock(void);

int lockd_process_mgr_start_normal_lock(void *data, int (*dead_cb) (int, void *));

void lockd_process_mgr_terminate_lock_app(int lock_app_pid,
					  int state);

void lockd_process_mgr_terminate_phone_lock(int phone_lock_pid);

void lockd_process_mgr_kill_lock_app(int lock_app_pid);

int lockd_process_mgr_check_lock(int pid);

int lockd_process_mgr_check_call(int pid);

int lockd_process_mgr_check_home(int pid);

int lockd_process_mgr_set_lockscreen_priority(int pid);
int lockd_process_mgr_set_pwlock_priority(int pid);
#endif				/* __LOCKD_PROCESS_MGR_H__ */
