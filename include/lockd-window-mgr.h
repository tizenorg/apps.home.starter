 /*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://www.tizenopensource.org/license
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

void
lockd_window_set_window_property(lockw_data * data, int lock_app_pid,
				 void *event);

void
lockd_window_set_window_effect(lockw_data * data, int lock_app_pid,
			       void *event);

void lockd_window_set_phonelock_pid(lockw_data * data, int phone_lock_pid);

void
lockd_window_mgr_ready_lock(void *data, lockw_data * lockw,
			    Eina_Bool(*create_cb) (void *, int, void *),
			    Eina_Bool(*show_cb) (void *, int, void *));

void lockd_window_mgr_finish_lock(lockw_data * lockw);

lockw_data *lockd_window_init(void);
#endif
