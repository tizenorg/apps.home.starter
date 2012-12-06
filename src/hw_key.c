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



#include <ail.h>
#include <bundle.h>
#include <Elementary.h>
#include <Ecore_X.h>
#include <Ecore_Input.h>
#include <sysman.h>
#include <syspopup_caller.h>
#include <utilX.h>
#include <vconf.h>
#include <system/media_key.h>

#include "hw_key.h"
#include "util.h"

#define TASKMGR_PKG_NAME "org.tizen.taskmgr"
#define CAMERA_PKG_NAME "org.tizen.camera-app"
#define CALLLOG_PKG_NAME "org.tizen.calllog"
#define SEARCH_PKG_NAME "org.tizen.smartsearch"
#define MUSIC_PLAYER_PKG_NAME "org.tizen.music-player"



static struct {
	Ecore_X_Window win;
	Ecore_Event_Handler *key_up;
	Ecore_Event_Handler *key_down;
	Ecore_Timer *long_press;
	Ecore_Timer *single_timer;
	Ecore_Timer *volume_up_long_press;
	Ecore_Timer *volume_down_long_press;
	Eina_Bool cancel;
} key_info = {
	.win = 0x0,
	.key_up = NULL,
	.key_down = NULL,
	.long_press = NULL,
	.single_timer = NULL,
	.volume_up_long_press = NULL,
	.volume_down_long_press = NULL,
	.cancel = EINA_FALSE,
};



static Eina_Bool _launch_taskmgr_cb(void* data)
{
	_D("Launch TASKMGR");

	key_info.long_press = NULL;

	if (aul_open_app(TASKMGR_PKG_NAME) < 0)
		_E("Failed to launch the taskmgr");

	return ECORE_CALLBACK_CANCEL;
}



static Eina_Bool _launch_home_screen(void *data)
{
	char *package;
	int ret;

	syspopup_destroy_all();
	key_info.single_timer = NULL;

	package = vconf_get_str(VCONFKEY_SETAPPL_SELECTED_PACKAGE_NAME);
	if (package) {
		ret = aul_open_app(package);
		if (ret < 0) {
			_E("cannot launch package %s(err:%d)", package, ret);

			if (-1 == ret) { // -1 : AUL returns '-1' when there is no package name in DB.
				ret = aul_open_app(HOME_SCREEN_PKG_NAME);
				if (ret < 0) {
					_E("Failed to open a default home, %s(err:%d)", HOME_SCREEN_PKG_NAME, ret);
				}
			}
		}

		free(package);
	} else {
		ret = aul_open_app(HOME_SCREEN_PKG_NAME);
		if (ret < 0) _E("Cannot open default home");
	}

	if (ret > 0) {
		if (-1 == sysconf_set_mempolicy_bypid(ret, OOM_IGNORE)) {
			_E("Cannot set the memory policy for Home-screen(%d)", ret);
		} else {
			_E("Set the memory policy for Home-screen(%d)", ret);
		}
	}

	return ECORE_CALLBACK_CANCEL;
}



inline static void _launch_svoice(void)
{
	_D("Launch smartsearch");
	if (aul_open_app(SEARCH_PKG_NAME) < 0)
		_E("Cannot open Smart-search");

	return;
}



inline static int _release_home_key(void)
{
	retv_if(NULL == key_info.long_press, EXIT_SUCCESS);
	ecore_timer_del(key_info.long_press);
	key_info.long_press = NULL;

	if (NULL == key_info.single_timer) {
		key_info.single_timer = ecore_timer_add(0.3, _launch_home_screen, NULL);
		return EXIT_SUCCESS;
	}
	ecore_timer_del(key_info.single_timer);
	key_info.single_timer = NULL;

	syspopup_destroy_all();
	_launch_svoice();

	return EXIT_SUCCESS;
}



inline static void _release_multimedia_key(const char *value)
{
	bundle *b;
	int ret;

	_D("Multimedia key is released with %s", value);
	ret_if(NULL == value);

	b = bundle_create();
	if (!b) {
		_E("Cannot create bundle");
		return;
	}

	bundle_add(b, "multimedia_key", value);
	ret = aul_launch_app(MUSIC_PLAYER_PKG_NAME, b);
	bundle_free(b);

	if (ret < 0)
		_E("Failed to launch the running apps, ret : %d", ret);
}



