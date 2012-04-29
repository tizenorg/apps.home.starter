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

