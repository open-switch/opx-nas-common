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
 * nas_if_utils.cpp
 *
 *  Created on: Jul 29, 2015
 */


#include "nas_if_utils.h"

#include "dell-base-if-lag.h"
#include "dell-base-if.h"
#include "dell-interface.h"
#include "iana-if-type.h"

#include "std_error_codes.h"
#include "ds_common_types.h"
#include "std_mac_utils.h"

#include "cps_api_object_key.h"

#include "cps_api_operation.h"
#include "cps_class_map.h"

#include "event_log.h"

extern "C" t_std_error dn_hal_get_interface_mac(hal_ifindex_t if_index, hal_mac_addr_t mac_addr)
{
    if(mac_addr == NULL) {
        EV_LOGGING (INTERFACE, ERR, "INTF-C","mac_addr_ptr NULL");
        return(STD_ERR_MK(e_std_err_INTERFACE, e_std_err_code_PARAM, 0));
    }

    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    t_std_error rc = STD_ERR_MK(e_std_err_INTERFACE, e_std_err_code_FAIL, 0);

    do {
        if (!cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                                DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_OBJ, cps_api_qualifier_TARGET)) {
            break;
        }

        cps_api_object_attr_add_u32(obj,DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX, if_index);

        if (cps_api_get(&gp) != cps_api_ret_code_OK)
            break;

        if (0 == cps_api_object_list_size(gp.list))
            break;

        obj = cps_api_object_list_get(gp.list,0);

        cps_api_object_attr_t attr = cps_api_object_attr_get(obj,DELL_IF_IF_INTERFACES_INTERFACE_PHYS_ADDRESS);
        if (attr != NULL) {
            void *_mac_str = cps_api_object_attr_data_bin(attr);
            size_t addr_len = strlen(static_cast<char *>(_mac_str));
            if (std_string_to_mac((hal_mac_addr_t *)mac_addr, static_cast<const char *>(_mac_str), addr_len)) {
                rc = STD_ERR_OK;
            }
        }

    } while (0);

    cps_api_get_request_close(&gp);
    return rc;
}

extern "C"
t_std_error dn_nas_lag_get_ndi_ids (hal_ifindex_t if_index, nas_ndi_obj_id_table_handle_t ndi_id_data)
{
    if(ndi_id_data == NULL) {
        EV_LOGGING(INTERFACE, ERR, "INTF-C","Input ndi_data is NULL");
        return(STD_ERR(INTERFACE, PARAM, 0));
    }

    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);

    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);
    t_std_error rc = STD_ERR(INTERFACE, FAIL, 0);

    do {
        if (!cps_api_key_from_attr_with_qual(cps_api_object_key(obj),
                DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_OBJ, cps_api_qualifier_TARGET)) {
            break;
        }

        cps_api_object_attr_add_u32(obj,DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX, if_index);
        cps_api_object_attr_add(obj,IF_INTERFACES_INTERFACE_TYPE,
                (const char *)IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_IEEE8023ADLAG,
                sizeof(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_IEEE8023ADLAG));

        if (cps_api_get(&gp) != cps_api_ret_code_OK)
            break;

        if (0 == cps_api_object_list_size(gp.list))
            break;

        obj = cps_api_object_list_get(gp.list,0);
        cps_api_attr_id_t  attr_list[] = {BASE_IF_LAG_IF_INTERFACES_INTERFACE_LAG_OPAQUE_DATA};

        if (!nas_ndi_obj_id_table_cps_unserialize (ndi_id_data, obj, attr_list,
                                                   sizeof (attr_list)/sizeof (cps_api_attr_id_t)))
        {
            EV_LOGGING(INTERFACE, ERR, "INTF-C","Failed to get LAG opaque data");
            break;
        }
        rc = STD_ERR_OK;

    } while (0);

    cps_api_get_request_close(&gp);
    return rc;
}

extern "C"
t_std_error nas_get_lag_if_index (uint64_t ndi_port, hal_ifindex_t *lag_if_index)
{
    cps_api_get_params_t gp;
    cps_api_get_request_init(&gp);
    cps_api_get_request_guard rg(&gp);

    EV_LOGGING (INTERFACE, DEBUG, "INTF-C", "querying for ifindex of ndi lag id 0x%x ", ndi_port);
    cps_api_object_t obj = cps_api_object_list_create_obj_and_append(gp.filters);

    cps_api_key_from_attr_with_qual(cps_api_object_key(obj), DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_OBJ,
            cps_api_qualifier_TARGET);

    cps_api_object_attr_add_u64(obj,BASE_IF_LAG_IF_INTERFACES_INTERFACE_LAG_OPAQUE_DATA, ndi_port);
    cps_api_object_attr_add(obj,IF_INTERFACES_INTERFACE_TYPE,
                  (const char *)IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_IEEE8023ADLAG,
                  sizeof(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_IEEE8023ADLAG));

    if (cps_api_get(&gp)==cps_api_ret_code_OK) {
        size_t mx = cps_api_object_list_size(gp.list);
        for (size_t ix = 0 ; ix < mx ; ++ix ) {
            cps_api_object_t obj = cps_api_object_list_get(gp.list,ix);
            cps_api_object_it_t it;
            cps_api_object_it_begin(obj,&it);
            cps_api_object_attr_t attr = cps_api_object_it_find(&it, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX);
            if (attr) {
                *lag_if_index = cps_api_object_attr_data_u32(attr);
                 return STD_ERR_OK;
            }
        }
    }
    return STD_ERR(INTERFACE,FAIL,0);
}

