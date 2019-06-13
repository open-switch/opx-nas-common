/*
 * Copyright (c) 2019 Dell Inc.
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

/*
 * filename: nas_com_bridge_utils.h
 */

#ifndef NAS_COM_BRIDGE_UTILS_H_
#define NAS_COM_BRIDGE_UTILS_H_

#include "std_error_codes.h"
#include "ds_common_types.h"

t_std_error nas_com_get_1d_br_untag_vid(const char *br_name, hal_vlan_id_t &vlan_id);


t_std_error nas_com_add_1d_br_untag_vid(const char *br_name, hal_vlan_id_t vlan_id);


t_std_error nas_com_del_1d_br_untag_vid(const char *br_name);


t_std_error nas_com_set_1d_br_reserved_vid(hal_vlan_id_t vlan_id);

#endif

