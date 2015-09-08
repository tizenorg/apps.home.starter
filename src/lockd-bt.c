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

#include <vconf.h>
#include <vconf-keys.h>

#include <bluetooth.h>

#include "lockd-debug.h"
#include "lockd-bt.h"


#define OUT_OF_RANGE_THRESHOLD		-70
#define IN_RANGE_THRESHOLD			-60

/*For test*/
//#define OUT_OF_RANGE_THRESHOLD		-45
//#define IN_RANGE_THRESHOLD			-40



#define VCONFKEY_BT_OUT 	"file/private/lockscreen/bt_out"
#define VCONFKEY_BT_IN		"file/private/lockscreen/bt_in"

#define OUT_OF_RANGE_ALERT			2
#define IN_RANGE_ALERT				1

static int g_is_security = 1;

static int out_of_range_threshold = -70;
static int in_range_threshold = -60;



#if 0
static void _lockd_bt_enabled_cb(const char *address,
		bt_device_connection_link_type_e link_type,
		int rssi_enabled, void *user_data)
{
	LOCKD_WARN("RSSI Enabled: %s %d %d", address, link_type, rssi_enabled);
}


static void _lockd_bt_rssi_alert_cb(char *bt_address,
		bt_device_connection_link_type_e link_type,
		int rssi_alert_type, int rssi_alert_dbm, void *user_data)
{
	int is_security = TRUE;
	LOCKD_WARN("RSSI Alert: [Address:%s LinkType:%d][RSSI Alert Type:%d dBm:%d]",
			bt_address, link_type, rssi_alert_type, rssi_alert_dbm);

#if 0
	if (rssi_alert_dbm < OUT_OF_RANGE_THRESHOLD) {
		g_is_security = TRUE;
		vconf_set_int(VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, TRUE);
	}

	if (rssi_alert_dbm > IN_RANGE_THRESHOLD) {
		g_is_security = FALSE;
		vconf_set_int(VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, FALSE);
	}
#else
	if (rssi_alert_type == OUT_OF_RANGE_ALERT) {
		g_is_security = TRUE;
		vconf_set_int(VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, g_is_security);
	}

	if (rssi_alert_type == IN_RANGE_ALERT) {
		g_is_security = FALSE;
		vconf_set_int(VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, g_is_security);
	}
#endif
}


static void _lockd_notify_security_auto_lock_cb(keynode_t * node, void *data)
{
	int is_security = TRUE;

	vconf_get_int(VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, &is_security);
	LOCKD_WARN("security auto lock is changed to [%d]", is_security);
	if (is_security != g_is_security) {
		LOCKD_ERR("Who changes auto lock security..!!, change value from [%d] to [%d]", is_security, g_is_security);
		vconf_set_int(VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, g_is_security);
	}
}


int lockd_start_bt_monitor(void)
{
	int r = 0;
	char *device_name = NULL;

	bt_device_connection_link_type_e link_type = BT_DEVICE_CONNECTION_LINK_BREDR;

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	r = bt_initialize();
	if (r < 0) {
		LOCKD_ERR("bt_initialize is failed, r=%d", r);
		return -1;
	}

	device_name = vconf_get_str(VCONFKEY_SETAPPL_AUTO_LOCK_DEVICE_NAME_STR);
	if (!device_name) {
		LOCKD_ERR("cant't get BT device name");
		return -1;
	}

	vconf_get_int(VCONFKEY_BT_OUT, &out_of_range_threshold);
	vconf_get_int(VCONFKEY_BT_IN, &in_range_threshold);

	if ((out_of_range_threshold > 0) || (in_range_threshold > 0) || (out_of_range_threshold > in_range_threshold)) {
		out_of_range_threshold = OUT_OF_RANGE_THRESHOLD;
		in_range_threshold = IN_RANGE_THRESHOLD;
	}


	LOCKD_WARN("OUT Range[%d] IN Range [%d] BT device address : [%s]", out_of_range_threshold, in_range_threshold, device_name);
	r = bt_device_enable_rssi_monitor(device_name, link_type,
		out_of_range_threshold, in_range_threshold,
		_lockd_bt_enabled_cb, NULL,
		_lockd_bt_rssi_alert_cb, NULL);
	if (r < 0) {
		LOCKD_ERR("bt_device_enable_rssi_monitor is failed, r=%d", r);
		return -1;
	}

	/* BT will be near when auto lock is set */
	g_is_security = FALSE;
	vconf_set_int(VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, FALSE);
	if (vconf_notify_key_changed
	    (VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, _lockd_notify_security_auto_lock_cb, NULL) != 0) {
		LOCKD_ERR("Fail vconf_notify_key_changed : VCONFKEY");
	}

	return 0;
}


void lockd_stop_bt_monitor(void)
{
	int r = 0;
	char *device_name = NULL;
	bt_device_connection_link_type_e link_type = BT_DEVICE_CONNECTION_LINK_BREDR;

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	device_name = vconf_get_str(VCONFKEY_SETAPPL_AUTO_LOCK_DEVICE_NAME_STR);
	if (!device_name) {
		LOCKD_ERR("cant't get BT device name");
	}
	r = bt_device_disable_rssi_monitor(device_name, link_type,
		_lockd_bt_enabled_cb, NULL);
	if (r < 0) {
		LOCKD_ERR("bt_device_disable_rssi_monitor is failed, r=%d", r);
	}

	/* BT will be far when auto lock is unset  */
	if (vconf_ignore_key_changed
	    (VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, _lockd_notify_security_auto_lock_cb) != 0) {
		LOCKD_ERR("Fail to unregister");
	}
	g_is_security = TRUE;

	r = bt_deinitialize();
	if (r < 0) {
		LOCKD_ERR("bt_deinitialize is failed, r=%d", r);
	}
}



static void _lockd_bt_rssi_stregth_cb(char *bt_address,
			bt_device_connection_link_type_e link_type,
			int rssi_dbm, void *user_data)
{
	LOCKD_WARN("RSSI Strength: [Address %s][Link Type %d][RSSI dBm %d], current auto lock pw security is [%d]",
		bt_address, link_type, rssi_dbm, g_is_security);

	if ((g_is_security) && (rssi_dbm > in_range_threshold)) {
		g_is_security = FALSE;
		vconf_set_int(VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, g_is_security);
	} 
}

void lockd_change_security_auto_lock(int is_connected)
{
	LOCKD_WARN("BT connect is changed to [%d], security status is [%d]", is_connected, g_is_security);

	if ((!is_connected) && (!g_is_security)) {
		g_is_security = TRUE;
		vconf_set_int(VCONFKEY_LOCKSCREEN_SECURITY_AUTO_LOCK, g_is_security);
	} else if (is_connected){
		int r = 0;
		char *device_name = NULL;
		bt_device_connection_link_type_e link_type = BT_DEVICE_CONNECTION_LINK_BREDR;

		device_name = vconf_get_str(VCONFKEY_SETAPPL_AUTO_LOCK_DEVICE_NAME_STR);
		if (!device_name) {
			LOCKD_ERR("cant't get BT device name");
		}

		r = bt_device_get_rssi_strength(device_name, link_type,
			_lockd_bt_rssi_stregth_cb, NULL);
	}
}

int lockd_get_auto_lock_security(void)
{
	LOCKD_DBG("BT auto lock has security : %d", g_is_security);
	return g_is_security;
}
#endif
