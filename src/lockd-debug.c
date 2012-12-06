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



#include <stdio.h>
#include <stdarg.h>
#include <fcntl.h>
#include <time.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <glib.h>

#include "lockd-debug.h"

#define LINEMAX 256
#define MAXFILELEN	1048576	/* 32000 */
#define LOGFILE "/tmp/starter.log"

void lockd_log_t(char *fmt, ...)
{
	va_list ap;
	FILE *fd = 0;
	char buf[LINEMAX] = { 0, };
	char debugString[LINEMAX] = { 0, };

	va_start(ap, fmt);
	vsnprintf(buf, sizeof(buf), fmt, ap);
	va_end(ap);
	int fileLen = 0;
	struct tm local_t;
	time_t current_time = 0;
	bzero((char *)&debugString, LINEMAX);
	time(&current_time);
	/* local_t = gmtime(&current_time); */
	gmtime_r(&current_time, &local_t);	/* for prevent 53555 */
	int len = snprintf(debugString, sizeof(debugString),
			   "[%d-%02d-%02d, %02d:%02d:%02d]: ",
			   local_t.tm_year + 1900, local_t.tm_mon + 1,
			   local_t.tm_mday, local_t.tm_hour, local_t.tm_min,
			   local_t.tm_sec);
	if (len == -1) {
		return;
	} else {
		debugString[len] = '\0';
	}
	len = g_strlcat(debugString, buf, LINEMAX);
	if (len >= LINEMAX) {
		/* TODO:ERROR handling */
		return;
	} else {
		debugString[len] = '\n';
	}
	/* FIXME this is for permission.. later we should fix and remove this... */
	/* system("chmod 666 "LOGFILE); */
	if ((fd = fopen(LOGFILE, "at+")) == NULL) {
		LOCKD_DBG("File fopen fail for writing Pwlock information");
	} else {
		int pid = -1;
		if (fwrite(debugString, strlen(debugString), 1, fd) < 1) {
			LOCKD_DBG
			    ("File fwrite fail for writing Pwlock information");
			fclose(fd);
			if ((pid = fork()) < 0) {
			} else if (pid == 0) {
				execl("/bin/rm", "rm", "-f", LOGFILE,
				      (char *)0);
			}
			/* system("rm -rf "LOGFILE);  */
		} else {
			fseek(fd, 0l, SEEK_END);
			fileLen = ftell(fd);
			if (fileLen > MAXFILELEN) {
				fclose(fd);
				if ((pid = fork()) < 0) {
					return;
				} else if (pid == 0) {
					execl("/bin/rm", "rm", "-f", LOGFILE,
					      (char *)0);
				}
				/* system("rm -rf "LOGFILE); */
			} else
				fclose(fd);
		}
	}
}
