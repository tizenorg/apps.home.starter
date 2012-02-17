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
#define MAXFILELEN	1048576
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
	gmtime_r(&current_time, &local_t);
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
		return;
	} else {
		debugString[len] = '\n';
	}
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
			} else
				fclose(fd);
		}
	}
}
