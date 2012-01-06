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

#include <Elementary.h>

#include <vconf.h>
#include <vconf-keys.h>
#include <glib.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h>
#include <sys/param.h>
#include <errno.h>

#include "lockd-debug.h"
#include "lock-daemon.h"
#include "lockd-process-mgr.h"

static int phone_lock_pid;

struct lockd_data {
	int phone_lock_app_pid;
	int phone_lock_state;
};

struct ucred {
	pid_t pid;
	uid_t uid;
	gid_t gid;
};

#define VCONFKEY_PHONE_LOCK_VERIFICATION "memory/lockscreen/phone_lock_verification"
#define PHLOCK_SOCK_PREFIX "/tmp/phlock"
#define PHLOCK_SOCK_MAXBUFF 65535
#define PHLOCK_APP_CMDLINE "/opt/apps/org.tizen.phone-lock/bin/phone-lock"

static void lockd_launch_app_lockscreen(struct lockd_data *lockd);

static void _lockd_notify_pm_state_cb(keynode_t * node, void *data)
{
	LOCKD_DBG("PM state Notification!!");

	struct lockd_data *lockd = (struct lockd_data *)data;
	int val = -1;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	if (vconf_get_int(VCONFKEY_PM_STATE, &val) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY_PM_STATE");
		return;
	}

	if (val == VCONFKEY_PM_STATE_LCDOFF) {
		lockd_launch_app_lockscreen(lockd);
	}
}

static void
_lockd_notify_lock_state_cb(keynode_t * node, void *data)
{
	LOCKD_DBG("lock state changed!!");

	struct lockd_data *lockd = (struct lockd_data *)data;
	int val = -1;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	if (vconf_get_int(VCONFKEY_IDLE_LOCK_STATE, &val) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY_IDLE_LOCK_STATE");
		return;
	}

	if (val == VCONFKEY_IDLE_UNLOCK) {
		LOCKD_DBG("unlocked..!!");
	}
}

static void
_lockd_notify_phone_lock_verification_cb(keynode_t * node, void *data)
{
	LOCKD_DBG("%s, %d", __func__, __LINE__);

	struct lockd_data *lockd = (struct lockd_data *)data;
	int val = -1;

	if (lockd == NULL) {
		LOCKD_ERR("lockd is NULL");
		return;
	}

	if (vconf_get_bool(VCONFKEY_PHONE_LOCK_VERIFICATION, &val) < 0) {
		LOCKD_ERR("Cannot get VCONFKEY_PHONE_LOCK_VERIFICATION");
		return;
	}

	if (val == TRUE) {
		vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_UNLOCK);
	}
}

static void lockd_launch_app_lockscreen(struct lockd_data *lockd)
{
	LOCKD_DBG("launch app lock screen");

	int call_state = -1, bootlock_state = -1;

	vconf_get_bool(VCONFKEY_SETAPPL_STATE_POWER_ON_LOCK_BOOL,
		       &bootlock_state);

	if (bootlock_state) {
		lockd->phone_lock_state = 1;
	} else {
		lockd->phone_lock_state = 0;
	}

	vconf_get_int(VCONFKEY_CALL_STATE, &call_state);
	if (call_state != VCONFKEY_CALL_OFF) {
		LOCKD_DBG
		    ("Current call state(%d) does not allow to launch lock screen.",
		     call_state);
		return;
	}

	if (lockd->phone_lock_state == 1) {
		vconf_set_bool(VCONFKEY_PHONE_LOCK_VERIFICATION, FALSE);
		/* Check phone lock application is already exit */
		if (lockd_process_mgr_check_lock(lockd->phone_lock_app_pid) == TRUE) {
			LOCKD_DBG("phone lock App is already running.");
			return;
		}
		vconf_set_int(VCONFKEY_IDLE_LOCK_STATE, VCONFKEY_IDLE_LOCK);
		lockd->phone_lock_app_pid =
		    lockd_process_mgr_start_phone_lock();
		phone_lock_pid = lockd->phone_lock_app_pid;
		LOCKD_DBG("%s, %d, phone_lock_pid = %d", __func__, __LINE__,
			  phone_lock_pid);
	}
}

