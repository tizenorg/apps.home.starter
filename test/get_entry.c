 /*
  * Copyright 2012  Samsung Electronics Co., Ltd
  *
  * Licensed under the Flora License, Version 1.0 (the "License");
  * you may not use this file except in compliance with the License.
  * You may obtain a copy of the License at
  *
  *     http://floralicense.org/license/
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
	unsigned char *prop_ret;
	Atom type_ret;
	unsigned long bytes_after, num_ret;
	int format_ret;
	unsigned int i;
	int num;

	Display *d;
	Atom a_ac;
	Atom a_ap;
	Status r;

	d = XOpenDisplay(NULL);
	if (d == NULL) {
		printf("Display open error\n");
		return 1;
	}

	a_ac = XInternAtom(d, "ENLIGHTENMENT_AUTOCAPITAL_ALLOW", False);
	if (a_ac == None) {
		printf("XInternAtom error\n");
		goto exit;
	}

	r = XGetWindowProperty(d, DefaultRootWindow(d), a_ac, 0, 0x7fffffff,
			       False, XA_CARDINAL, &type_ret, &format_ret,
			       &num_ret, &bytes_after, &prop_ret);
	if (r != Success) {
		printf("XGetWindowProperty error\n");
		goto exit;
	}

	if (type_ret == XA_CARDINAL && format_ret == 32 && num_ret > 0
	    && prop_ret) {
		printf("Auto capital: %lu\n", ((unsigned long *)prop_ret)[0]);
	}
	if (prop_ret)
		XFree(prop_ret);

	a_ap = XInternAtom(d, "ENLIGHTENMENT_AUTOPERIOD_ALLOW", False);
	if (a_ap == None) {
		printf("XInternAtom error\n");
		goto exit;
	}

	r = XGetWindowProperty(d, DefaultRootWindow(d), a_ap, 0, 0x7fffffff,
			       False, XA_CARDINAL, &type_ret, &format_ret,
			       &num_ret, &bytes_after, &prop_ret);
	if (r != Success) {
		printf("XGetWindowProperty error\n");
		goto exit;
	}

	if (type_ret == XA_CARDINAL && format_ret == 32 && num_ret > 0
	    && prop_ret) {
		printf("Auto period: %lu\n", ((unsigned long *)prop_ret)[0]);
	}
	if (prop_ret)
		XFree(prop_ret);

 exit:
	XCloseDisplay(d);
	return 0;
}
