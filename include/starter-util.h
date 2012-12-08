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



#ifndef __STARTER_UTIL_H__
#define __STARTER_UTIL_H__

#include <stdio.h>
#include <sys/time.h>

#define WRITE_FILE_LOG(fmt, arg...) do {            \
	FILE *fp;\
    struct timeval tv;				\
    gettimeofday(&tv, NULL);		\
	fp = fopen("/var/log/boottime", "a+");\
	if (NULL == fp) break;\
    fprintf(fp, "%u%09u : "fmt"\n", (int) tv.tv_sec, (int) tv.tv_usec, ##arg); \
	fclose(fp);\
} while (0)

#endif
