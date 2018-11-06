/*
 * Copyright (c) 2017 Dell Inc.
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
 * nas_switch_profile.cpp
 *
 */


#include "std_config_node.h"
#include "ds_common_types.h"
#include "std_error_codes.h"
#include "event_log.h"
#include "std_assert.h"
#include <string.h>
#include "std_envvar.h"
#include "cps_api_object.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"
#include "cps_api_db_interface.h"
#include "cps_api_object_tools.h"
#include "dell-base-switch-element.h"
#include "dell-base-acl.h"
#include "nas_sw_profile_api.h"
#include "nas_sw_profile.h"
#include "std_utils.h"
#include <map>
#include <string>

static nas_cmn_sw_init_info_t nas_switch_info;
typedef std::map<std::string, std::string> nas_sw_npu_profile_kv_pair_t;
typedef std::map<std::string, std::string>::iterator npu_profile_kv_iter;
static auto npu_profile_kvpair = new nas_sw_npu_profile_kv_pair_t;

static uint32_t nas_std_child_node_count_get(std_config_node_t node)
{
    uint32_t count = 0;
    std_config_node_t chld_node;

    STD_ASSERT(node != NULL);

    for (chld_node = std_config_get_child(node);
         chld_node != NULL;
         chld_node = std_config_next_node(chld_node)) {
        count++;
    }

    return count;
}

static void nas_std_cfg_str_attr_update(std_config_node_t node, const char *attr,
                                 char * dest)
{
    char *node_attr = NULL;

    STD_ASSERT(dest != NULL);
    STD_ASSERT(attr != NULL);

    node_attr = std_config_attr_get(node, attr);
    if(node_attr != NULL) {
       safestrncpy((char*)dest, node_attr, NAS_CMN_PROFILE_NAME_SIZE);
       EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","attr-name %s :%s",attr, dest);
    }
    return;
}
static void nas_std_cfg_attr_update(std_config_node_t node, const char *attr,
                             uint32_t * dest)
{
    char *node_attr = NULL;

    STD_ASSERT(dest != NULL);
    STD_ASSERT(attr != NULL);

    node_attr = std_config_attr_get(node, attr);
    if(node_attr != NULL) {
       *dest = strtoul(node_attr, NULL,0);
       EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","attr-name %s :%d",attr, *dest);
    }
    return;
}

extern "C" {
/* parse switch.xml file and update switch profile */
t_std_error nas_switch_update_profile_info (std_config_node_t node)
{
    uint32_t profile_count, num_profiles;
    std_config_node_t chld_node;

    nas_std_cfg_str_attr_update(node, "default_profile",
                                nas_switch_info.def_profile.name);

    num_profiles = nas_std_child_node_count_get(node);

    if(num_profiles == 0){
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","supported profiles not present config file ");
        return STD_ERR(HALCOM, PARAM,2);
    }

    nas_switch_info.profiles = (nas_cmn_sw_profile_t *)calloc(num_profiles,sizeof(nas_cmn_sw_profile_t));

    if(nas_switch_info.profiles == NULL){
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Failed to alloc memory for profiles");
        return STD_ERR(HALCOM, PARAM,2);
    }

    for (chld_node = std_config_get_child(node), profile_count = 0;
         chld_node != NULL;
         chld_node = std_config_next_node(chld_node), profile_count++) {

        nas_std_cfg_str_attr_update(chld_node, "supported_profile",
                                    nas_switch_info.profiles[profile_count].pname.name);
    }
    /* current prfile will be , default profile */
    safestrncpy(nas_switch_info.current_profile.name,nas_switch_info.def_profile.name,
            sizeof(nas_switch_info.current_profile.name));
    nas_switch_info.num_profiles = profile_count;
    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","num_profiles:%d",profile_count);
    return STD_ERR_OK;
}

extern "C" {
/* parse switch.xml file and update switch NPU profile */
t_std_error nas_switch_update_npu_profile_info (std_config_node_t node)
{
    uint32_t num_profiles;
    std_config_node_t chld_node;
    char *key_attr = NULL;
    char *value_attr = NULL;
    npu_profile_kv_iter kviter;

    num_profiles = nas_std_child_node_count_get(node);

    EV_LOGGING(NAS_COM, DEBUG, "NAS_CMN-SWITCH",
            "Num of Key/Value pairs read from switch config file %d \r\n",
            num_profiles);

    if(num_profiles == 0) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                "NPU Profile's not present in config file ");
        return STD_ERR(HALCOM, PARAM, ENOENT);
    }

    for (chld_node = std_config_get_child(node); chld_node != NULL;
            (chld_node = std_config_next_node(chld_node))) {

         if (((key_attr = std_config_attr_get(chld_node, "id")) == NULL) ||
             ((value_attr = std_config_attr_get(chld_node, "value")) == NULL)) {
             /* Skip if the attibute is not present or one of the variable/value
                is missing */
             continue;
         }
         if (strlen (key_attr) >= NAS_CMN_NPU_PROFILE_ATTR_SIZE ||
                 strlen (value_attr) >= NAS_CMN_NPU_PROFILE_ATTR_SIZE)
         {
             /* Check and skip if the attribute value length is > than
                NAS_CMN_NPU_PROFILE_ATTR_SIZE */
             continue;
         }

         EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
                    "NPU Profile Key %s Value %s", key_attr, value_attr);
         kviter = npu_profile_kvpair->find(key_attr);

         if (kviter == npu_profile_kvpair->end()) {
             npu_profile_kvpair->insert(std::pair<std::string, std::string>(key_attr, value_attr));
         }
         else {
             kviter->second = value_attr;
         }
    }

    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","Number of Profiles : %d", num_profiles);
    return STD_ERR_OK;
    }
}

t_std_error nas_switch_npu_profile_get_next_value(char* variable,
                           char* value)
{
    npu_profile_kv_iter kviter;
    std::string key;

    if (variable == NULL || value == NULL) {
        return STD_ERR(HALCOM, PARAM, ENOENT);
    }

    /*Check if this is query to get the First Key/Value pair */
    if (strlen(variable) == 0) {
        if (npu_profile_kvpair->size() < 1) {
            return STD_ERR(HALCOM, PARAM, ENOENT);
        }
        /* Return the 1st element in the KV Pair Map */
        kviter = npu_profile_kvpair->begin();
    } else {
        key = variable;
        kviter = npu_profile_kvpair->find(key);
        if (kviter == npu_profile_kvpair->end()) {
            return STD_ERR(HALCOM, FAIL,0);
        }
        kviter++;
        if (kviter == npu_profile_kvpair->end()) {
            return STD_ERR(HALCOM, FAIL,0);
        }
    }
    safestrncpy(variable, kviter->first.c_str(), NAS_CMN_NPU_PROFILE_ATTR_SIZE);
    safestrncpy(value, kviter->second.c_str(), NAS_CMN_NPU_PROFILE_ATTR_SIZE);

    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH", "Get next key-value pair key %s, value %s\n",
               variable, value);
    return STD_ERR_OK;
}
/* parse switch.xml file and update UFT modes and info */
t_std_error nas_switch_update_uft_info (std_config_node_t node)
{
    uint32_t uft_mode_count, mode, num_uft_entries;
    std_config_node_t chld_node;

    nas_std_cfg_attr_update(node, "default_mode", &nas_switch_info.def_uftmode);

    num_uft_entries = nas_std_child_node_count_get(node);

    if(num_uft_entries == 0){
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","UFT mode entries not present config file ");
        return STD_ERR(HALCOM, PARAM,2);
    }

    nas_switch_info.uft_info = (nas_cmn_uft_info_t *)calloc(num_uft_entries, sizeof(nas_cmn_uft_info_t));

    if(nas_switch_info.uft_info == NULL){
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Failed to alloc memory for UFT info");
        return STD_ERR(HALCOM, PARAM,2);
    }
    for (chld_node = std_config_get_child(node), uft_mode_count = 0;
         chld_node != NULL;
         chld_node = std_config_next_node(chld_node), uft_mode_count++) {
        nas_std_cfg_attr_update(chld_node, "mode",
                                &nas_switch_info.uft_info[uft_mode_count].mode);
        mode = nas_switch_info.uft_info[uft_mode_count].mode;
        /* Validate modes supported */
        if ((mode < BASE_SWITCH_UFT_MODE_MIN) ||
            (mode > BASE_SWITCH_UFT_MODE_MAX))
        {
            nas_switch_info.uft_info[uft_mode_count].mode = 0;
            EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Unsupported UFT mode %d present"
                        " skip update",mode);
            continue;
        }
        nas_std_cfg_attr_update(chld_node, "l2_max",
                                &nas_switch_info.uft_info[uft_mode_count].l2_table_size);
        nas_std_cfg_attr_update(chld_node, "l3_max",
                                &nas_switch_info.uft_info[uft_mode_count].l3_route_table_size);
        nas_std_cfg_attr_update(chld_node, "arp_max",
                                &nas_switch_info.uft_info[uft_mode_count].l3_host_table_size);
    }
    /* Init default mode as current mode */
    nas_switch_info.current_uftmode = nas_switch_info.def_uftmode;
    nas_switch_info.num_uft_modes = uft_mode_count;
    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","num_uft_modes:%d",uft_mode_count);
    return STD_ERR_OK;
}

