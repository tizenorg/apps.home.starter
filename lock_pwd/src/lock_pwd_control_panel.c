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

#include <app_control.h>
#include <utilX.h>
#include <ui-gadget.h>
#include <call-manager.h>

#include "lock_mgr.h"
#include "util.h"
#include "lock_pwd_util.h"
#include "lock_pwd_control_panel.h"

static struct _s_lock_pwd_control_panel {
	Evas_Object *control_panel_layout;
	Evas_Object *btn_cancel;
	Evas_Object *btn_return_to_call;
	Evas_Object *btn_plmn;
	cm_client_h cm_handle;
} s_lock_pwd_control_panel = {
	.control_panel_layout = NULL,
	.btn_cancel = NULL,
	.btn_return_to_call = NULL,
	.btn_plmn = NULL,
	.cm_handle = NULL,
};




void lock_pwd_control_panel_cancel_btn_enable_set(Eina_Bool enable)
{
	ret_if(!s_lock_pwd_control_panel.control_panel_layout);

	if (enable) {
		elm_object_signal_emit(s_lock_pwd_control_panel.control_panel_layout, "btn,cancel,enable", "prog");
	} else {
		elm_object_signal_emit(s_lock_pwd_control_panel.control_panel_layout, "btn,cancel,disable", "prog");
	}
}



static void _btn_cancel_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	_D("Cancel button is clicked");

	lock_mgr_sound_play(LOCK_SOUND_TAP);

	if (!lock_mgr_lockscreen_launch()) {
		_E("Failed to launch lockscreen");
	}

	lock_pwd_util_view_init();
}



static void _btn_return_to_call_clicked_cb(void *data, Evas_Object *obj, const char *emission, const char *source)
{
	_D("'Return to call' button is clicked");

	ret_if(!s_lock_pwd_control_panel.cm_handle);

	cm_activate_ui(s_lock_pwd_control_panel.cm_handle);
}



