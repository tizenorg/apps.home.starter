/*
 * Copyright (c) 2000 - 2015 Samsung Electronics Co., Ltd. All rights reserved.
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
 */

#ifndef __NOTIFIER_H__
#define __NOTIFIER_H__
#include <stdbool.h>

#if !defined(PUBLIC)
#define PUBLIC          __attribute__((visibility("default")))  /**<All other from outside modules can access this typed API */
#endif

#if !defined(PROTECTED)
#define PROTECTED       __attribute__((visibility("hidden")))   /**<All other from outside modules can not access this directly */
#endif

#if !defined(PRIVATE)
#define PRIVATE         __attribute__((visibility("internal"))) /**<Does not export APIs to the other. only can be accessed in this module */
#endif


struct desktop_notifier {
    int number;
    int ifd;
    Ecore_Fd_Handler *handler;
};


PRIVATE void pkg_event_init(void);
PRIVATE void pkg_event_fini(void);

#endif
