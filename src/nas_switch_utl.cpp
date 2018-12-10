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

/*
 * nas_switch_utl.cpp
 *
 *  Created on: July 23, 2015
 */
#include "dell-base-switch-element.h"

#include "cps_api_object_key.h"
#include "cps_api_operation.h"
#include "cps_api_object_category.h"
#include "event_log.h"
#include "std_utils.h"
#include "std_time_tools.h"

#include "dell-base-pas.h"
#include "ds_common_types.h"
#include "hal_if_mapping.h"
#include "event_log.h"
#include "cps_api_object_key.h"
#include "cps_class_map.h"
#include "nas_switch.h"

#define MAX_TRY_BEFORE_LOG 5

extern "C" {

/*  @TODO Change API name and revisit the implementation of get max npu API */
unsigned int dn_hal_low_get_number_of_npus(void) {
    return 1;
}

t_std_error nas_switch_get_sys_mac_base(hal_mac_addr_t *mac_base)
{
    if(mac_base == NULL) {
        EV_LOGGING(NAS_L2, ERR,"NAS-L2-SWITCH","invalid parameter");
        return(STD_ERR_MK(e_std_err_BOARD, e_std_err_code_PARAM, 0));
    }

    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    t_std_error rc = STD_ERR_MK(e_std_err_BOARD, e_std_err_code_FAIL, 0);

    do {
        if (!cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY, cps_api_qualifier_TARGET)) {
            break;
        }

        uint32_t switch_id = 0; /*  default switch ID. @TODO add switch id  */
        cps_api_set_key_data_uint(obj, BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_SWITCH_ID, &switch_id, sizeof(switch_id));

        if (cps_api_get(&gp) != cps_api_ret_code_OK)
            break;

        if (0 == cps_api_object_list_size(gp.list))
            break;

        obj = cps_api_object_list_get(gp.list,0);

        cps_api_object_attr_t attr = cps_api_object_attr_get(obj, BASE_SWITCH_SWITCHING_ENTITIES_SWITCHING_ENTITY_DEFAULT_MAC_ADDRESS);
        if (attr != NULL) {
            memcpy(*mac_base,cps_api_object_attr_data_bin(attr),sizeof(*mac_base));
            rc = STD_ERR_OK;
        }

    } while (0);

    cps_api_get_request_close(&gp);
    return rc;
}

void nas_switch_wait_for_sys_base_mac(hal_mac_addr_t *mac_base) {
    while (true) {
        if (nas_switch_get_sys_mac_base(mac_base)== STD_ERR_OK) {
            break;
        }
        EV_LOGGING(NAS_COM, DEBUG, "NAS-COM-MAC","Failed to get system mac..");
        std_usleep(MILLI_TO_MICRO(500));
    }
}
}

static t_std_error nas_fetch_base_mac(hal_mac_addr_t *mac_base) {

    if (mac_base == NULL) {
        return(STD_ERR(COM, FAIL, 0));
    }

    cps_api_return_code_t ret = cps_api_ret_code_ERR;
    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                          BASE_PAS_CHASSIS_OBJ, cps_api_qualifier_OBSERVED);
    if (cps_api_get(&gp) != cps_api_ret_code_OK) {
        EV_LOGGING(NAS_COM, ERR, "NAS-COM-MAC", "Error in Getting BASE MAC address from PAS: Error 0x%x ", ret);
        return(STD_ERR(BOARD,FAIL,0));

    }

    cps_api_object_t chassis_obj = cps_api_object_list_get(gp.list,0);
    cps_api_object_attr_t base_mac_attr = cps_api_object_attr_get(chassis_obj,
                               BASE_PAS_CHASSIS_BASE_MAC_ADDRESSES);
    if (base_mac_attr == NULL) {
        return(STD_ERR(COM,FAIL, 0));

    }
    void *_base_mac = cps_api_object_attr_data_bin(base_mac_attr);
    memcpy(*mac_base, _base_mac,sizeof(hal_mac_addr_t));
    cps_api_get_request_close(&gp);
    return(STD_ERR_OK);
}

/* NAS services use nas_switch_wait_for_sys_base_mac to get base mac address*/

t_std_error nas_get_platform_base_mac_address(hal_mac_addr_t *mac_base) {

    cps_api_key_t key;
    size_t counter = MAX_TRY_BEFORE_LOG;

    memset (&key, 0, sizeof (key));
    cps_api_key_from_attr_with_qual(&key,
                          BASE_PAS_CHASSIS_OBJ, cps_api_qualifier_OBSERVED);
    while (!cps_api_is_registered(&key, NULL)) {
        if (counter >= MAX_TRY_BEFORE_LOG) {
            EV_LOGGING(INTERFACE, ERR ,"NAS-COM-MAC","Platform Service not ready when waiting for base mac");
            counter = 0;
        }
        std_usleep(MILLI_TO_MICRO(1000));
        counter++;
    }
    counter = MAX_TRY_BEFORE_LOG;
    while (1) {
        if (nas_fetch_base_mac(mac_base) == STD_ERR_OK)  {
            break;
        }
        if (counter >= MAX_TRY_BEFORE_LOG) {
            EV_LOGGING(INTERFACE, ERR ,"NAS-COM-MAC","Waiting for base mac address");
            counter = 0;
        }
        std_usleep(MILLI_TO_MICRO(1000));
        counter++;
    }
    return(STD_ERR_OK);
}
