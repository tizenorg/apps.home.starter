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

#ifndef __LOCKD_WINDOW_MGR_H__
#define __LOCKD_WINDOW_MGR_H__

typedef struct _lockw_data lockw_data;

int
lockd_window_mgr_get_focus_win_pid(void);

Eina_Bool
lockd_window_set_window_property(lockw_data * data, int lock_app_pid,
				 void *event);

Eina_Bool
lockd_window_set_window_effect(lockw_data * data, int lock_app_pid,
			       void *event);

void
lockd_window_set_scroll_property(lockw_data * data, int lock_type);

void lockd_window_set_phonelock_pid(lockw_data * data, int phone_lock_pid);

void
lockd_window_mgr_ready_lock(void *data, lockw_data * lockw,
			    Eina_Bool(*create_cb) (void *, int, void *),
			    Eina_Bool(*show_cb) (void *, int, void *));

void lockd_window_mgr_finish_lock(lockw_data * lockw);

lockw_data *lockd_window_init(void);

void lockd_create_ug_window(void *data);

void lockd_destroy_ug_window(void *data);

void lockd_show_phonelock_alpha_ug(void *data);
#endif				/* __LOCKD_WINDOW_MGR_H__ */
