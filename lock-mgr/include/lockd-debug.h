/*
 * Copyright (c) 2000 - 2012 Samsung Electronics Co., Ltd All Rights Reserved 
 *
 * This file is part of <starter>
 * Written by <Seungtaek Chung> <seungtaek.chung@samsung.com>, <Mi-Ju Lee> <miju52.lee@samsung.com>, <Xi Zhichan> <zhichan.xi@samsung.com>
 *
 * PROPRIETARY/CONFIDENTIAL
 *
 * This software is the confidential and proprietary information of SAMSUNG ELECTRONICS ("Confidential Information").
 * You shall not disclose such Confidential Information and shall use it only in accordance
 * with the terms of the license agreement you entered into with SAMSUNG ELECTRONICS.
 * SAMSUNG make no representations or warranties about the suitability of the software,
 * either express or implied, including but not limited to the implied warranties of merchantability,
 * fitness for a particular purpose, or non-infringement.
 * SAMSUNG shall not be liable for any damages suffered by licensee as a result of using,
 * modifying or distributing this software or its derivatives.
 *
 */

#ifndef __LOCKD_DEBUG_H__
#define __LOCKD_DEBUG_H__

#include <stdio.h>
#include <dlog.h>

#ifdef  LOG_TAG
#undef  LOG_TAG
#endif

#define LOG_TAG "starter"

#define ENABLE_LOG_SYSTEM

void lockd_log_t(char *fmt, ...);

#ifdef ENABLE_LOG_SYSTEM
#define STARTER_ERR(fmt, arg...)  LOGE("["LOG_TAG"%s:%d:E] "fmt, __FILE__, __LINE__, ##arg)
#define STARTER_DBG(fmt, arg...)  LOGD("["LOG_TAG"%s:%d:D] "fmt, __FILE__, __LINE__, ##arg)
#else
#define STARTER_ERR(fmt, arg...)
#define STARTER_DBG(fmt, arg...)
#endif

#ifdef ENABLE_LOG_SYSTEM
#define _ERR(fmt, arg...) do { STARTER_ERR(fmt, ##arg); lockd_log_t("["LOG_TAG":%d:E] "fmt, __LINE__, ##arg); } while (0)
#define _DBG(fmt, arg...) do { STARTER_DBG(fmt, ##arg); lockd_log_t("["LOG_TAG":%d:D] "fmt, __LINE__, ##arg); } while (0)

#define LOCKD_ERR(fmt, arg...) _ERR(fmt, ##arg)
#define LOCKD_DBG(fmt, arg...) _DBG(fmt, ##arg)
#else
#define _ERR(...)
#define _DBG(...)

#define LOCKD_ERR(...)
#define LOCKD_ERR(...)
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif				/* __LOCKD_DEBUG_H__ */
