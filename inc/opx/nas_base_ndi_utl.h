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

/*!
 * \file   nas_base_ndi_utl.h
 * \brief  Multi-NPU NDI programming utilities used by Base Object commit.
 *  Common logic to provision and rollback objects to/from NDI 
 *  with multi-NPU support.
 * \date   02-2015
 */

#ifndef _NAS_BASE_NDI_UTL_H_
#define _NAS_BASE_NDI_UTL_H_

#include "nas_base_obj.h"
#include <string> 

namespace nas {

void base_create_obj_ndi (base_obj_t& t_new, bool rolling_back);

void base_delete_obj_ndi (base_obj_t& t_del, bool rolling_back);

void base_modify_obj_ndi (base_obj_t& t_new, base_obj_t& t_old, 
                          bool rolling_back);

}

#endif