/* parse switch.xml file and update ECMP info */
t_std_error nas_switch_update_ecmp_info (std_config_node_t node)
{
    /* Init default value as current value */
    nas_std_cfg_attr_update(node, "default_max_ecmp_per_grp",
                                &nas_switch_info.def_max_ecmp_per_grp);
    nas_switch_info.cur_max_ecmp_per_grp = nas_switch_info.def_max_ecmp_per_grp;
    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","num_ecmp_values:%d",
                                nas_switch_info.cur_max_ecmp_per_grp);
    return STD_ERR_OK;
}

/* parse switch.xml file and update IPv6 Extended prefix info */
t_std_error nas_switch_update_ipv6_extended_prefix_info (std_config_node_t node)
{
    nas_std_cfg_attr_update(node, "config_required",
                                &nas_switch_info.is_ipv6_ext_prefix_cfg_req);

    if (nas_switch_info.is_ipv6_ext_prefix_cfg_req) {

        nas_std_cfg_attr_update(node, "default_routes",
                                &nas_switch_info.def_ipv6_ext_prefix_routes);

        nas_std_cfg_attr_update(node, "max_routes",
                                &nas_switch_info.max_ipv6_ext_prefix_routes);

        nas_std_cfg_attr_update(node, "lpm_block_size",
                                &nas_switch_info.ipv6_ext_prefix_route_blk_size);

        /* Init default value as current value */
        nas_switch_info.cur_ipv6_ext_prefix_routes = nas_switch_info.def_ipv6_ext_prefix_routes;
        nas_switch_info.next_ipv6_ext_prefix_routes = nas_switch_info.cur_ipv6_ext_prefix_routes;
    }

    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","IPv6 Ext prefix Cfg Required:%d",
                                nas_switch_info.is_ipv6_ext_prefix_cfg_req);

    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","IPv6 Extended prefix route, "
                                "Default:%d, Max:%d, Blk size:%d",
                                nas_switch_info.def_ipv6_ext_prefix_routes,
                                nas_switch_info.max_ipv6_ext_prefix_routes,
                                nas_switch_info.ipv6_ext_prefix_route_blk_size);

    return STD_ERR_OK;
}

/* parse switch.xml file and update ACL profile info */
t_std_error nas_switch_update_acl_profile_info (std_config_node_t node)
{
    char     stage[NAS_CMN_NPU_PROFILE_ATTR_SIZE] = { 0 };
    uint32_t stage_type = 0;
    uint32_t max_pool_count = 0;
    uint32_t total_pool_count = 0;
    uint32_t app_group_idx = 0;
    uint32_t default_pool_count = 0;
    uint32_t depth_per_pool = 0;
    uint32_t num_app = 0;
    uint32_t app_idx = 0;

    std_config_node_t chld_node;
    std_config_node_t app_group_info_node;
    std_config_node_t app_info_node;

    for (chld_node = std_config_get_child(node);
         chld_node != NULL;
         chld_node = std_config_next_node(chld_node)) {

        nas_std_cfg_str_attr_update(chld_node, "type", stage);
        nas_std_cfg_attr_update(chld_node, "max_pool_avail", &max_pool_count);

        /* update the stage specific information based on the type string */
        if (strcmp ("ingress", stage) == 0)
            nas_switch_info.acl_profile_max_ingress_pool_count = max_pool_count;
        else if (strcmp ("egress", stage) == 0)
            nas_switch_info.acl_profile_max_egress_pool_count = max_pool_count;
        total_pool_count += max_pool_count;
    }

    nas_switch_info.acl_profile_app_group_info = (nas_cmn_acl_profile_app_group_info_t *)
                        calloc(total_pool_count, sizeof(nas_cmn_acl_profile_app_group_info_t));

    if(nas_switch_info.acl_profile_app_group_info == NULL){
        nas_switch_info.acl_profile_max_ingress_pool_count = 0;
        nas_switch_info.acl_profile_max_egress_pool_count = 0;
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Failed to allocate memory for ACL profile info");
        return STD_ERR(HALCOM, PARAM,2);
    }

    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
               "total pool count:%d, ingress_pool:%d, egress_pool:%d",
               total_pool_count,
               nas_switch_info.acl_profile_max_ingress_pool_count,
               nas_switch_info.acl_profile_max_egress_pool_count);

    for (chld_node = std_config_get_child(node);
         chld_node != NULL;
         chld_node = std_config_next_node(chld_node)) {

        nas_std_cfg_str_attr_update(chld_node, "type", stage);

        if (strcmp ("ingress", stage) == 0)
            stage_type = BASE_ACL_STAGE_INGRESS;
        else if (strcmp ("egress", stage) == 0)
            stage_type = BASE_ACL_STAGE_EGRESS;

        for (app_group_info_node = std_config_get_child(chld_node);
             app_group_info_node != NULL;
             app_group_info_node = std_config_next_node(app_group_info_node), app_group_idx++) {

            nas_switch_info.acl_profile_app_group_info[app_group_idx].stage_type = stage_type;
            nas_std_cfg_str_attr_update(app_group_info_node, "name",
                    nas_switch_info.acl_profile_app_group_info[app_group_idx].app_group_name);

            nas_std_cfg_attr_update(app_group_info_node, "default_pool_count",
                    &default_pool_count);

            nas_switch_info.acl_profile_app_group_info[app_group_idx].default_pool_count = default_pool_count;
            nas_switch_info.acl_profile_app_group_info[app_group_idx].current_pool_count = default_pool_count;
            nas_switch_info.acl_profile_app_group_info[app_group_idx].next_boot_pool_count = default_pool_count;

            nas_std_cfg_attr_update(app_group_info_node, "depth_per_pool", &depth_per_pool);
            nas_switch_info.acl_profile_app_group_info[app_group_idx].depth_per_pool = depth_per_pool;

            num_app = nas_std_child_node_count_get(app_group_info_node);
            nas_switch_info.acl_profile_app_group_info[app_group_idx].num_app = num_app;

            if(num_app == 0){
                EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","ACL profile app group app info is incomplete");
                return STD_ERR(HALCOM, PARAM,2);
            }

            nas_switch_info.acl_profile_app_group_info[app_group_idx].app_info  = (nas_cmn_acl_profile_app_info_t *)
                calloc(num_app, sizeof(nas_cmn_acl_profile_app_info_t));

            if(nas_switch_info.acl_profile_app_group_info[app_group_idx].app_info == NULL) {
                EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Failed to allocate memory for ACL profile app info");
                return STD_ERR(HALCOM, PARAM,2);
            }

            EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
                    "Pool:%s, default_pool_count:%d, depth_per_pool:%d, num_app:%d",
                    nas_switch_info.acl_profile_app_group_info[app_group_idx].app_group_name,
                    nas_switch_info.acl_profile_app_group_info[app_group_idx].default_pool_count,
                    nas_switch_info.acl_profile_app_group_info[app_group_idx].depth_per_pool,
                    nas_switch_info.acl_profile_app_group_info[app_group_idx].num_app);

            for (app_info_node = std_config_get_child(app_group_info_node), app_idx = 0;
                 app_info_node != NULL;
                 app_info_node = std_config_next_node(app_info_node), app_idx++) {

                nas_std_cfg_str_attr_update(app_info_node, "app",
                    nas_switch_info.acl_profile_app_group_info[app_group_idx].app_info[app_idx].app_name);

                nas_std_cfg_attr_update(app_info_node, "num_units_per_entry",
                    &nas_switch_info.acl_profile_app_group_info[app_group_idx].app_info[app_idx].num_units_per_entry);

                EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
                        "App:%s, num_units_per_entry:%d",
                        nas_switch_info.acl_profile_app_group_info[app_group_idx].app_info[app_idx].app_name,
                        nas_switch_info.acl_profile_app_group_info[app_group_idx].app_info[app_idx].num_units_per_entry);
            }
        }
    }

    nas_switch_info.acl_profile_valid = true;

    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH", "switch ACL profile read success");

    return STD_ERR_OK;
}

/* switch profile API's can be used by
    -CPS get/set from nas-l2 and
    -from nas-ndi to read init parametrs
*/

/* nas_sw_profile_startup_cps_db_get - will get the object
   stored in the CPS DB with startup qualifier */
cps_api_object_list_t nas_sw_profile_startup_cps_db_get()
{
    cps_api_object_list_t obj_list = cps_api_object_list_create();
    cps_api_object_guard og(cps_api_obj_tool_create(cps_api_qualifier_STARTUP_CONFIG,
        BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY,false));

    if(obj_list==nullptr || og.get()==nullptr)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH", "Creating list or object failed");
        if(obj_list != nullptr)
        {
            cps_api_object_list_destroy(obj_list, true);
        }
        return nullptr;
    }

    cps_api_object_attr_add_u32(og.get(),
                BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_SWITCH_ID,
                NAS_CMN_DEFAULT_SWITCH_ID);
    if(cps_api_db_get(og.get(),obj_list)==cps_api_ret_code_OK)
    {
        return obj_list;
    }
    EV_LOGGING(NAS_COM, INFO , "NAS-CMN-SWITCH", "No data in startup DB");
    cps_api_object_list_destroy(obj_list, true);
    return nullptr;
}

