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
#include <X11/Xlib.h>
#include <X11/Xatom.h>
#include <X11/Xutil.h>

int main(int argc, char *argv[])
{
	XTextProperty xtp;
	Display *d;
	Atom a_e17;
	Status r;

	d = XOpenDisplay(NULL);
	if (d == NULL) {
		printf("Display open error\n");
		return 1;
	}

	a_e17 = XInternAtom(d, "ENLIGHTENMENT_THEME", False);
	if (a_e17 == None) {
		printf("XInternAtom error\n");
		goto exit;
	}

	r = XGetTextProperty(d, DefaultRootWindow(d), &xtp, a_e17);
	if (!r) {
		printf("XGetTextProperty error\n");
		goto exit;
	}

	printf("THEME: [%s]\n", (char *)xtp.value);

	XFree(xtp.value);

 exit:
	XCloseDisplay(d);
	return 0;
}