inline static void lockd_set_sock_option(int fd, int cli)
{
	int size;
	struct timeval tv = { 1, 200 * 1000 };

	size = PHLOCK_SOCK_MAXBUFF;
	setsockopt(fd, SOL_SOCKET, SO_SNDBUF, &size, sizeof(size));
	setsockopt(fd, SOL_SOCKET, SO_RCVBUF, &size, sizeof(size));
	if (cli)
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

static int lockd_create_sock(void)
{
	struct sockaddr_un saddr;
	int fd;

	fd = socket(AF_UNIX, SOCK_STREAM, 0);
	if (fd < 0) {
		if (errno == EINVAL) {
			fd = socket(AF_UNIX, SOCK_STREAM, 0);
			if (fd < 0) {
				LOCKD_DBG
				    ("second chance - socket create error");
				return -1;
			}
		} else {
			LOCKD_DBG("socket error");
			return -1;
		}
	}

	bzero(&saddr, sizeof(saddr));
	saddr.sun_family = AF_UNIX;

	strncpy(saddr.sun_path, PHLOCK_SOCK_PREFIX, strlen(PHLOCK_SOCK_PREFIX));
	saddr.sun_path[strlen(PHLOCK_SOCK_PREFIX)] = 0;

	unlink(saddr.sun_path);

	if (bind(fd, (struct sockaddr *)&saddr, sizeof(saddr)) < 0) {
		LOCKD_DBG("bind error");
		return -1;
	}

	if (chmod(saddr.sun_path, (S_IRWXU | S_IRWXG | S_IRWXO)) < 0) {
		LOCKD_DBG("failed to change the socket permission");
		return -1;
	}

	lockd_set_sock_option(fd, 0);

	if (listen(fd, 10) == -1) {
		LOCKD_DBG("listen error");
		return -1;
	}

	return fd;
}

static gboolean lockd_glib_check(GSource * src)
{
	GSList *fd_list;
	GPollFD *tmp;

	fd_list = src->poll_fds;
	do {
		tmp = (GPollFD *) fd_list->data;
		if ((tmp->revents & (POLLIN | POLLPRI)))
			return TRUE;
		fd_list = fd_list->next;
	} while (fd_list);

	return FALSE;
}

static char *lockd_read_cmdline_from_proc(int pid)
{
	int memsize = 32;
	char path[32];
	char *cmdline = NULL, *tempptr = NULL;
	FILE *fp = NULL;

	snprintf(path, sizeof(path), "/proc/%d/cmdline", pid);

	fp = fopen(path, "r");
	if (fp == NULL) {
		LOCKD_DBG("Cannot open cmdline on pid[%d]", pid);
		return NULL;
	}

	cmdline = malloc(32);
	if (cmdline == NULL) {
		LOCKD_DBG("%s", "Out of memory");
		fclose(fp);
		return NULL;
	}

	bzero(cmdline, memsize);
	if (fgets(cmdline, 32, fp) == NULL) {
		LOCKD_DBG("%s", "Cannot read cmdline");
		free(cmdline);
		fclose(fp);
		return NULL;
	}

	while (cmdline[memsize - 2] != 0) {
		cmdline[memsize - 1] = (char)fgetc(fp);
		tempptr = realloc(cmdline, memsize + 32);
		if (tempptr == NULL) {
			fclose(fp);
			LOCKD_DBG("%s", "Out of memory");
			return NULL;
		}
		cmdline = tempptr;
		bzero(cmdline + memsize, 32);
		fgets(cmdline + memsize, 32, fp);
		memsize += 32;
	}

	if (fp != NULL)
		fclose(fp);
	return cmdline;
}

static int lockd_sock_handler(int fd)
{
	int cl;
	int len;
	int sun_size;
	int clifd = -1;
	char cmd[PHLOCK_SOCK_MAXBUFF];
	char *cmdline = NULL;

	struct ucred cr;
	struct sockaddr_un lockd_addr;

	cl = sizeof(cr);
	sun_size = sizeof(struct sockaddr_un);

	if ((clifd =
	     accept(fd, (struct sockaddr *)&lockd_addr,
		    (socklen_t *) & sun_size)) == -1) {
		if (errno != EINTR)
			LOCKD_DBG("accept error");
		return -1;
	}

	if (getsockopt(clifd, SOL_SOCKET, SO_PEERCRED, &cr, (socklen_t *) & cl)
	    < 0) {
		LOCKD_DBG("peer information error");
		close(clifd);
		return -1;
	}
	LOCKD_DBG("Peer's pid=%d, uid=%d, gid=%d\n", cr.pid, cr.uid, cr.gid);

	memset(cmd, 0, PHLOCK_SOCK_MAXBUFF);

	lockd_set_sock_option(clifd, 1);

	len = recv(clifd, cmd, PHLOCK_SOCK_MAXBUFF, 0);

	if (cmd == NULL) {
		LOCKD_DBG("recv error, cmd is NULL");
		close(clifd);
		return -1;
	}

	if (len != strlen(cmd)) {
		LOCKD_DBG("recv error %d %d", len, strlen(cmd));
		close(clifd);
		return -1;
	}

	LOCKD_DBG("cmd %s", cmd);

	cmdline = lockd_read_cmdline_from_proc(cr.pid);
	if (cmdline == NULL) {
		LOCKD_DBG("Error on opening /proc/%d/cmdline", cr.pid);
		return -1;
	}

	LOCKD_DBG("/proc/%d/cmdline : %s", cr.pid, cmdline);

	if ((!strncmp(cmdline, PHLOCK_APP_CMDLINE, strlen(cmdline)))
	    && (phone_lock_pid == cr.pid)) {
		LOCKD_DBG("pid [%d] %s is verified, unlock..!!\n", cr.pid,
			  cmdline);
		lockd_process_mgr_terminate_phone_lock(phone_lock_pid);
		phone_lock_pid = 0;
		vconf_set_bool(VCONFKEY_PHONE_LOCK_VERIFICATION, TRUE);
	}

	return 0;
}

static gboolean lockd_glib_handler(gpointer data)
{
	GPollFD *gpollfd = (GPollFD *) data;
	if (lockd_sock_handler(gpollfd->fd) < 0) {
		LOCKD_DBG("lockd_sock_handler is failed..!!");
	}
	return TRUE;
}

static gboolean lockd_glib_dispatch(GSource * src, GSourceFunc callback,
				    gpointer data)
{
	callback(data);
	return TRUE;
}

static gboolean lockd_glib_prepare(GSource * src, gint * timeout)
{
	return FALSE;
}

static GSourceFuncs funcs = {
	.prepare = lockd_glib_prepare,
	.check = lockd_glib_check,
	.dispatch = lockd_glib_dispatch,
	.finalize = NULL
};

static int lockd_init_sock(void)
{
	int fd;
	GPollFD *gpollfd;
	GSource *src;
	int ret;

	fd = lockd_create_sock();
	if (fd < 0) {
		LOCKD_DBG("lock daemon create sock failed..!!");
	}

	src = g_source_new(&funcs, sizeof(GSource));

	gpollfd = (GPollFD *) g_malloc(sizeof(GPollFD));
	gpollfd->events = POLLIN;
	gpollfd->fd = fd;

	g_source_add_poll(src, gpollfd);
	g_source_set_callback(src, (GSourceFunc) lockd_glib_handler,
			      (gpointer) gpollfd, NULL);
	g_source_set_priority(src, G_PRIORITY_LOW);

	ret = g_source_attach(src, NULL);
	if (ret == 0)
		return -1;

	g_source_unref(src);

	return 0;
}

static void lockd_init_vconf(struct lockd_data *lockd)
{
	int val = -1;

	if (vconf_notify_key_changed
	    (VCONFKEY_PM_STATE, _lockd_notify_pm_state_cb, lockd) != 0) {
		LOCKD_ERR("Fail vconf_notify_key_changed : VCONFKEY_PM_STATE");
	}

	if (vconf_notify_key_changed
	    (VCONFKEY_PHONE_LOCK_VERIFICATION,
	     _lockd_notify_phone_lock_verification_cb, lockd) != 0) {
		if (vconf_get_bool(VCONFKEY_PHONE_LOCK_VERIFICATION, &val) < 0) {
			LOCKD_ERR
			    ("Cannot get VCONFKEY_PHONE_LOCK_VERIFICATION");
			vconf_set_bool(VCONFKEY_PHONE_LOCK_VERIFICATION, 0);
			if (vconf_notify_key_changed
			    (VCONFKEY_PHONE_LOCK_VERIFICATION,
			     _lockd_notify_phone_lock_verification_cb,
			     lockd) != 0) {
				LOCKD_ERR
				    ("Fail vconf_notify_key_changed : VCONFKEY_PHONE_LOCK_VERIFICATION");
			}
		} else {
			LOCKD_ERR
			    ("Fail vconf_notify_key_changed : VCONFKEY_PHONE_LOCK_VERIFICATION");
		}
	}

	if (vconf_notify_key_changed
	    (VCONFKEY_IDLE_LOCK_STATE,
	     _lockd_notify_lock_state_cb,
	     lockd) != 0) {
		LOCKD_ERR
		    ("[Error] vconf notify : lock state");
	}
	
}

static void lockd_start_lock_daemon(void *data)
{
	struct lockd_data *lockd = NULL;
	int r = 0;

	lockd = (struct lockd_data *)data;

	if (!lockd) {
		return;
	}

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	lockd_init_vconf(lockd);

	r = lockd_init_sock();
	if (r < 0) {
		LOCKD_DBG("lockd init socket failed: %d", r);
	}

	LOCKD_DBG("%s, %d", __func__, __LINE__);
}

int start_lock_daemon()
{
	struct lockd_data *lockd = NULL;
	int val = -1;

	LOCKD_DBG("%s, %d", __func__, __LINE__);

	lockd = (struct lockd_data *)malloc(sizeof(struct lockd_data));
	memset(lockd, 0x0, sizeof(struct lockd_data));
	lockd_start_lock_daemon(lockd);

	vconf_get_bool(VCONFKEY_SETAPPL_STATE_POWER_ON_LOCK_BOOL, &val);
	LOCKD_DBG("%s, %d, val = %d", __func__, __LINE__, val);

	if (val) {
		lockd_launch_app_lockscreen(lockd);
	}

	return 0;
}
