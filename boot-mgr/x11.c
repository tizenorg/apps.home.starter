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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

#define DEFAULT_WINDOW_H 1280

void prop_string_set(const char *name, const char *value)
{
	Display *d;
	Atom a_name;
	Atom a_UTF8;
	XTextProperty xtp;

	if (name == NULL || value == NULL || value[0] == '\0')
		return;

	d = XOpenDisplay(NULL);
	if (d == NULL)
		return;

	a_name = XInternAtom(d, name, False);
	if (a_name == None)
		goto exit;

	a_UTF8 = XInternAtom(d, "UTF8_STRING", False);
	if (a_UTF8 == None)
		goto exit;

	xtp.value = (unsigned char *)value;
	xtp.format = 8;
	xtp.encoding = a_UTF8;
	xtp.nitems = strlen(value);

	XSetTextProperty(d, DefaultRootWindow(d), &xtp, a_name);

 exit:
	XCloseDisplay(d);
}

void prop_int_set(const char *name, unsigned int val)
{
	Display *d;
	Atom a_name;

	if (name == NULL)
		return;

	d = XOpenDisplay(NULL);
	if (d == NULL)
		return;

	a_name = XInternAtom(d, name, False);
	if (a_name == None)
		goto exit;

	XChangeProperty(d, DefaultRootWindow(d), a_name, XA_CARDINAL, 32,
			PropModeReplace, (unsigned char *)&val, 1);

 exit:
	XCloseDisplay(d);
}

void set_window_scale(void)
{
	double root_width = 0.0, root_height = 0.0;
	char buf[128] = { 0, };
	Display *disp;
	int screen_num;

	disp = XOpenDisplay(NULL);
	if (disp == NULL)
		return;

	screen_num = DefaultScreen(disp);

	root_width = DisplayWidth(disp, screen_num);
	root_height = DisplayHeight(disp, screen_num);

	XCloseDisplay(disp);

	snprintf(buf, sizeof(buf), "%lf", root_height / DEFAULT_WINDOW_H);

	if (root_width == 800 && root_height == 1280) {
		snprintf(buf, sizeof(buf), "0.71");
	}

	setenv("ELM_SCALE", buf, 1);
	setenv("SCALE_FACTOR", buf, 1);
}
