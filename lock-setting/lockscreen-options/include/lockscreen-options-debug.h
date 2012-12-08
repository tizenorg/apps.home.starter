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



#ifndef __LOCKSCREEN_OPTIONS_DEBUG_H__
#define __LOCKSCREEN_OPTIONS_DEBUG_H__

#include <stdio.h>

#define OPENLOCK_USING_PLATFORM_DEBUG

#ifdef OPENLOCK_USING_PLATFORM_DEBUG
#ifndef LOG_TAG
#define LOG_TAG "lockscreen-options"
#endif
#include <dlog.h>

#define LOCKOPTIONS_DBG(fmt, args...) LOGD("["LOG_TAG"%s:%d:E] "fmt, __FILE__, __LINE__, ##args)
#define LOCKOPTIONS_WARN(fmt, args...) LOGW("["LOG_TAG"%s:%d:E] "fmt, __FILE__, __LINE__, ##args)
#define LOCKOPTIONS_ERR(fmt, args...) LOGE("["LOG_TAG"%s:%d:E] "fmt, __FILE__, __LINE__, ##args)
#else
#define LOCKOPTIONS_DBG(fmt, args...) do{printf("[LOCKOPTIONS_DBG][%s(%d)] "fmt " \n", __FILE__, __LINE__, ##args);}while(0);
#define LOCKOPTIONS_WARN(fmt, args...) do{printf("[LOCKOPTIONS_WARN][%s(%d)] "fmt " \n", __FILE__, __LINE__, ##args);}while(0);
#define LOCKOPTIONS_ERR(fmt, args...) do{printf("[LOCKOPTIONS_ERR][%s(%d)] "fmt " \n", __FILE__, __LINE__, ##args);}while(0);
#endif

#ifndef TRUE
#define TRUE 1
#endif
#ifndef FALSE
#define FALSE 0
#endif

#endif
