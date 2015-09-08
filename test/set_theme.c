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

#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

void set_elm_theme(void)
{
	char *theme;
	Display *d;
	Atom a_e17;
	Atom a_UTF8;
	XTextProperty xtp;

	theme = "beat:kessler";

	d = XOpenDisplay(NULL);
	if (d == NULL)
		return;

	a_e17 = XInternAtom(d, "ENLIGHTENMENT_THEME", False);
	if (a_e17 == None)
		goto exit;

	a_UTF8 = XInternAtom(d, "UTF8_STRING", False);
	if (a_UTF8 == None)
		goto exit;

	xtp.value = (unsigned char *)theme;
	xtp.format = 8;
	xtp.encoding = a_UTF8;
	xtp.nitems = strlen(theme);

	XSetTextProperty(d, DefaultRootWindow(d), &xtp, a_e17);

 exit:
	XCloseDisplay(d);
}

int main(int argc, char *argv[])
{
	set_elm_theme();
	return 0;
}
