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

#endif
