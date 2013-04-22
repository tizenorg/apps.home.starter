 /*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://floralicense.org/license/
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

#define HOME_SCREEN_PKG_NAME "org.tizen.menu-screen"
#define CONF_PATH_NUMBER 1024

#ifdef  LOG_TAG
#undef  LOG_TAG
#define LOG_TAG "starter"
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
