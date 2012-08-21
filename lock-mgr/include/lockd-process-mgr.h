/*
 * Copyright 2012  Samsung Electronics Co., Ltd
 *
 * Licensed under the Flora License, Version 1.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *  http://www.tizenopensource.org/license
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef __LOCKD_PROCESS_MGR_H__
#define __LOCKD_PROCESS_MGR_H__

int lockd_process_mgr_start_lock(void *data, int (*dead_cb) (int, void *));

int lockd_process_mgr_restart_lock(void);

void lockd_process_mgr_terminate_lock_app(int lock_app_pid,
					  int state);

int lockd_process_mgr_check_lock(int pid);

#endif				/* __LOCKD_PROCESS_MGR_H__ */