static Eina_Bool _key_release_cb(void *data, int type, void *event)
{
	Evas_Event_Key_Up *ev = event;
	int val = -1;

	_D("Released");

	if (!ev) {
		_D("Invalid event object");
		return ECORE_CALLBACK_RENEW;
	}

	if (!strcmp(ev->keyname, KEY_END)) {
	} else if (!strcmp(ev->keyname, KEY_CONFIG)) {
	} else if (!strcmp(ev->keyname, KEY_SEND)) {
	} else if (!strcmp(ev->keyname, KEY_HOME)) {

		if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) {
			_D("Cannot get VCONFKEY_IDLE_LOCK_STATE");
		}
		if (val == VCONFKEY_IDLE_LOCK) {
			_D("lock state, ignore home key..!!");
			return ECORE_CALLBACK_RENEW;
		}

		if (EINA_TRUE == key_info.cancel) {
			_D("Cancel key is activated");
			if (key_info.long_press) {
				ecore_timer_del(key_info.long_press);
				key_info.long_press = NULL;
			}

			if (key_info.single_timer) {
				ecore_timer_del(key_info.single_timer);
				key_info.single_timer = NULL;
			}

			return ECORE_CALLBACK_RENEW;
		}

		_release_home_key();
	} else if (!strcmp(ev->keyname, KEY_PAUSE)) {
	} else if (!strcmp(ev->keyname, KEY_CANCEL)) {
		_D("CANCEL Key is released");
		key_info.cancel = EINA_FALSE;
	} else if (!strcmp(ev->keyname, KEY_MEDIA)) {
		_release_multimedia_key("KEY_PLAYCD");
	}

	return ECORE_CALLBACK_RENEW;
}



static Eina_Bool _key_press_cb(void *data, int type, void *event)
{
	Evas_Event_Key_Down *ev = event;
	int val = -1;

	_D("Pressed");

	if (!ev) {
		_D("Invalid event object");
		return ECORE_CALLBACK_RENEW;
	}

	if (!strcmp(ev->keyname, KEY_SEND)) {
		_D("Launch calllog");
		if (aul_open_app(CALLLOG_PKG_NAME) < 0)
			_E("Failed to launch %s", CALLLOG_PKG_NAME);
	} else if(!strcmp(ev->keyname, KEY_CONFIG)) {
		_D("Launch camera");
		if (aul_open_app(CAMERA_PKG_NAME) < 0)
			_E("Failed to launch %s", CAMERA_PKG_NAME);
	} else if (!strcmp(ev->keyname, KEY_HOME)) {
		if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) {
			_D("Cannot get VCONFKEY_IDLE_LOCK_STATE");
		}
		if (val == VCONFKEY_IDLE_LOCK) {
			_D("lock state, ignore home key..!!");
			return ECORE_CALLBACK_RENEW;
		}
		if (key_info.long_press) {
			ecore_timer_del(key_info.long_press);
			key_info.long_press = NULL;
		}

		key_info.long_press = ecore_timer_add(0.5, _launch_taskmgr_cb, NULL);
		if (!key_info.long_press)
			_E("Failed to add timer for long press detection");
	} else if (!strcmp(ev->keyname, KEY_CANCEL)) {
		_D("Cancel button is pressed");
		key_info.cancel = EINA_TRUE;
	} else if (!strcmp(ev->keyname, KEY_MEDIA)) {
		_D("Media key is pressed");
	}

	return ECORE_CALLBACK_RENEW;
}



void _media_key_event_cb(media_key_e key, media_key_event_e status, void *user_data)
{
	_D("MEDIA KEY EVENT");
	if (MEDIA_KEY_STATUS_PRESSED == status) return;

	if (MEDIA_KEY_PAUSE == key) {
		_release_multimedia_key("KEY_PAUSECD");
	} else if (MEDIA_KEY_PLAY == key) {
		_release_multimedia_key("KEY_PLAYCD");
	}
}



void create_key_window(void)
{
	key_info.win = ecore_x_window_input_new(0, 0, 0, 1, 1);
	if (!key_info.win) {
		_D("Failed to create hidden window");
		return;
	}
	ecore_x_event_mask_unset(key_info.win, ECORE_X_EVENT_MASK_NONE);
	ecore_x_icccm_title_set(key_info.win, "menudaemon,key,receiver");
	ecore_x_netwm_name_set(key_info.win, "menudaemon,key,receiver");
	ecore_x_netwm_pid_set(key_info.win, getpid());

	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_HOME, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_VOLUMEDOWN, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_VOLUMEUP, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_CONFIG, SHARED_GRAB);
	utilx_grab_key(ecore_x_display_get(), key_info.win, KEY_MEDIA, SHARED_GRAB);

	key_info.key_up = ecore_event_handler_add(ECORE_EVENT_KEY_UP, _key_release_cb, NULL);
	if (!key_info.key_up)
		_D("Failed to register a key up event handler");

	key_info.key_down = ecore_event_handler_add(ECORE_EVENT_KEY_DOWN, _key_press_cb, NULL);
	if (!key_info.key_down)
		_D("Failed to register a key down event handler");

	media_key_reserve(_media_key_event_cb, NULL);
}



void destroy_key_window(void)
{
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_HOME);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_VOLUMEDOWN);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_VOLUMEUP);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_CONFIG);
	utilx_ungrab_key(ecore_x_display_get(), key_info.win, KEY_MEDIA);

	if (key_info.key_up) {
		ecore_event_handler_del(key_info.key_up);
		key_info.key_up = NULL;
	}

	if (key_info.key_down) {
		ecore_event_handler_del(key_info.key_down);
		key_info.key_down = NULL;
	}

	ecore_x_window_delete_request_send(key_info.win);
	key_info.win = 0x0;

	media_key_release();
}



// End of a file
