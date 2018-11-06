/*
 * Copyright (c) 2016 Dell Inc.
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
 * nas_switch.c
 *
 *  Created on: Mar 31, 2015
 */


#include "nas_switch.h"
#include "std_assert.h"
#include "std_config_node.h"
#include "std_envvar.h"
#include "std_utils.h"
#include "event_log.h"
#include "cps_api_operation.h"
#include "nas_sw_profile_api.h"
#include "nas_sw_profile.h"

#include <string.h>
#include <stdio.h>

static const char *switch_cfg_path = NULL;
static bool failed = false;

static nas_switches_t switches;
static size_t num_switches = 0;
static bool nas_sw_fc_supported = 0;

static nas_switch_detail_t *switch_cfg;

bool nas_switch_get_fc_supported(void) {
    return nas_sw_fc_supported;
}

size_t nas_switch_get_max_npus(void) {
    return num_switches;
}

uint32_t * int_array_from_string(const char * str, size_t *expected_len) {
    std_parsed_string_t ps = NULL;
    if (!std_parse_string(&ps,str,",")) {
        return NULL;
    }
    uint32_t * lst = NULL;
    do {
        *expected_len = std_parse_string_num_tokens(ps);
        lst = (uint32_t*)calloc(*expected_len,sizeof(*lst));
        if (lst==NULL) break;
        size_t ix = 0;
        for ( ; ix < *expected_len ; ++ix ) {
            lst[ix] = atoi(std_parse_string_at(ps,ix));
        }
    } while(0);
    std_parse_string_free(ps);
    return lst;
}


void _fn_switch_parser(std_config_node_t node, void *user_data) {
    const char * name = std_config_name_get(node);

    if (failed) return;    //if already failed.. no sense in continuing

    if (strcmp(name,"switch_topology")==0) {
        const char * switch_ids = std_config_attr_get(node,"switch_ids");
        if (switch_ids==NULL) {
            EV_LOGGING(NAS_COM, ERR, "SWITCH","Missing switch_ids - switch file config. Exiting... fix %s",switch_cfg_path);
            return;
        }

        switches.switch_list = int_array_from_string(switch_ids,&switches.number_of_switches);
        if (switches.switch_list==NULL) {
            EV_LOGGING(NAS_COM, ERR, "SWITCH","Switch ids are invalid %s - switch file config. Exiting... fix %s",switch_ids,switch_cfg_path);
            failed = true; return;
        }
        //really no point in cleaning up because the switch init failure will cause the calling process to exit

        switch_cfg = (nas_switch_detail_t *)calloc(switches.number_of_switches,sizeof(*switch_cfg));
        if (switch_cfg==NULL) failed = true;
        if (failed) EV_LOGGING(NAS_COM, ERR, "SWITCH","memory alloc failed Switch ID %s Switch File config %s. Exiting...",
                               switch_ids,switch_cfg_path);
    }

    if (strcmp(name,"switch")==0) {
        const char * id = std_config_attr_get(node,"id");
        const char * npus = std_config_attr_get(node,"npus");
        const char * hw_info = std_config_attr_get(node,"hw_info");
        const char * _cpu_port_id = std_config_attr_get(node,"cpu_port_id");
        if (id==NULL || npus==NULL) {
            EV_LOGGING(NAS_COM, ERR, "SWITCH","Invalid switch config. Exiting... fix %s",switch_cfg_path);
            return;
        }
        size_t switch_ix =atoi(id);
        if (switch_ix >= switches.number_of_switches) {
            EV_LOGGING(NAS_COM, ERR, "SWITCH","Invalid switch id = %s. Exiting... fix %s",id,switch_cfg_path);
            failed = true;
            return;
        }
        switch_cfg[switch_ix].npus = (npu_id_t*)int_array_from_string(npus,&switch_cfg[switch_ix].number_of_npus);
        if (switch_cfg[switch_ix].npus==NULL) failed = true;

        num_switches += switch_cfg[switch_ix].number_of_npus;
        memset(switch_cfg[switch_ix].hw_info,0,MAX_HW_INFO_SZ+1);
        if (hw_info != NULL) {
            safestrncpy((char *)switch_cfg[switch_ix].hw_info, hw_info, MAX_HW_INFO_SZ);
        }
        switch_cfg[switch_ix].cpu_port_id = (_cpu_port_id == NULL) ?  0 : atoi(_cpu_port_id);

    }
    if (strncmp(name,"switch_feature", strlen(name))==0) {
        const char * status = std_config_attr_get(node,"fc_enabled");
        if (status == NULL) {
            EV_LOGGING(NAS_COM, ERR, "SWITCH","fc_enabled not present in %s",switch_cfg_path);
            return;
        }
        nas_sw_fc_supported = atoi(status);
        EV_LOGGING(NAS_COM, INFO, "SWITCH","fc_enabled %d",nas_sw_fc_supported);
    }
    if (strncmp(name,"switch_profile", strlen(name))==0) {
        EV_LOGGING(NAS_COM, INFO, "SWITCH"," Parse switch profile info");
        nas_switch_update_profile_info(node);
    }
    if (strncmp(name,"switch_npu_profile", strlen(name))==0) {
        EV_LOGGING(NAS_COM, INFO, "SWITCH"," Parse NPU Switch Profile Info");
        nas_switch_update_npu_profile_info(node);
    }
    if (strncmp(name,"switch_forwarding_mode", strlen(name))==0) {
        EV_LOGGING(NAS_COM, INFO, "SWITCH"," Parse switch UFT info");
        nas_switch_update_uft_info(node);
    }
    if (strncmp(name,"switch_ecmp", strlen(name))==0) {
        EV_LOGGING(NAS_COM, INFO, "SWITCH"," Parse switch ECMP info");
        nas_switch_update_ecmp_info(node);
    }

    if (strncmp(name,"ipv6_extended_prefix", strlen(name))==0)
    {
        EV_LOGGING(NAS_COM, INFO, "SWITCH"," Parse switch IPv6 Extended prefix info");
        nas_switch_update_ipv6_extended_prefix_info (node);
    }

    if (strncmp(name,"switch_acl_profile", strlen(name))==0) {
        EV_LOGGING(NAS_COM, INFO, "SWITCH"," Parse switch ACL profile info");
        nas_switch_update_acl_profile_info (node);
    }
}