/* nas_sw_acl_profile_startup_cps_db_get - will get the ACL profile object
   stored in the CPS DB with startup qualifier */
cps_api_object_list_t nas_sw_acl_profile_startup_cps_db_get()
{
    cps_api_object_list_t obj_list = cps_api_object_list_create();
    cps_api_object_guard og(cps_api_obj_tool_create(cps_api_qualifier_STARTUP_CONFIG,
        BASE_ACL_SWITCHING_ENTITY_APP_GROUP,false));

    if(obj_list==nullptr || og.get()==nullptr)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                   "Creating list or object for SW ACL profile entity failed");
        if(obj_list != nullptr)
        {
            cps_api_object_list_destroy(obj_list, true);
        }
        return nullptr;
    }

    if(cps_api_db_get(og.get(),obj_list)==cps_api_ret_code_OK)
    {
        return obj_list;
    }
    EV_LOGGING(NAS_COM, INFO , "NAS-CMN-SWITCH",
               "No SW ACL profile data in startup DB");
    cps_api_object_list_destroy(obj_list, true);
    return nullptr;
}


/* nas_sw_profile_current_profile_get - will get the currently
   active switch profile */
t_std_error nas_sw_profile_current_profile_get(uint32_t sw_id,
                                                char *current_profile,
                                                uint32_t len)
{
    if (current_profile == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM,2);
    }
    if (nas_switch_info.num_profiles == 0)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","profiles not configured - current get");
        return STD_ERR(HALCOM,PARAM,2);
    }

    safestrncpy(current_profile, nas_switch_info.current_profile.name, len);

    return STD_ERR_OK;
}
/* nas_sw_profile_default_profile_get - will get default switch profile */
t_std_error nas_sw_profile_default_profile_get(uint32_t sw_id,
                                                char *default_profile,
                                                int len)
{
    if (default_profile == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input %s", __FUNCTION__);
        return STD_ERR(HALCOM,PARAM,2);
    }
    if (nas_switch_info.num_profiles == 0)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","profiles not configured - default profile get");
        return STD_ERR(HALCOM,PARAM,2);
    }

    safestrncpy(default_profile, nas_switch_info.def_profile.name, len);

    return STD_ERR_OK;
}
/* nas_sw_profile_conf_profile_get - will get the configured switch profile */
t_std_error nas_sw_profile_conf_profile_get(uint32_t sw_id, char *conf_profile,
                                            int len)
{
    if (conf_profile == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM,2);
    }
    if (nas_switch_info.num_profiles == 0)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","profiles not configured - conf get");
        return STD_ERR(HALCOM,PARAM,2);
    }

    /* If no object in DB or corrupted data, return default */
    safestrncpy(conf_profile, nas_switch_info.def_profile.name, len);

    cps_api_object_list_guard obj_list(nas_sw_profile_startup_cps_db_get());
    cps_api_object_list_t list = obj_list.get();
    if(list==nullptr)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","No object in startup DB");
    }
    else
    {
        //Currently handled for only one switch id, get the first object
        cps_api_object_t obj = cps_api_object_list_get(list,0);
        if(obj != nullptr)
        {
            char *next_boot_profile =
                (char *)cps_api_object_get_data(obj,
                BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_SWITCH_PROFILE);
            if(next_boot_profile != NULL)
            {
                safestrncpy(conf_profile, next_boot_profile,
                        strlen(conf_profile)+1);
            }
        }
    }
    return STD_ERR_OK;
}

t_std_error nas_sw_profile_supported_profiles_get(uint32_t sw_id,
                                                    cps_api_object_t cps_obj)
{
    if (nas_switch_info.num_profiles == 0)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","profiles not supported on sw %d"
                        " - supported_profiles_get", sw_id);
        return STD_ERR(HALCOM,PARAM,2);
    }
    for (uint32_t index = 0; index < nas_switch_info.num_profiles;index++)
    {
        cps_api_object_attr_add(cps_obj,
            BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_SUPPORTED_PROFILES,
            nas_switch_info.profiles[index].pname.name,
            sizeof(nas_switch_info.profiles[index].pname.name));
    }
    return STD_ERR_OK;
}
/* nas_sw_profile_supported - verifies the given profile with
    supported profiles, if it matches returns 1, else 0 .*/
bool nas_sw_profile_supported(uint32_t sw_id, char *conf_profile)
{
    if (conf_profile == NULL)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","Invalid input");
        return false;
    }
    if (nas_switch_info.num_profiles == 0)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","profiles not configurable");
        return false;
    }
    for (auto index = 0; index < nas_switch_info.num_profiles ; index++)
    {
        if (strcmp(conf_profile,nas_switch_info.profiles[index].pname.name) == 0)
        {
            EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","profile %s supported",
                        conf_profile);
            return true;
        }
    }
    EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","profile %s not supported",
                conf_profile);
    return false;
}
/* set the switch profile, which can be used on next reboot, if user saves the
   config */
t_std_error nas_sw_profile_conf_profile_set(uint32_t sw_id, char *conf_profile,
                                            cps_api_operation_types_t op_type)
{
    if (op_type == cps_api_oper_DELETE)
    {
        EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","Delete configured profile %s, set to default ",
                    (conf_profile)?conf_profile:"NULL");
        memset(nas_switch_info.next_boot_profile.name, 0,
                sizeof(nas_switch_info.next_boot_profile.name));
        strncpy(nas_switch_info.next_boot_profile.name,
                nas_switch_info.def_profile.name,
                sizeof(nas_switch_info.next_boot_profile.name));
        return STD_ERR_OK;
    }
    if(nas_sw_profile_supported(sw_id, conf_profile) == false)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","profile %s not supported - conf set",
                    (conf_profile)?conf_profile:"NULL");
        return STD_ERR(HALCOM,PARAM,2);
    }
    memset(nas_switch_info.next_boot_profile.name, 0,
            sizeof(nas_switch_info.next_boot_profile.name));
    strncpy(nas_switch_info.next_boot_profile.name, conf_profile,
            sizeof(nas_switch_info.next_boot_profile.name));
    return STD_ERR_OK;
}
/* General API to return number profile supported, this is based on
   number of profiles mentioned in the switch.xml file */
t_std_error nas_sw_profile_num_profiles_get(uint32_t sw_id,
                                            uint32_t *num_profiles)
{
    if (num_profiles == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM,2);
    }
    *num_profiles = nas_switch_info.num_profiles;
    return STD_ERR_OK;
}


/* UFT related get/set and other API's*/
/* nas_sw_uft_mode_supported - verifies the given UFT with
    supported modes, if it matches returns 1, else 0 .*/
bool nas_sw_uft_mode_supported(uint32_t mode)
{
    if (nas_switch_info.num_uft_modes == 0)
    {
        EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","UFT not configurable");
        return false;
    }
    if ((mode < BASE_SWITCH_UFT_MODE_MIN) ||
        (mode > BASE_SWITCH_UFT_MODE_MAX))
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","UFT mode %d out of range",
                    mode);
        return false;
    }
    for (auto index = 0; index < nas_switch_info.num_uft_modes; index++)
    {
        if (nas_switch_info.uft_info[index].mode == mode)
        {
            EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","UFT mode %d supported",
                        mode);
            return true;
        }
    }
    EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","UFT mode %d not supported",
                mode);
    return false;
}

/* nas_sw_profile_current_uft_get - will get current active UFT mode */
t_std_error nas_sw_profile_current_uft_get(uint32_t *current_uft)
{
    if (current_uft == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM,2);
    }
    if (nas_switch_info.num_uft_modes == 0)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","UFT modes not configured - current get");
        return STD_ERR(HALCOM,PARAM,2);
    }
    *current_uft = nas_switch_info.current_uftmode;
    return STD_ERR_OK;
}
/* nas_sw_profile_conf_uft_get - will get configured UFT mode */
t_std_error nas_sw_profile_conf_uft_get(uint32_t *conf_uft)
{
    if (conf_uft == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM,2);
    }
    if (nas_switch_info.num_uft_modes == 0)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","UFT modes not configured - conf get");
        return STD_ERR(HALCOM,PARAM,2);
    }

    /* If no object in DB or corrupted data, return default */
    *conf_uft = nas_switch_info.def_uftmode;

    cps_api_object_list_guard obj_list(nas_sw_profile_startup_cps_db_get());
    cps_api_object_list_t list = obj_list.get();
    if(list==nullptr)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","No object in startup DB");
    }
    else
    {
        //Currently handled for only one switch id, get the first object
        cps_api_object_t obj = cps_api_object_list_get(list,0);
        if(obj != nullptr)
        {
            uint32_t *next_boot_uft =
                (uint32_t *)cps_api_object_get_data(obj,
                BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_UFT_MODE);
            if(next_boot_uft != NULL)
                *conf_uft = *next_boot_uft;

        }
    }
    return STD_ERR_OK;
}
/* nas_sw_profile_conf_uft_set - configures the UFT mode,
   which will take effect on next boot, if its saved   */