static Evas_Object *_create_btn_return_to_call(Evas_Object *parent)
{
	Evas_Object *layout = NULL;

	retv_if(!parent, NULL);

	layout = elm_layout_add(parent);
	retv_if(!layout, NULL);

	if (!elm_layout_file_set(layout, LOCK_PWD_EDJE_FILE, "btn-return-to-call")) {
		_E("Failed to set edje file : %s", LOCK_PWD_EDJE_FILE);
		goto error;
	}

	evas_object_size_hint_weight_set(layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

	elm_object_signal_callback_add(layout, "btn,return_to_call,clicked", "btn.return_to_call", _btn_return_to_call_clicked_cb, NULL);

	evas_object_show(layout);

	return layout;

error:
	if (layout) {
		evas_object_del(layout);
	}

	return NULL;
}



static void _destroy_btn_return_to_call(void)
{
	if (s_lock_pwd_control_panel.btn_return_to_call) {
		elm_object_signal_callback_del(s_lock_pwd_control_panel.btn_return_to_call, "btn,return_to_call,clicked", "btn.return_to_call", _btn_return_to_call_clicked_cb);
		evas_object_del(s_lock_pwd_control_panel.btn_return_to_call);
		s_lock_pwd_control_panel.btn_return_to_call = NULL;
	}
}



#if 0
static Evas_Object *_create_btn_plmn(Evas_Object *parent)
{
	Evas_Object *plmn = NULL;

	return plmn;
}
#endif


static void _destroy_btn_plmn(void)
{
	if (s_lock_pwd_control_panel.btn_plmn) {
		evas_object_del(s_lock_pwd_control_panel.btn_plmn);
		s_lock_pwd_control_panel.btn_plmn = NULL;
	}
}



static void _call_status_changed_cb(cm_call_status_e call_status, const char *call_num, void *user_data)
{
	Evas_Object *control_panel_layout = NULL;
	Evas_Object *btn_return_to_call = NULL;

	control_panel_layout = user_data;
	ret_if(!control_panel_layout);

	btn_return_to_call = elm_object_part_content_get(control_panel_layout, "sw.btn.return_to_call");

	switch (call_status) {
	case CM_CALL_STATUS_IDLE:
		if (btn_return_to_call) {
			_D("remove 'Return to call' button");
			_destroy_btn_return_to_call();
		}
		break;
	case CM_CALL_STATUS_RINGING:
	case CM_CALL_STATUS_OFFHOOK:
		if (!btn_return_to_call) {
			_D("create 'Return to call' button");
			btn_return_to_call = _create_btn_return_to_call(control_panel_layout);
			if (btn_return_to_call) {
				elm_object_part_content_set(control_panel_layout, "sw.btn.return_to_call", btn_return_to_call);
				s_lock_pwd_control_panel.btn_return_to_call = btn_return_to_call;
			} else {
				_E("Failed to add a button for Return to call");
			}
		}
		break;
	default:
		_E("call status error : %d", call_status);
		break;
	}
}



Evas_Object *lock_pwd_control_panel_create(Evas_Object *parent)
{
	Evas_Object *control_panel_layout = NULL;
	int ret = 0;

	/* Initialize callmgr-client */
	cm_init(&s_lock_pwd_control_panel.cm_handle);

	retv_if(!parent, NULL);

	control_panel_layout = elm_layout_add(parent);
	retv_if(!control_panel_layout, NULL);

	if (!elm_layout_file_set(control_panel_layout, LOCK_PWD_EDJE_FILE, "lock-control-panel")) {
		_E("Failed to set edje file : %s", LOCK_PWD_EDJE_FILE);
		goto ERROR;
	}
	s_lock_pwd_control_panel.control_panel_layout = control_panel_layout;

	evas_object_size_hint_weight_set(control_panel_layout, EVAS_HINT_EXPAND, EVAS_HINT_EXPAND);
	evas_object_size_hint_align_set(control_panel_layout, EVAS_HINT_FILL, EVAS_HINT_FILL);

#if 0
	/* plmn button */
	Evas_Object *btn_plmn = NULL;
	btn_plmn = _create_btn_plmn(control_panel_layout);
	if (btn_plmn) {
		elm_object_part_content_set(control_panel_layout, "sw.btn.plmn", btn_plmn);
		s_lock_pwd_control_panel.btn_plmn = btn_plmn;
	} else {
		_E("Failed to add a button for PLMN");
	}
#endif

	/* cancel button */
	elm_object_part_text_set(control_panel_layout, "txt.cancel", _("IDS_COM_BUTTON_CANCEL"));
	elm_object_signal_callback_add(control_panel_layout, "btn,cancel,clicked", "btn.cancel", _btn_cancel_clicked_cb, NULL);

	evas_object_show(control_panel_layout);

	ret = cm_set_call_status_cb(s_lock_pwd_control_panel.cm_handle, _call_status_changed_cb, control_panel_layout);
	if (ret != CM_ERROR_NONE) {
		_E("Failed to register call status callback");
	}

	return control_panel_layout;

ERROR:
	_E("Failed to create password control panel");

	if (control_panel_layout) {
		evas_object_del(control_panel_layout);
		s_lock_pwd_control_panel.control_panel_layout = NULL;
	}

	return NULL;
}

void lock_pwd_control_panel_del(void)
{
	_destroy_btn_return_to_call();
	_destroy_btn_plmn();

	if (s_lock_pwd_control_panel.btn_cancel) {
		elm_object_signal_callback_del(s_lock_pwd_control_panel.control_panel_layout, "btn,cancel,clicked", "btn.cancel", _btn_cancel_clicked_cb);
		evas_object_del(s_lock_pwd_control_panel.btn_cancel);
		s_lock_pwd_control_panel.btn_cancel= NULL;
	}

	if (s_lock_pwd_control_panel.control_panel_layout) {
		evas_object_del(s_lock_pwd_control_panel.control_panel_layout);
		s_lock_pwd_control_panel.control_panel_layout = NULL;
	}

	/* Deinitialize callmgr-client */
	cm_unset_call_status_cb(s_lock_pwd_control_panel.cm_handle);
	cm_deinit(s_lock_pwd_control_panel.cm_handle);
	s_lock_pwd_control_panel.cm_handle = NULL;
}
