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

#ifndef __LOCKD_DEBUG_H__
#define __LOCKD_DEBUG_H__

#include <stdio.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif

#define LOG_TAG "STARTER"

#define ENABLE_LOG_SYSTEM

#ifdef ENABLE_LOG_SYSTEM
#define STARTER_ERR(fmt, arg...)  LOGE("["LOG_TAG"%s:%d:E] "fmt, __FILE__, __LINE__, ##arg)
#define STARTER_DBG(fmt, arg...)  LOGD("["LOG_TAG"%s:%d:D] "fmt, __FILE__, __LINE__, ##arg)
#define STARTER_WARN(fmt, arg...)  LOGW("["LOG_TAG"%s:%d:W] "fmt, __FILE__, __LINE__, ##arg)
#define STARTER_SECURE_ERR(fmt, arg...)  SECURE_LOGE("["LOG_TAG"%s:%d:E] "fmt, __FILE__, __LINE__, ##arg)
#define STARTER_SECURE_DBG(fmt, arg...)  SECURE_LOGD("["LOG_TAG"%s:%d:D] "fmt, __FILE__, __LINE__, ##arg)
#define STARTER_SECURE_WARN(fmt, arg...)  SECURE_LOGW("["LOG_TAG"%s:%d:W] "fmt, __FILE__, __LINE__, ##arg)
#else
#define STARTER_ERR(fmt, arg...)
#define STARTER_DBG(fmt, arg...)
#define STARTER_WARN(fmt, arg...)
#define STARTER_SECURE_ERR(fmt, arg...)
#define STARTER_SECURE_DBG(fmt, arg...)
#define STARTER_SECURE_WARN(fmt, arg...)
#endif

#ifdef ENABLE_LOG_SYSTEM
#define _ERR(fmt, arg...) do { STARTER_ERR(fmt, ##arg); } while (0)
#define _DBG(fmt, arg...) do { STARTER_DBG(fmt, ##arg); } while (0)
#define _WARN(fmt, arg...) do { STARTER_WARN(fmt, ##arg); } while (0)

#define _SECURE_ERR(fmt, arg...) do { STARTER_SECURE_ERR(fmt, ##arg); } while (0)
#define _SECURE_DBG(fmt, arg...) do { STARTER_SECURE_DBG(fmt, ##arg); } while (0)
#define _SECURE_WARN(fmt, arg...) do { STARTER_SECURE_WARN(fmt, ##arg); } while (0)

#define LOCKD_ERR(fmt, arg...) _ERR(fmt, ##arg)
#define LOCKD_DBG(fmt, arg...) _DBG(fmt, ##arg)
#define LOCKD_WARN(fmt, arg...) _WARN(fmt, ##arg)
#define LOCKD_SECURE_ERR(fmt, arg...) _SECURE_ERR(fmt, ##arg)
#define LOCKD_SECURE_DBG(fmt, arg...) _SECURE_DBG(fmt, ##arg)
#define LOCKD_SECURE_WARN(fmt, arg...) _SECURE_WARN(fmt, ##arg)
#else
#define _ERR(...)
#define _DBG(...)
#define _WARN(...)
#define _SECURE_ERR(...)
#define _SECURE_DBG(...)
#define _SECURE_WARN(...)

#define LOCKD_ERR(...)
#define LOCKD_DBG(...)
#define LOCKD_WARN(...)
#define LOCKD_SECURE_ERR(...)
#define LOCKD_SECURE_DBG(...)
#define LOCKD_SECURE_WARN(...)
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif				/* __LOCKD_DEBUG_H__ */