t_std_error nas_sw_profile_conf_uft_set(uint32_t conf_uft,
                                        cps_api_operation_types_t op_type)
{
    if (op_type == cps_api_oper_DELETE)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","Delete conf UFT mode, set to default");
        nas_switch_info.next_boot_uftmode = nas_switch_info.def_uftmode;
        return STD_ERR_OK;
    }
    if (nas_sw_uft_mode_supported(conf_uft) == false)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","UFT mode %d not supported - conf set",
                    conf_uft);
        return STD_ERR(HALCOM,PARAM,2);
    }

    nas_switch_info.next_boot_uftmode =  conf_uft;
    return STD_ERR_OK;
}

void inline _nas_sw_fill_uft_info(uint32_t uft_mode,
                            uint32_t *l2_size,
                            uint32_t *l3_size,
                            uint32_t *host_size)
{
    for (auto index = 0; index < nas_switch_info.num_uft_modes; index++)
    {
        if (nas_switch_info.uft_info[index].mode == uft_mode)
        {
            *l2_size = nas_switch_info.uft_info[index].l2_table_size;
            *l3_size = nas_switch_info.uft_info[index].l3_route_table_size;
            *host_size = nas_switch_info.uft_info[index].l3_host_table_size;
        }
    }
    return;
}
/* nas_sw_profile_uft_info_get - for a given valid UFT mode
   this API will return the corresponding l2,l3,host table sizes */
t_std_error nas_sw_profile_uft_info_get(uint32_t uft_mode,
                                        uint32_t *l2_size,
                                        uint32_t *l3_size,
                                        uint32_t *host_size)
{
    if (nas_sw_uft_mode_supported(uft_mode) == false)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","UFT mode %d not supported - info get",
                    uft_mode);
        return STD_ERR(HALCOM,PARAM,2);
    }
    if ((l2_size == NULL) || (l3_size == NULL) || (host_size == NULL))
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM,2);
    }
    _nas_sw_fill_uft_info(uft_mode, l2_size, l3_size, host_size);

    return STD_ERR_OK;
}
/* nas_sw_profile_uft_num_modes_get - will return the number of
   UFT modes supported */
t_std_error nas_sw_profile_uft_num_modes_get(uint32_t *num_uft_modes)
{
    if (nas_switch_info.num_uft_modes == 0)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","UFT modes not supported - num modes get");
        return STD_ERR(HALCOM,PARAM,2);
    }
    if (num_uft_modes == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM,2);
    }
    *num_uft_modes = nas_switch_info.num_uft_modes;
    return STD_ERR_OK;
}
/* nas_sw_profile_uft_default_mode_info_get - will return the default
   UFT mode and corresponding table size values */
t_std_error nas_sw_profile_uft_default_mode_info_get(uint32_t *def_uft_mode,
                                                     uint32_t *l2_size,
                                                     uint32_t *l3_size,
                                                     uint32_t *host_size)
{
    if (nas_switch_info.num_uft_modes == 0)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","UFT modes not supported - def mode get");
        return STD_ERR(HALCOM,PARAM,2);
    }
    if ((def_uft_mode == NULL) || (l2_size == NULL) ||
        (l3_size == NULL) || (host_size == NULL))
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM,2);
    }
    *def_uft_mode = nas_switch_info.def_uftmode;

    _nas_sw_fill_uft_info(*def_uft_mode, l2_size, l3_size, host_size);

    return STD_ERR_OK;
}

/* ECMP related get/set and other API's*/
bool nas_sw_max_ecmp_per_grp_supported(uint32_t max_ecmp_per_grp)
{
    if ((max_ecmp_per_grp < NAS_CMN_MAX_ECMP_ENTRIES_PER_GRP_MIN) ||
        (max_ecmp_per_grp > NAS_CMN_MAX_ECMP_ENTRIES_PER_GRP_MAX))
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","ECMP value  %d out of range",
                    max_ecmp_per_grp);
        return false;
    }
    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","max_ecmp_per_grp %d supported",
                max_ecmp_per_grp);
    return true;
}

/* nas_sw_profile_current_ecmp_get - will get current active
    max_ecmp_per_grp*/
t_std_error nas_sw_profile_cur_max_ecmp_per_grp_get(uint32_t *cur_max_ecmp_per_grp)
{
    if (cur_max_ecmp_per_grp == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM,2);
    }
    *cur_max_ecmp_per_grp = nas_switch_info.cur_max_ecmp_per_grp;
    return STD_ERR_OK;
}
/* nas_sw_profile_conf_ecmp_get - will get configured UFT mode */
t_std_error nas_sw_profile_conf_max_ecmp_per_grp_get(uint32_t *conf_max_ecmp_per_grp)
{
    if (conf_max_ecmp_per_grp == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM,2);
    }

    /* If no object in DB or corrupted data, return default */
    *conf_max_ecmp_per_grp = nas_switch_info.def_max_ecmp_per_grp;

    cps_api_object_list_guard obj_list(nas_sw_profile_startup_cps_db_get());
    cps_api_object_list_t list = obj_list.get();
    if(list==nullptr)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","No object in startup DB");
    }
    else
    {
        //Currently handled for only one switch id, get the first object
        cps_api_object_t obj = cps_api_object_list_get(list,0);
        if(obj != nullptr)
        {
            uint32_t *next_boot_max_ecmp_per_grp =
                (uint32_t *)cps_api_object_get_data(obj,
                BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_MAX_ECMP_ENTRY_PER_GROUP);
            if(next_boot_max_ecmp_per_grp != NULL)
                *conf_max_ecmp_per_grp = *next_boot_max_ecmp_per_grp;

        }
    }
    return STD_ERR_OK;
}
/* nas_sw_profile_conf_ecmp_set - configures max_ecmp_per_grp mode,
   which will take effect on next boot, if its saved   */
t_std_error nas_sw_profile_conf_max_ecmp_per_grp_set(uint32_t conf_max_ecmp_per_grp,
                                        cps_api_operation_types_t op_type)
{
    if (op_type == cps_api_oper_DELETE)
    {
        /* Set next boot with default value on delete */
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","Delete conf max_ecmp_per_grp, set to default");
        nas_switch_info.next_boot_max_ecmp_per_grp = nas_switch_info.def_max_ecmp_per_grp;
        return STD_ERR_OK;
    }
    if (nas_sw_max_ecmp_per_grp_supported(conf_max_ecmp_per_grp) == false)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","max_ecmp_per_grp %d not supported - conf set",
                    conf_max_ecmp_per_grp);
        return STD_ERR(HALCOM,PARAM,2);
    }

    nas_switch_info.next_boot_max_ecmp_per_grp =  conf_max_ecmp_per_grp;
    return STD_ERR_OK;
}

/*** ACL profile/app-group info related get/set API's ***/

/* nas_sw_acl_profile_db_config_supported - verifies that
 * ACL profile DB configuration for the given app group is supported.
 * returns true if supported, else 0.
 */
static bool nas_sw_acl_profile_db_config_supported (const char *app_group_name)
{
    uint32_t app_grp_idx;
    uint32_t num_pools = 0;

    if (!nas_switch_info.acl_profile_valid)
    {
        EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","ACL profile DB not configurable");
        return false;
    }

    for (app_grp_idx = 0;
         app_grp_idx < (nas_switch_info.acl_profile_max_ingress_pool_count +
                        nas_switch_info.acl_profile_max_egress_pool_count);
         app_grp_idx++)
    {
        if ((app_group_name == NULL) ||
            (strcmp(app_group_name, nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name) == 0))
        {
            switch (nas_switch_info.acl_profile_app_group_info[app_grp_idx].stage_type)
            {
                case BASE_ACL_STAGE_INGRESS:
                    num_pools = nas_switch_info.acl_profile_max_ingress_pool_count;
                    break;
                case BASE_ACL_STAGE_EGRESS:
                    num_pools = nas_switch_info.acl_profile_max_egress_pool_count;
                    break;
                default:
                    break;
            }
            /* ACL profile DB config not supported when number of pools for that app-group is not more than 1 */
            if (num_pools <= 1)
                break;

            EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
                    "ACL profile %s DB config supported",
                    ((app_group_name != NULL)?app_group_name:""));
            return true;
        }
    }
    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
               "ACL profile %s DB config not supported",
               ((app_group_name != NULL)?app_group_name:""));
    return false;
}


/* retrieves the ACL profile app group info from startup DB.
 */
