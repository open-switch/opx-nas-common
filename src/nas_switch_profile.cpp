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
#include "nas_sw_profile_api.h"
#include "nas_sw_profile.h"
#include "std_utils.h"

static nas_cmn_sw_init_info_t nas_switch_info;

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
        EV_LOGGING(NAS_COM, DEBUG, "NAS-CMN-SWITCH","Delete configured profile, set to default ",
                    conf_profile);
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
/* General API to retun number profile supported, this is based on
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
/* nas_sw_profile_uft_num_modes_get - will retun the number of
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
/* nas_sw_profile_uft_default_mode_info_get - will retun the default
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

/****** CPS DB  Get, Set *******/

t_std_error nas_sw_parse_db_and_update(uint32_t sw_id)
{
    bool profile_found, uft_found, ecmp_found;
    char *conf_profile = NULL;

    profile_found = uft_found = ecmp_found = false;

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

    /* Update current values, same as next boot values on the boot */
    strncpy(nas_switch_info.current_profile.name,
            nas_switch_info.next_boot_profile.name,
            sizeof(nas_switch_info.current_profile.name));
    nas_switch_info.current_uftmode = nas_switch_info.next_boot_uftmode;
    nas_switch_info.cur_max_ecmp_per_grp = nas_switch_info.next_boot_max_ecmp_per_grp;

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

    cps_api_return_code_t ret =  cps_api_db_commit_one(cps_api_oper_SET, db_obj, NULL, false);
    if (ret != cps_api_ret_code_OK)
    {
        EV_LOGGING(NAS_L2, ERR, "SWITCH","Failed to clear switch elements from DB");
        cps_api_object_delete(db_obj);
        return STD_ERR(HALCOM,FAIL,2);
    }

    cps_api_object_delete(db_obj);

    return STD_ERR_OK;
}

} /* extern C */