t_std_error nas_switch_init(void) {

    switch_cfg_path = std_getenv("DN_SWITCH_CFG");
    if (switch_cfg_path==NULL) switch_cfg_path =NAS_SWITCH_CFG_DEFAULT_PATH;

    std_config_hdl_t h = std_config_load(switch_cfg_path);
    if (h==NULL) return STD_ERR(HALCOM,FAIL,0);
    t_std_error rc = STD_ERR(HALCOM,FAIL,0);;
    do {
        std_config_node_t node = std_config_get_root(h);
        if (node==NULL) break;

        std_config_for_each_node(node,_fn_switch_parser,NULL);
        rc = STD_ERR_OK;
    } while(0);

    std_config_unload(h);
    if (failed) rc = STD_ERR(HALCOM,FAIL,0);

    /* Read from DB and update */
    rc = nas_sw_parse_db_and_update(0);
    if (rc != STD_ERR_OK)
    {
        EV_LOGGING(NAS_COM, ERR, "SWITCH"," reading init config from DB failed");
        return STD_ERR(HALCOM,FAIL,0);
    }

    return rc;
}

const nas_switches_t * nas_switch_inventory() {
    return &switches;
}

const nas_switch_detail_t *nas_switch(nas_switch_id_t switch_id) {
    if (switch_id >= nas_switch_inventory()->number_of_switches) return NULL;
    return &(switch_cfg[switch_id]);
}

bool nas_find_switch_id_by_npu(uint_t npu_id, nas_switch_id_t * p_switch_id) {
    int switch_id = 0;
    int npu;
    for ( ; switch_id < nas_switch_inventory()->number_of_switches; switch_id++) {
        for (npu = 0; npu < switch_cfg[switch_id].number_of_npus; npu++) {
            if (switch_cfg[switch_id].npus[npu] == npu_id) {
                if (p_switch_id)
                    *p_switch_id = switch_id;
                return true;
            }
        }
    }

    return false;
}

const char *nas_switch_get_hw_info(nas_switch_id_t switch_id)
{
    const nas_switch_detail_t *switch_cfg = nas_switch(switch_id);
    if (switch_cfg != NULL) {
        return ((char *)switch_cfg->hw_info);
    } else {
        return NULL;
    }
}

uint32_t nas_switch_get_cpu_port_id (nas_switch_id_t switch_id)
{
    const nas_switch_detail_t *switch_cfg = nas_switch(switch_id);
    if (switch_cfg != NULL) {
        return (switch_cfg->cpu_port_id);
    } else {
        return 0;
    }
}

