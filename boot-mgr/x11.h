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

#ifndef __STARTER_X11_H__
#define __STARTER_X11_H__

void prop_string_set(const char *name, const char *value);
void prop_int_set(const char *name, unsigned int val);
void set_window_scale(void);

#endif				/* __STARTER_X11_H__ */