static t_std_error nas_sw_acl_profile_startup_db_info_get (const char *app_group_name, uint32_t *p_conf_pool_count)
{
    uint32_t      acl_profile_sw_id = 0;
    uint32_t      next_boot_pool_count = 0;
    const char   *p_app_group = NULL;
    size_t        len;
    size_t        ix = 0;
    bool          app_group_id_found = false;
    bool          next_boot_pool_count_found = false;

    cps_api_object_list_guard obj_list(nas_sw_acl_profile_startup_cps_db_get());
    cps_api_object_list_t list = obj_list.get();
    if(list==nullptr)
    {
        EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","No object in startup DB");
        return STD_ERR (HALCOM,PARAM,2);
    }

    len = cps_api_object_list_size(list);

    for (ix=0; ix < len; ++ix)
    {
        cps_api_object_t cps_obj = cps_api_object_list_get(list, ix);
        cps_api_object_it_t it;
        cps_api_object_it_begin(cps_obj,&it);

        app_group_id_found = false;
        next_boot_pool_count_found = false;

        for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it))
        {
            switch(cps_api_object_attr_id(it.attr))
            {
                case BASE_ACL_SWITCHING_ENTITY_SWITCH_ID:
                    acl_profile_sw_id = cps_api_object_attr_data_u32(it.attr);
                    break;
                case BASE_ACL_SWITCHING_ENTITY_APP_GROUP_ID:
                    p_app_group = (const char *)cps_api_object_attr_data_bin(it.attr);
                    if (strncmp(app_group_name, p_app_group, cps_api_object_attr_len(it.attr)) == 0)
                    {
                        app_group_id_found = true;
                    }
                    break;
                case BASE_ACL_SWITCHING_ENTITY_APP_GROUP_POOL_COUNT:
                    next_boot_pool_count = cps_api_object_attr_data_u32(it.attr);
                    next_boot_pool_count_found = true;
                    break;
                default:
                    break;
            }
        }


        if (app_group_id_found && next_boot_pool_count_found)
        {
            *p_conf_pool_count = next_boot_pool_count;
            EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","switch ACL profile app-group info from DB"
                       "sw_id:%d, app-group:%s, next_boot_pool_count:%d",
                       acl_profile_sw_id, p_app_group, next_boot_pool_count);
            return STD_ERR_OK;
        }
    }
    EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Failed switch ACL profile app-group info get from DB"
            "app-group:%s", app_group_name);

    return STD_ERR (HALCOM,PARAM,2);

}

/* retrieves the ACL profile app group info from DB.
 * if qualifier is observed, it returns from running DB,
 * if qualifier is target, it returns from startup DB.
 */
static cps_api_object_t nas_sw_acl_profile_app_group_info_to_obj (uint32_t app_grp_idx, cps_api_qualifier_t qualifier)
{
    uint32_t conf_pool_count = 0;
    uint32_t app_idx = 0;
    uint32_t default_pool_count;
    uint32_t depth_per_pool;
    uint32_t stage_type;
    uint32_t current_pool_count;

    cps_api_attr_id_t ids[3] = {BASE_ACL_SWITCHING_ENTITY_APP_GROUP_APPLICATION, 0,
        BASE_ACL_SWITCHING_ENTITY_APP_GROUP_APPLICATION_APP_ID};
    const int ids_len = sizeof(ids)/sizeof(ids[0]);

    if (app_grp_idx >= (nas_switch_info.acl_profile_max_ingress_pool_count + nas_switch_info.acl_profile_max_egress_pool_count)) {
        return NULL;
    }

    cps_api_object_t obj = cps_api_object_create();

    if (!obj) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                   "Obj create failed for ACL profile app group info get");
        return NULL;
    }

    if (!cps_api_key_from_attr_with_qual (cps_api_object_key (obj),
                BASE_ACL_SWITCHING_ENTITY_APP_GROUP, qualifier)) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                   "Failed to create Key from Switching entity Object");

        return NULL;
    }

    cps_api_object_attr_add_u32(obj,
            BASE_ACL_SWITCHING_ENTITY_SWITCH_ID,
            NAS_CMN_DEFAULT_SWITCH_ID);

    cps_api_object_attr_add(obj, BASE_ACL_SWITCHING_ENTITY_APP_GROUP_ID,
            nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name,
            strlen (nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name)+1);

    default_pool_count = nas_switch_info.acl_profile_app_group_info[app_grp_idx].default_pool_count;
    depth_per_pool = nas_switch_info.acl_profile_app_group_info[app_grp_idx].depth_per_pool;
    stage_type = nas_switch_info.acl_profile_app_group_info[app_grp_idx].stage_type;
    /* get info from startup DB, if no DB, then return current pool count */
    current_pool_count = nas_switch_info.acl_profile_app_group_info[app_grp_idx].current_pool_count;

    cps_api_object_attr_add_u32(obj,
            BASE_ACL_SWITCHING_ENTITY_APP_GROUP_STAGE, stage_type);

    cps_api_object_attr_add_u32(obj,
            BASE_ACL_SWITCHING_ENTITY_APP_GROUP_DEPTH, depth_per_pool);

    cps_api_object_attr_add_u32(obj,
            BASE_ACL_SWITCHING_ENTITY_APP_GROUP_DEFAULT_POOL_COUNT, default_pool_count);

    if (qualifier == cps_api_qualifier_TARGET) {

        if (nas_sw_acl_profile_startup_db_info_get
                (nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name,
                 &conf_pool_count) == STD_ERR_OK)
        {
            current_pool_count = conf_pool_count;
        }

        cps_api_object_attr_add_u32(obj,
                BASE_ACL_SWITCHING_ENTITY_APP_GROUP_POOL_COUNT,
                current_pool_count);

        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                "switch ACL profile DB get for app group:%s, next boot pool_count:%d",
                nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name,
                current_pool_count);
    } else if (qualifier == cps_api_qualifier_OBSERVED) {
        cps_api_object_attr_add_u32(obj,
                BASE_ACL_SWITCHING_ENTITY_APP_GROUP_POOL_COUNT, current_pool_count);

        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                "switch ACL profile DB get for app group:%s, current pool_count:%d",
                nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name,
                current_pool_count);
    }

    for (app_idx = 0; app_idx < nas_switch_info.acl_profile_app_group_info[app_grp_idx].num_app; app_idx++) {
        ids[1] = app_idx;
        ids[2] = BASE_ACL_SWITCHING_ENTITY_APP_GROUP_APPLICATION_APP_ID;
        cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_info[app_idx].app_name,
                strlen (nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_info[app_idx].app_name)+1);

        ids[1] = app_idx;
        ids[2] = BASE_ACL_SWITCHING_ENTITY_APP_GROUP_APPLICATION_NUM_UNITS_PER_ENTRY;
        cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_U32,
                &nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_info[app_idx].num_units_per_entry,
                sizeof(uint32_t));
    }

    return obj;
}


/* update switch ACL profile cache */
static t_std_error nas_switch_acl_profile_update_cache (const char *app_group_name,
                            uint32_t next_boot_pool_count, bool update_current_count)
{
    uint32_t app_grp_idx;

    if (app_group_name == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                "Invalid app_group_name for ACL profile DB config ");
        return STD_ERR(HALCOM,PARAM,2);
    }

    if (!nas_switch_info.acl_profile_valid)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                "ACL profile DB config for %s not supported",
                app_group_name);
        return STD_ERR(HALCOM,PARAM,2);
    }

    for (app_grp_idx = 0;
         app_grp_idx < (nas_switch_info.acl_profile_max_ingress_pool_count +
                        nas_switch_info.acl_profile_max_egress_pool_count);
         app_grp_idx++)
    {
        if (strcmp(app_group_name, nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name) == 0)
        {
            nas_switch_info.acl_profile_app_group_info[app_grp_idx].next_boot_pool_count = next_boot_pool_count;

            if (update_current_count)
                nas_switch_info.acl_profile_app_group_info[app_grp_idx].current_pool_count = next_boot_pool_count;

            EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
                    "switch ACL profile update for app group:%s, next_boot_pool_count:%d",
                    app_group_name, next_boot_pool_count);

            return STD_ERR_OK;
        }
    }
    return STD_ERR(HALCOM,PARAM,2);
}

/* reset switch ACL profile app group info next boot pool count to default value */
static t_std_error nas_switch_acl_profile_reset_cache_to_default (const char *app_group_name)
{
    uint32_t app_grp_idx;

    if (app_group_name == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                "Invalid app_group_name for ACL profile DB config ");
        return STD_ERR(HALCOM,PARAM,2);
    }

    for (app_grp_idx = 0;
         app_grp_idx < (nas_switch_info.acl_profile_max_ingress_pool_count +
                        nas_switch_info.acl_profile_max_egress_pool_count);
         app_grp_idx++)
    {
        if (strcmp(app_group_name, nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name) == 0)
        {
            nas_switch_info.acl_profile_app_group_info[app_grp_idx].next_boot_pool_count =
                nas_switch_info.acl_profile_app_group_info[app_grp_idx].default_pool_count;

            EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
                    "switch ACL profile reset to default pool count for app group:%s",
                    app_group_name);

            return STD_ERR_OK;
        }
    }

    return STD_ERR(HALCOM,PARAM,2);
}


/* General API to return number of switch ACL profile pool's supported, this is based on
   number of pools mentioned in the switch.xml file */
t_std_error nas_sw_acl_profile_num_db_get(uint32_t *num_acl_db)
{
    if (num_acl_db == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                   "Invalid input for ACL profile info get");
        return STD_ERR(HALCOM,PARAM,2);
    }

    *num_acl_db = 0;
    if (!nas_switch_info.acl_profile_valid)
    {
        EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
                "No information on Switch ACL profile in DB");
        return STD_ERR_OK;
    }

    *num_acl_db = nas_switch_info.acl_profile_max_ingress_pool_count +
                  nas_switch_info.acl_profile_max_egress_pool_count;

    return STD_ERR_OK;
}


