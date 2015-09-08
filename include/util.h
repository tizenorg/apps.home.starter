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

#ifndef __MENU_DAEMON_UTIL_H__
#define __MENU_DAEMON_UTIL_H__
#include <dlog.h>
#include <stdio.h>
#include <sys/time.h>

#define APP_TRAY_PKG_NAME "com.samsung.app-tray"
#define MENU_SCREEN_PKG_NAME "org.tizen.menu-screen"
#define PROVIDER_PKG_NAME "com.samsung.data-provider-master"
#define SVOICE_PKG_NAME "com.samsung.svoice"
#define CLUSTER_HOME_PKG_NAME "com.samsung.cluster-home"
#define EASY_HOME_PKG_NAME "com.samsung.easy-home"
#define EASY_APPS_PKG_NAME "com.samsung.easy-apps"
#define HOMESCREEN_PKG_NAME "org.tizen.homescreen"
#define TASKMGR_PKG_NAME "org.tizen.task-mgr"
#define DEFAULT_TASKMGR_PKG_NAME "com.samsung.taskmgr"
#define CONF_PATH_NUMBER 1024

#ifdef  LOG_TAG
#undef  LOG_TAG
#define LOG_TAG "STARTER"
#endif

/* Log */
#if !defined(_W)
#define _W(fmt, arg...) LOGW("[%s:%d] "fmt"\n", __func__, __LINE__, ##arg)
#endif

#if !defined(_D)
#define _D(fmt, arg...) LOGD("[%s:%d] "fmt"\n", __func__, __LINE__, ##arg)
#endif

#if !defined(_E)
#define _E(fmt, arg...) LOGE("[%s:%d] "fmt"\n", __func__, __LINE__, ##arg)
#endif

#if !defined(_SECURE_W)
#define _SECURE_W(fmt, arg...) SECURE_LOGW("[%s:%d] "fmt"\n", __func__, __LINE__, ##arg)
#endif

#if !defined(_SECURE_D)
#define _SECURE_D(fmt, arg...) SECURE_LOGD("[%s:%d] "fmt"\n", __func__, __LINE__, ##arg)
#endif

#if !defined(_SECURE_E)
#define _SECURE_E(fmt, arg...) SECURE_LOGE("[%s:%d] "fmt"\n", __func__, __LINE__, ##arg)
#endif

#define retv_if(expr, val) do { \
	if(expr) { \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		return (val); \
	} \
} while (0)

#define ret_if(expr) do { \
	if(expr) { \
		_E("(%s) -> %s() return", #expr, __FUNCTION__); \
		return; \
	} \
} while (0)

#define goto_if(expr, val) do { \
	if(expr) { \
		_E("(%s) -> goto", #expr); \
		goto val; \
	} \
} while (0)

#define break_if(expr) { \
	if(expr) { \
		_E("(%s) -> break", #expr); \
		break; \
	} \
}

#define continue_if(expr) { \
	if(expr) { \
		_E("(%s) -> continue", #expr); \
		continue; \
	} \
}

#define PRINT_TIME(str) do { \
	struct timeval tv; \
	gettimeofday(&tv, NULL); \
	_D("[%s:%d] %s TIME=%u.%u", __func__, __LINE__, str, (int)tv.tv_sec, (int)tv.tv_usec); \
} while (0)

#define _F(fmt, arg...) do {            \
	FILE *fp;\
	fp = fopen("/var/log/starter.log", "a+");\
	if (NULL == fp) break;\
    fprintf(fp, "[%s:%d] "fmt"\n", __func__, __LINE__, ##arg); \
	fclose(fp);\
} while (0)



#endif /* __MENU_DAEMON_UTIL_H__ */
