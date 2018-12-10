/*
 * Copyright (c) 2018 Dell Inc.
 *
 * Licensed under the Apache License, Version 2.0 (the "License"); you may
 * not use this file except in compliance with the License. You may obtain
 * a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
 *
 * THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
 * CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
 * LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
 * FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
 *
 * See the Apache Version 2.0 License for specific language governing
 * permissions and limitations under the License.
 */

/**
 * filename: nas_com_log.h
 **/


#ifndef _NAS_COM_LOG_H_
#define _NAS_COM_LOG_H_

#include "event_log.h"

#define NAS_COM_TRACE(...) \
      EV_LOGGING (NAS_COM, DEBUG, "NAS-COM" ,##__VA_ARGS__)

#define NAS_COM_ERR(...) \
      EV_LOGGING (NAS_COM, ERR, "NAS-COM" ,##__VA_ARGS__)

#endif
