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

#include <E_DBus.h>
#include <dbus/dbus-glib.h>

#include "starter.h"
#include "starter-util.h"
#include "util.h"

#define DBUS_HOME_BUS_NAME "org.tizen.coreapps.home"
#define DBUS_HOME_RAISE_PATH "/Org/Tizen/Coreapps/home/raise"
#define DBUS_HOME_RAISE_INTERFACE DBUS_HOME_BUS_NAME".raise"
#define DBUS_HOME_RAISE_MEMBER "homeraise"

#define DEVICED_BUS_NAME       "org.tizen.system.deviced"
#define DEVICED_OBJECT_PATH    "/Org/Tizen/System/DeviceD"
#define DEVICED_INTERFACE_NAME DEVICED_BUS_NAME

#define DEVICED_PATH		DEVICED_OBJECT_PATH"/Process"
#define DEVICED_INTERFACE	DEVICED_INTERFACE_NAME".Process"

#define DEVICED_SET_METHOD	"oomadj_set"

#define DBUS_REPLY_TIMEOUT (120 * 1000)

static struct _info {
	DBusConnection *connection;
} s_info = {
	.connection = NULL,
};

static DBusConnection *_dbus_connection_get(void) {
	if (s_info.connection == NULL) {
		_W("no connection for dbus. get dbus connection");
		DBusError derror;
		DBusConnection *connection = NULL;

		dbus_error_init(&derror);
		connection = dbus_bus_get_private(DBUS_BUS_SYSTEM, &derror);
		if (connection == NULL) {
			_E("Failed to get dbus connection:%s", derror.message);
			dbus_error_free(&derror);
			return NULL;
		}
		dbus_connection_setup_with_g_main(connection, NULL);
		dbus_error_free(&derror);

		s_info.connection = connection;
	}

	return s_info.connection;
}

static int _append_variant(DBusMessageIter *iter, const char *sig, char *param[])
{
	char *ch;
	int i;
	int int_type;
	uint64_t int64_type;

	if (!sig || !param)
		return 0;

	for (ch = (char*)sig, i = 0; *ch != '\0'; ++i, ++ch) {
		switch (*ch) {
		case 'i':
			int_type = atoi(param[i]);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_INT32, &int_type);
			break;
		case 'u':
			int_type = atoi(param[i]);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT32, &int_type);
			break;
		case 't':
			int64_type = atoi(param[i]);
			dbus_message_iter_append_basic(iter, DBUS_TYPE_UINT64, &int64_type);
			break;
		case 's':
			dbus_message_iter_append_basic(iter, DBUS_TYPE_STRING, &param[i]);
			break;
		default:
			return -EINVAL;
		}
	}

	return 0;
}

static int _invoke_dbus_method_sync(const char *dest, const char *path,
		const char *interface, const char *method,
		const char *sig, char *param[])
{
	DBusMessage *msg;
	DBusMessageIter iter;
	DBusMessage *reply;
	DBusError err;
	int r, ret;

	DBusConnection *conn = NULL;

	conn = (DBusConnection *)_dbus_connection_get();
	if (!conn) {
		_E("dbus_bus_get error");
		return -1;
	}

	msg = dbus_message_new_method_call(dest, path, interface, method);
	if (!msg) {
		_E("dbus_message_new_method_call(%s:%s-%s)", path, interface, method);
		return -EBADMSG;
	}

	dbus_message_iter_init_append(msg, &iter);
	r = _append_variant(&iter, sig, param);
	if (r < 0) {
		_E("append_variant error(%d)", r);
		dbus_message_unref(msg);
		return -EBADMSG;
	}

	dbus_error_init(&err);

	reply = dbus_connection_send_with_reply_and_block(conn, msg, DBUS_REPLY_TIMEOUT, &err);
	dbus_message_unref(msg);
	if (!reply) {
		_E("dbus_connection_send error(%s:%s)", err.name, err.message);
		dbus_error_free(&err);
		return -EBADMSG;
	}

	return reply;
}

static int _dbus_message_send(const char *path, const char *interface, const char *member)
{
	int ret = 0;
	DBusMessage *msg = NULL;
	DBusConnection *conn = NULL;

	conn = (DBusConnection *)_dbus_connection_get();
	if (!conn) {
		_E("dbus_bus_get error");
		return -1;
	}

	msg = dbus_message_new_signal(path, interface, member);
	if (!msg) {
		_E("dbus_message_new_signal(%s:%s-%s)", path, interface, member);
		return -1;
	}

	ret = dbus_connection_send(conn, msg, NULL); //async call
	dbus_message_unref(msg);
	if (ret != TRUE) {
		_E("dbus_connection_send error(%s:%s-%s)", path, interface, member);
		return -ECOMM;
	}
	_D("dbus_connection_send, ret=%d", ret);
	return 0;
}

void starter_dbus_home_raise_signal_send(void)
{
	int ret = 0;

	ret = _dbus_message_send(
			DBUS_HOME_RAISE_PATH,
			DBUS_HOME_RAISE_INTERFACE,
			DBUS_HOME_RAISE_MEMBER);
	_E("Sending HOME RAISE signal, result:%d", ret);
}

int starter_dbus_set_oomadj(int pid, int oom_adj_value)
{
	if(pid <= 0){
		_E("Pid is invalid");
		return -1;
	}
	DBusError err;
	DBusMessage *msg;
	char *pa[4];
	char buf1[16];
	char buf2[16];
	int ret, val;

	snprintf(buf1, sizeof(buf1), "%d", pid);
	snprintf(buf2, sizeof(buf2), "%d", oom_adj_value);

	pa[0] = DEVICED_SET_METHOD;
	pa[1] = "2";
	pa[2] = buf1;
	pa[3] = buf2;

	msg = _invoke_dbus_method_sync(DEVICED_BUS_NAME, DEVICED_PATH, DEVICED_INTERFACE, DEVICED_SET_METHOD, "siss", pa);
	if (!msg)
		return -EBADMSG;

	dbus_error_init(&err);

	ret = dbus_message_get_args(msg, &err, DBUS_TYPE_INT32, &val, DBUS_TYPE_INVALID);
	if (!ret) {
		_E("no message : [%s:%s]", err.name, err.message);
		val = -EBADMSG;
	}

	dbus_message_unref(msg);
	dbus_error_free(&err);

	_D("%s-%s : %d", DEVICED_INTERFACE, pa[0], val);
	return val;
}