/* General API to return the switch ACL profile app group info */
t_std_error nas_sw_acl_profile_db_get_next(const char *app_group_name,
                                           char *app_group_name_next, uint32_t *num_pools_req)
{
    uint32_t app_grp_idx;

    if (app_group_name_next == NULL || num_pools_req == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                   "Invalid input for ACL profile DB get");
        return STD_ERR(HALCOM,PARAM,2);
    }
    for (app_grp_idx = 0;
         app_grp_idx < (nas_switch_info.acl_profile_max_ingress_pool_count +
                        nas_switch_info.acl_profile_max_egress_pool_count);
         app_grp_idx++)
    {
        if ((app_group_name == NULL) ||
            (strcmp (app_group_name, nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name) == 0))
        {
            if (app_group_name != NULL)
            {
                app_grp_idx++;
                if (app_grp_idx >= (nas_switch_info.acl_profile_max_ingress_pool_count + nas_switch_info.acl_profile_max_egress_pool_count))
                {
                    /* no more entries */
                    return STD_ERR(HALCOM,PARAM,2);
                }
            }
            safestrncpy (app_group_name_next,
                    nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name,
                    NAS_CMN_PROFILE_NAME_SIZE);
            *num_pools_req = nas_switch_info.acl_profile_app_group_info[app_grp_idx].current_pool_count;
            return STD_ERR_OK;
        }
    }

    /* no more entries */
    return STD_ERR(HALCOM,PARAM,2);
}


/* nas_sw_acl_profile_info_get -
   this API will return the switch ACL profile information in object format.
 */
t_std_error nas_sw_acl_profile_info_get (cps_api_object_t obj)
{
    int app_grp_idx;

    if (nas_sw_acl_profile_db_config_supported (NULL) == false)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","switch ACL profile not supported");
        return STD_ERR(HALCOM,PARAM,2);
    }

    cps_api_attr_id_t ids[3] = {BASE_ACL_SWITCHING_ENTITY_APP_GROUP, 0,
        BASE_ACL_SWITCHING_ENTITY_APP_GROUP_ID};
    const int ids_len = sizeof(ids)/sizeof(ids[0]);

    uint32_t acl_profile_max_ingress_pool_count = nas_switch_info.acl_profile_max_ingress_pool_count;
    uint32_t acl_profile_max_egress_pool_count = nas_switch_info.acl_profile_max_egress_pool_count;

    cps_api_object_attr_add_u32(obj,
            BASE_ACL_SWITCHING_ENTITY_MAX_INGRESS_ACL_POOL_COUNT,
            acl_profile_max_ingress_pool_count);
    cps_api_object_attr_add_u32(obj,
            BASE_ACL_SWITCHING_ENTITY_MAX_EGRESS_ACL_POOL_COUNT,
            acl_profile_max_egress_pool_count);

    for (app_grp_idx = 0;
         app_grp_idx < (nas_switch_info.acl_profile_max_ingress_pool_count +
                        nas_switch_info.acl_profile_max_egress_pool_count);
         app_grp_idx++)
    {
        ids[1] = app_grp_idx;
        ids[2] = BASE_ACL_SWITCHING_ENTITY_APP_GROUP_ID;
        cps_api_object_e_add(obj,ids,ids_len,cps_api_object_ATTR_T_BIN,
                nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name,
                strlen (nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name)+1);
    }

    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
            "switch ACL profile info get success ");
    return STD_ERR_OK;
}

/* nas_sw_acl_profile_app_group_info_get -
 * this API will return the ACL profile app group information.
 * if is_get_all is true, then it returns all app group info.
 * else, it returns the app group info for the given app group id.
 * if qualifier is observed,
 *     it returns the current pool count assigned for the app-group.
 * if qualifier is target,
 *     it returns the last saved next boot pool count assigned for the app-group.
 */
t_std_error nas_sw_acl_profile_app_group_info_get (const char *app_group_name, bool is_get_all,
                                                   cps_api_object_list_t list, cps_api_qualifier_t qualifier)
{
    uint32_t app_grp_idx;

    if ((is_get_all == false) && (app_group_name == NULL))
        return STD_ERR(HALCOM,PARAM,2);

    for (app_grp_idx = 0;
         app_grp_idx < (nas_switch_info.acl_profile_max_ingress_pool_count +
                        nas_switch_info.acl_profile_max_egress_pool_count);
         app_grp_idx++) {

        if ((is_get_all) ||
            (strcmp(app_group_name, nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name) == 0))
        {
            cps_api_object_list_t obj = nas_sw_acl_profile_app_group_info_to_obj (app_grp_idx, qualifier);

            if ((obj != NULL) &&
                (!cps_api_object_list_append(list, obj)))
            {
                cps_api_object_delete(obj);

                EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                        "switch ACL profile get failed during list append for app group:%s",
                        nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name);
                return STD_ERR(HALCOM,PARAM,2);
            }

            EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH",
                    "switch ACL profile get for app group:%s success",
                    nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name);
            if (!is_get_all)
                return STD_ERR_OK;
        }
    }

    return STD_ERR_OK;
}

/* nas_sw_acl_profile_app_group_info_set - configures ACL DB info,
 * configures ACL profile app group info which will take effect on next boot, if saved.
 */
t_std_error nas_sw_acl_profile_app_group_info_set (const char *app_group_name, uint32_t next_boot_pool_req,
                                                   cps_api_operation_types_t op_type)
{
    if (nas_sw_acl_profile_db_config_supported (app_group_name) == false)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH",
                   "ACL profile DB config for %s not supported",
                   app_group_name);
        return STD_ERR(HALCOM,PARAM,2);
    }

    if (op_type == cps_api_oper_DELETE)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH",
                   "Delete ACL profile DB conf for %s, set to default",
                   app_group_name);
        return (nas_switch_acl_profile_reset_cache_to_default(app_group_name));
    }
    return (nas_switch_acl_profile_update_cache (app_group_name, next_boot_pool_req, false));
}


/****** CPS DB  Get, Set for Switching ACL profile entity *******/

/* updates sw ACL profile info to running config */
t_std_error nas_switch_upd_acl_profile_info_to_running_cps_db (uint32_t sw_id)
{
    uint32_t app_grp_idx;
    uint32_t next_boot_pool_count;
    t_std_error rc = STD_ERR_OK;
    char     buff[1024];

    for (app_grp_idx = 0;
         app_grp_idx < (nas_switch_info.acl_profile_max_ingress_pool_count +
                        nas_switch_info.acl_profile_max_egress_pool_count);
         app_grp_idx++)
    {
        /* In some cases there will be stale values in DB with RUNNING qualifier,
         * so overrite them by writing the complete DB.
         */
        cps_api_object_t db_obj = cps_api_object_init(buff, sizeof(buff));

        cps_api_key_from_attr_with_qual(cps_api_object_key(db_obj),
                BASE_ACL_SWITCHING_ENTITY_APP_GROUP,
                cps_api_qualifier_RUNNING_CONFIG);

        cps_api_object_set_type_operation(cps_api_object_key(db_obj),cps_api_oper_SET);

        /* after updating local, update CPS DB for these objects with RUNNING qualifiers.
           in some scenarions where startup and running wil be different, if startup.xml gets
           deleted. So after reading from DB(in both cases if Obj present in DB or not) update
           current value to DB as RUNNING */

        cps_api_object_attr_add_u32 (db_obj, BASE_ACL_SWITCHING_ENTITY_SWITCH_ID, sw_id);

        cps_api_object_attr_add (db_obj, BASE_ACL_SWITCHING_ENTITY_APP_GROUP_ID,
                nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name,
                strlen (nas_switch_info.acl_profile_app_group_info[app_grp_idx].app_group_name)+1);

        next_boot_pool_count = nas_switch_info.acl_profile_app_group_info[app_grp_idx].next_boot_pool_count;

        cps_api_object_attr_add_u32(db_obj,BASE_ACL_SWITCHING_ENTITY_APP_GROUP_POOL_COUNT, next_boot_pool_count);

        cps_api_return_code_t ret =  cps_api_db_commit_one(cps_api_oper_SET, db_obj, NULL, false);

        if (ret != cps_api_ret_code_OK)
        {
            EV_LOGGING(NAS_L2, ERR, "SWITCH",
                    "Failed to clear switch ACL profile elements from DB");
            rc = STD_ERR(HALCOM,FAIL,2);
            break;
        }
    }

    return rc;
}

