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
 * filename: nas_com_bridge_utils.cpp
 */

#include "event_log.h"
#include "std_error_codes.h"
#include "std_utils.h"
#include "ds_common_types.h"
#include "std_mutex_lock.h"
#include <string>
#include <unordered_map>

//map to maintain parent_bridge_port index (VN) to vlan_id
static auto  bridge_to_vlan_id_map = new std::unordered_map<std::string ,hal_vlan_id_t>;
static hal_vlan_id_t reserved_untagged_vlanid  = 0;


static std_mutex_lock_create_static_init_fast(bridge_to_vlan_id_map_mutex);

void nas_com_get_1d_br_reserved_vid(hal_vlan_id_t *vlan_id) {
   *(vlan_id) = reserved_untagged_vlanid;
}

t_std_error nas_com_get_1d_br_untag_vid(const char *name, hal_vlan_id_t &vlan_id) {

    if (name == NULL) return STD_ERR(INTERFACE,PARAM,0);

    std::string br_name(name);
    std_mutex_simple_lock_guard lock(&bridge_to_vlan_id_map_mutex);
    auto it = bridge_to_vlan_id_map->find(std::string(br_name));
    if (it == bridge_to_vlan_id_map->end()) {
       /* If not in the map it is not an attached case, but send the default untagged VLAN id for 1D bridge  */
       nas_com_get_1d_br_reserved_vid(&vlan_id);
       if (vlan_id == 0) {
           EV_LOGGING(NAS_COM ,ERR, "BRIDGE-UTIL", "failed to find vlan-id for 1d bridge %s", name);
           return STD_ERR(INTERFACE,PARAM,0);

       }
       EV_LOGGING(NAS_COM ,INFO, "BRIDGE-UTIL", "Untagged RESERVED VLAN id for 1d bridge %s is %d ", name, vlan_id);
       return STD_ERR_OK;
    }
    vlan_id = it->second;
    EV_LOGGING(NAS_COM ,INFO, "BRIDGE-UTIL", "Untagged VLAN id for 1d bridge %s is %d ", name, vlan_id);
    return STD_ERR_OK;
}

t_std_error nas_com_add_1d_br_untag_vid(const char * name, hal_vlan_id_t vlan_id) {

    if (name == NULL) return STD_ERR(INTERFACE,PARAM,0);
    EV_LOGGING(NAS_COM ,DEBUG, "BRIDGE-UTIL", "Untagged VLAN id  map add for 1d bridge  %s and %d ",
                                 name, vlan_id);

    std::string br_name(name);
    std_mutex_simple_lock_guard lock(&bridge_to_vlan_id_map_mutex);
    auto it =  bridge_to_vlan_id_map->find(br_name);
    if (it == bridge_to_vlan_id_map->end()) {
        bridge_to_vlan_id_map->insert(make_pair(br_name,vlan_id));
        return STD_ERR_OK;
       /*  already present */
    }
    EV_LOGGING(NAS_COM ,DEBUG, "BRIDGE-UTIL", "Untagged VLAN id  already present for 1d bridge %s and %d ",
                                 it->first.c_str(), it->second);
    return STD_ERR(INTERFACE,PARAM,0);
}

t_std_error nas_com_del_1d_br_untag_vid(const char * name) {

    if (name == NULL) return STD_ERR(INTERFACE,PARAM,0);

    std::string br_name(name);
    EV_LOGGING(NAS_COM ,DEBUG, "BRIDGE-UTIL", "Untagged VLAN id  map delete for 1d bridge  %s ", name);
    std_mutex_simple_lock_guard lock(&bridge_to_vlan_id_map_mutex);
    auto it =  bridge_to_vlan_id_map->find(br_name);
    if (it == bridge_to_vlan_id_map->end()) {
        EV_LOGGING(NAS_COM ,DEBUG, "BRIDGE-UTIL", "Untagged VLAN id  not present for 1d bridge %s ", name);
       /*  not present  */
       return STD_ERR(INTERFACE,PARAM,0);
    }
    bridge_to_vlan_id_map->erase(it);
    return STD_ERR_OK;
}

t_std_error nas_com_set_1d_br_reserved_vid(hal_vlan_id_t vlan_id) {

    std_mutex_simple_lock_guard lock(&bridge_to_vlan_id_map_mutex);
    reserved_untagged_vlanid = vlan_id;
    EV_LOGGING(NAS_COM ,DEBUG, "BRIDGE-UTIL", "Reserved Untagged VLAN id for 1d bridge set to  %d ", vlan_id);
    return STD_ERR_OK;
}