/* parse switch ACL profile info from startup DB and update running config */
t_std_error nas_sw_acl_profile_parse_db_and_update(uint32_t sw_id)
{
    uint32_t      acl_profile_sw_id = 0;
    uint32_t      next_boot_pool_count = 0;
    const char   *app_group_name = NULL;
    size_t        len = 0;
    size_t        ix = 0;
    bool          acl_profile_found = false;
    bool          app_group_id_found = false;
    bool          next_boot_pool_count_found = false;

    cps_api_object_list_t list;

    if (!nas_switch_info.acl_profile_valid)
    {
        EV_LOGGING(NAS_COM, DEBUG, "SWITCH-INIT",
                "No information on Switch ACL profile in DB");
        return STD_ERR_OK;
    }
    list = nas_sw_acl_profile_startup_cps_db_get();

    if (list != nullptr)
    {
        len = cps_api_object_list_size(list);
        acl_profile_found = true;
    }

    /* parse and update local database */
    for (ix=0; ix < len; ++ix)
    {
        cps_api_object_t cps_obj = cps_api_object_list_get(list, ix);
        cps_api_object_it_t it;
        cps_api_object_it_begin(cps_obj,&it);

        for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it))
        {
            switch(cps_api_object_attr_id(it.attr))
            {
                case BASE_ACL_SWITCHING_ENTITY_SWITCH_ID:
                    acl_profile_sw_id = cps_api_object_attr_data_u32(it.attr);
                    if (acl_profile_sw_id != sw_id)
                    {
                        EV_LOGGING(NAS_COM, ERR, "SWITCH-INIT",
                                   "Mismatch. switch ACL profile DB APP group sw_id(%d) is not same as input sw_id(%d)",
                                   acl_profile_sw_id, sw_id);
                    }
                break;
                case BASE_ACL_SWITCHING_ENTITY_APP_GROUP_ID:
                    app_group_name = (const char *)cps_api_object_attr_data_bin(it.attr);
                    app_group_id_found = true;
                    break;
                case BASE_ACL_SWITCHING_ENTITY_APP_GROUP_POOL_COUNT:
                    next_boot_pool_count = cps_api_object_attr_data_u32(it.attr);
                    next_boot_pool_count_found = true;
                    break;
                default:
                    break;
            }
        }

        if (!app_group_id_found || !next_boot_pool_count_found)
        {
            EV_LOGGING(NAS_COM, ERR, "SWITCH-INIT",
                       "switch ACL profile info from DB is incomplete. "
                       "Missing app group id or next boot pool count app_found:%d, pool_count_found:%d",
                       app_group_id_found, next_boot_pool_count_found);
        }
        else
        {
            EV_LOGGING(NAS_COM, DEBUG, "SWITCH-INIT","switch ACL profile app-group info from DB"
                       "sw_id:%d, app-group:%s, next_boot_pool_count:%d",
                       acl_profile_sw_id, app_group_name, next_boot_pool_count);
            nas_switch_acl_profile_update_cache (app_group_name, next_boot_pool_count, true);
        }
        app_group_id_found = false;
        next_boot_pool_count_found = false;
    }

    if (list != nullptr)
    {
        cps_api_object_list_destroy(list, true);
    }

    if(acl_profile_found == false)
    {
        EV_LOGGING(NAS_COM, NOTICE, "SWITCH-INIT",
                   "switch ACL profile info not present in startup DB,"
                   "set default ACL profile info");
    }
    nas_switch_upd_acl_profile_info_to_running_cps_db (sw_id);

    return STD_ERR_OK;
}

/****** CPS DB  Get, Set for Switching entity *******/
t_std_error nas_sw_parse_db_and_update(uint32_t sw_id)
{
    bool profile_found, uft_found, ecmp_found, ipv6_ext_prefix_found;
    char *conf_profile = NULL;

    profile_found = uft_found = ecmp_found = false;
    ipv6_ext_prefix_found = false;

    cps_api_object_list_t list = nas_sw_profile_startup_cps_db_get();

    size_t len = 0;
    size_t ix = 0;

    if (list != nullptr) {
        len = cps_api_object_list_size(list);
    }

    memset(nas_switch_info.current_profile.name, 0,
            sizeof(nas_switch_info.current_profile.name));
    memset(nas_switch_info.next_boot_profile.name, 0,
            sizeof(nas_switch_info.next_boot_profile.name));

    /* parse and update local database */
    for (ix=0; ix < len; ++ix)
    {
        cps_api_object_t cps_obj = cps_api_object_list_get(list, ix);
        cps_api_object_it_t it;
        cps_api_object_it_begin(cps_obj,&it);

        for ( ; cps_api_object_it_valid(&it) ; cps_api_object_it_next(&it) )
        {
            int id = (int) cps_api_object_attr_id(it.attr);
            switch (id)
            {
                case BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_SWITCH_PROFILE:
                {
                    conf_profile = (char *)cps_api_object_attr_data_bin(it.attr);
                    strncpy(nas_switch_info.next_boot_profile.name, conf_profile,
                            sizeof(nas_switch_info.next_boot_profile.name));
                    EV_LOGGING(NAS_COM, NOTICE, "SWITCH-INIT","switch profile from DB %s",
                                conf_profile);
                    profile_found = true;
                }
                break;
                case BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_UFT_MODE:
                {
                    nas_switch_info.next_boot_uftmode = cps_api_object_attr_data_u32(it.attr);
                    EV_LOGGING(NAS_COM, NOTICE, "SWITCH-INIT","switch UFT mode from DB %d",
                                nas_switch_info.next_boot_uftmode);
                    uft_found = true;
                }
                break;
                case BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_MAX_ECMP_ENTRY_PER_GROUP:
                {
                    nas_switch_info.next_boot_max_ecmp_per_grp = cps_api_object_attr_data_u32(it.attr);
                    EV_LOGGING(NAS_COM, NOTICE, "SWITCH-INIT","switch max_ecmp_per_grp from DB %d",
                                                nas_switch_info.next_boot_max_ecmp_per_grp);
                    ecmp_found = true;
                }
                break;
                case BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_IPV6_EXTENDED_PREFIX_ROUTES:
                {
                    nas_switch_info.next_ipv6_ext_prefix_routes = cps_api_object_attr_data_u32(it.attr);
                    EV_LOGGING(NAS_COM, NOTICE, "SWITCH-INIT","switch ipv6 extended prefix routes from DB %d",
                                                nas_switch_info.next_ipv6_ext_prefix_routes);
                    ipv6_ext_prefix_found = true;
                }
                break;
                default:
                break;
            }
        }
    }

    if (list != nullptr) {
        cps_api_object_list_destroy(list, true);
    }

    /* if vlaues are not stored in DB update next boot with def values */
    if (profile_found == false)
    {
        /* copy default profile name */
        strncpy(nas_switch_info.next_boot_profile.name,
                nas_switch_info.def_profile.name,
                sizeof(nas_switch_info.next_boot_profile.name));
        EV_LOGGING(NAS_COM, NOTICE, "SWITCH-INIT","switch profile not present in DB,"
                        "set default %s", nas_switch_info.next_boot_profile.name);
    }
    if(uft_found == false)
    {
        nas_switch_info.next_boot_uftmode =  nas_switch_info.def_uftmode;
        EV_LOGGING(NAS_COM, NOTICE, "SWITCH-INIT","switch UFT mode not present in DB,"
                        "set default %d", nas_switch_info.next_boot_uftmode);
    }
    if (ecmp_found == false)
    {
        nas_switch_info.next_boot_max_ecmp_per_grp =  nas_switch_info.def_max_ecmp_per_grp;
        EV_LOGGING(NAS_COM, NOTICE, "SWITCH-INIT","switch max_ecmp_per_grp not present in DB,"
                        "set default %d", nas_switch_info.next_boot_max_ecmp_per_grp);
    }
    if (ipv6_ext_prefix_found == false)
    {
        nas_switch_info.next_ipv6_ext_prefix_routes =  nas_switch_info.def_ipv6_ext_prefix_routes;
        EV_LOGGING(NAS_COM, NOTICE, "SWITCH-INIT","switch ipv6_ext_prefix_routes not present in DB,"
                        "set default %d", nas_switch_info.next_ipv6_ext_prefix_routes);
    }

    /* Update current values, same as next boot values on the boot */
    strncpy(nas_switch_info.current_profile.name,
            nas_switch_info.next_boot_profile.name,
            sizeof(nas_switch_info.current_profile.name));
    nas_switch_info.current_uftmode = nas_switch_info.next_boot_uftmode;
    nas_switch_info.cur_max_ecmp_per_grp = nas_switch_info.next_boot_max_ecmp_per_grp;
    nas_switch_info.cur_ipv6_ext_prefix_routes = nas_switch_info.next_ipv6_ext_prefix_routes;

    /* In some cases there will be stale values in DB with RUNNING qualifier,
       clear them*/
    cps_api_object_t db_obj = cps_api_object_create();

    if (db_obj == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "SWITCH","DB running clear - failed to create CPS obj");
        return STD_ERR(HALCOM,FAIL,2);
    }

    cps_api_key_from_attr_with_qual(cps_api_object_key(db_obj),
                                BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY,
                                cps_api_qualifier_RUNNING_CONFIG);
    cps_api_object_set_type_operation(cps_api_object_key(db_obj),cps_api_oper_SET);

    /* after updating local, update CPS DB for these objects with RUNNING qualifiers.
       in some scenarions where startup and running wil be different, if startup.xml gets
       deleted. So after reading from DB(in both cases if Obj present in DB or not) update
       current value to DB as RUNNING */
    cps_api_set_key_data (db_obj, BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_SWITCH_ID,
                          cps_api_object_ATTR_T_U32,&sw_id, sizeof(sw_id));
    if (nas_sw_profile_supported(sw_id, nas_switch_info.current_profile.name))
    {
       cps_api_object_attr_add(db_obj, BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_SWITCH_PROFILE,
                                nas_switch_info.current_profile.name,
                                sizeof(nas_switch_info.current_profile.name));
    }
    cps_api_object_attr_add_u32(db_obj, BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_UFT_MODE,
                                nas_switch_info.current_uftmode);
    cps_api_object_attr_add_u32(db_obj,
                                BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_MAX_ECMP_ENTRY_PER_GROUP,
                                nas_switch_info.cur_max_ecmp_per_grp);
   cps_api_object_attr_add_u32(db_obj,
                                BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_IPV6_EXTENDED_PREFIX_ROUTES,
                                nas_switch_info.cur_ipv6_ext_prefix_routes);

    cps_api_return_code_t ret =  cps_api_db_commit_one(cps_api_oper_SET, db_obj, NULL, false);
    if (ret != cps_api_ret_code_OK)
    {
        EV_LOGGING(NAS_COM, ERR, "SWITCH","Failed to clear switch elements from DB");
        cps_api_object_delete(db_obj);
        return STD_ERR(HALCOM,FAIL,2);
    }

    cps_api_object_delete(db_obj);

    /* parse sw ACL profile info from DB and update cache */
    t_std_error rc = nas_sw_acl_profile_parse_db_and_update(sw_id);

    if (rc != STD_ERR_OK)
    {
        EV_LOGGING(NAS_COM, ERR, "SWITCH","Failed to clear switch ACL profile elements from DB");
        return STD_ERR(HALCOM,FAIL,2);
    }

    return STD_ERR_OK;
}


/* nas_sw_profile_cur_ipv6_ext_prefix_routes_get - will get current confiured ipv6
    extended prefix routes */
bool nas_sw_profile_is_ipv6_ext_prefix_config_required(void)
{
    return (nas_switch_info.is_ipv6_ext_prefix_cfg_req ? true : false);
}


/* nas_sw_profile_cur_ipv6_ext_prefix_routes_get - will get current confiured ipv6
    extended prefix routes */
t_std_error nas_sw_profile_cur_ipv6_ext_prefix_routes_get(uint32_t *cur_ipv6_ext_prefix_routes)
{
    if (nas_sw_profile_is_ipv6_ext_prefix_config_required() == false) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","To Support IPv6 Extended prefix routes,"
                   "config is not required");
        /* e_std_err_code_NOTSUPPORTED Will be changed to e_std_err_code_NOTAPPLICABLE,
         * After adding new error code*/
        return STD_ERR(HALCOM, PARAM, e_std_err_code_NOTSUPPORTED);
    }

    if (cur_ipv6_ext_prefix_routes == NULL) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM, e_std_err_code_PARAM);
    }
    *cur_ipv6_ext_prefix_routes = nas_switch_info.cur_ipv6_ext_prefix_routes;
    return STD_ERR_OK;
}

/* nas_sw_profile_max_ipv6_ext_prefix_routes_get - will get max supportedipv6
    extended prefix routes */
t_std_error nas_sw_profile_max_ipv6_ext_prefix_routes_get(uint32_t *max_ipv6_ext_prefix_routes)
{
    if (nas_sw_profile_is_ipv6_ext_prefix_config_required() == false) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","To Support IPv6 Extended prefix routes,"
                   "config is not required");
        /* e_std_err_code_NOTSUPPORTED Will be changed to e_std_err_code_NOTAPPLICABLE,
         * After adding new error code*/
        return STD_ERR(HALCOM, PARAM, e_std_err_code_NOTSUPPORTED);
    }

    if (max_ipv6_ext_prefix_routes == NULL) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM, e_std_err_code_PARAM);
    }
    *max_ipv6_ext_prefix_routes = nas_switch_info.max_ipv6_ext_prefix_routes;
    return STD_ERR_OK;
}

/* nas_sw_profile_ipv6_ext_prefix_route_lpm_blk_size_get - will get max supported ipv6
    extended prefix route lpm block size */
t_std_error nas_sw_profile_ipv6_ext_prefix_route_lpm_blk_size_get(uint32_t *ipv6_ext_prefix_route_blk_size)
{
    if (nas_sw_profile_is_ipv6_ext_prefix_config_required() == false) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","To Support IPv6 Extended prefix routes,"
                   "config is not required");
        /* e_std_err_code_NOTSUPPORTED Will be changed to e_std_err_code_NOTAPPLICABLE,
         * After adding new error code*/
        return STD_ERR(HALCOM, PARAM, e_std_err_code_NOTSUPPORTED);
    }

    if (ipv6_ext_prefix_route_blk_size == NULL) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM, e_std_err_code_PARAM);
    }
    *ipv6_ext_prefix_route_blk_size = nas_switch_info.ipv6_ext_prefix_route_blk_size;
    return STD_ERR_OK;
}

/* nas_sw_profile_conf_ipv6_ext_prefix_routes_get - will get configured ipv6
 * extended prefix routes size */
t_std_error nas_sw_profile_conf_ipv6_ext_prefix_routes_get(uint32_t *conf_ipv6_ext_prefix_routes)
{
    if (conf_ipv6_ext_prefix_routes == NULL)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","Invalid input");
        return STD_ERR(HALCOM,PARAM, e_std_err_code_PARAM);
    }

    if (nas_sw_profile_is_ipv6_ext_prefix_config_required() == false) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","To Support IPv6 Extended prefix routes,"
                   "config is not required");
        /* e_std_err_code_NOTSUPPORTED Will be changed to e_std_err_code_NOTAPPLICABLE,
         * After adding new error code*/
        return STD_ERR(HALCOM, PARAM, e_std_err_code_NOTSUPPORTED);
    }

    /* If no object in DB or corrupted data, return default */
    *conf_ipv6_ext_prefix_routes = nas_switch_info.def_ipv6_ext_prefix_routes;

    cps_api_object_list_guard obj_list(nas_sw_profile_startup_cps_db_get());
    cps_api_object_list_t list = obj_list.get();
    if(list==nullptr)
    {
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","No object in startup DB");
    }
    else
    {
        //Currently handled for only one switch id, get the first object
        cps_api_object_t obj = cps_api_object_list_get(list,0);
        if(obj != nullptr)
        {
            uint32_t *next_ipv6_ext_prefix_routes =
                (uint32_t *)cps_api_object_get_data(obj,
                BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_IPV6_EXTENDED_PREFIX_ROUTES);
            if(next_ipv6_ext_prefix_routes != NULL)
                *conf_ipv6_ext_prefix_routes = *next_ipv6_ext_prefix_routes;

        }
    }
    return STD_ERR_OK;
}

/* ECMP related get/set and other API's*/
t_std_error nas_sw_profile_validate_ipv6_ext_prefix_routes_size (uint32_t ipv6_ext_prefix_routes)
{
    if (ipv6_ext_prefix_routes > nas_switch_info.max_ipv6_ext_prefix_routes)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","IPv6 Extended prefix routes %d out of range",
                   ipv6_ext_prefix_routes);
        return STD_ERR(HALCOM,PARAM,e_std_err_code_PARAM);
    }

    if ((ipv6_ext_prefix_routes % nas_switch_info.ipv6_ext_prefix_route_blk_size) != 0)
    {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","IPv6 Extended prefix routes "
                  "value should be multiples of %d",
                  nas_switch_info.ipv6_ext_prefix_route_blk_size);
        return STD_ERR(HALCOM,PARAM,e_std_err_code_PARAM);
    }


    EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","ipv6_ext_prefix_routes size %d",
              ipv6_ext_prefix_routes);

    return STD_ERR_OK;
}


/* nas_sw_profile_conf_ipv6_ext_prefix_routes_set - configures ipv6 ext prefix routes,
   which will take effect on next boot, if its saved   */
t_std_error nas_sw_profile_conf_ipv6_ext_prefix_routes_set(uint32_t conf_ipv6_ext_prefix_routes,
                                        cps_api_operation_types_t op_type)
{
    t_std_error rc = STD_ERR_OK;

    if (nas_sw_profile_is_ipv6_ext_prefix_config_required() == false) {
        EV_LOGGING(NAS_COM, ERR, "NAS-CMN-SWITCH","To Support IPv6 Extended prefix routes,"
                   "config is not required");
        /* e_std_err_code_NOTSUPPORTED Will be changed to e_std_err_code_NOTAPPLICABLE,
         * After adding new error code*/
        return STD_ERR(HALCOM, PARAM, e_std_err_code_NOTSUPPORTED);
    }

    if (op_type == cps_api_oper_DELETE)
    {
        /* Set next boot with default value on delete */
        EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","Delete conf ipv6 ext prefix routes, set to default");
        nas_switch_info.next_ipv6_ext_prefix_routes = nas_switch_info.def_ipv6_ext_prefix_routes;
        return STD_ERR_OK;
    }

    EV_LOGGING(NAS_COM, INFO, "NAS-CMN-SWITCH","IPv6 Ext Prefix routes size %d",
               conf_ipv6_ext_prefix_routes);

    rc = nas_sw_profile_validate_ipv6_ext_prefix_routes_size(conf_ipv6_ext_prefix_routes);
    if (rc != STD_ERR_OK) {
        return rc;
    }

    nas_switch_info.next_ipv6_ext_prefix_routes =  conf_ipv6_ext_prefix_routes;
    return STD_ERR_OK;
}

} /* extern C */
