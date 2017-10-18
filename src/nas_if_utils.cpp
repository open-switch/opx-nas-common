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
#include "hal_if_mapping.h"

#include "dell-base-if-lag.h"
#include "dell-base-if.h"
#include "dell-interface.h"
#include "iana-if-type.h"
#include "hal_if_mapping.h"

#include "std_error_codes.h"
#include "ds_common_types.h"
#include "std_mac_utils.h"
#include "std_utils.h"

#include "cps_api_object_key.h"

#include "cps_api_operation.h"
#include "cps_class_map.h"

#include "event_log.h"

t_std_error dn_hal_get_interface_mac(hal_ifindex_t if_index, hal_mac_addr_t mac_addr)
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

bool nas_is_virtual_port(hal_ifindex_t if_idx)
{
    interface_ctrl_t _port;
    memset(&_port, 0, sizeof(_port));

    _port.if_index = if_idx;
    _port.q_type = HAL_INTF_INFO_FROM_IF;

    if (dn_hal_get_interface_info(&_port) != STD_ERR_OK) {
        EV_LOGGING(INTERFACE, ERR,"INTF-C","Failed to get if_info");
        return false;
    }

    if((_port.int_type == nas_int_type_PORT) &&
       (_port.npu_id == 0 && _port.port_id == 0))
        return true;

    return false;
}

bool nas_get_phy_port_mapping_change(cps_api_object_t evt_obj, nas_int_port_mapping_t *mapping_status)
{
    cps_api_key_t match_key;
    cps_api_key_from_attr_with_qual(&match_key,
                                    DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_OBJ,
                                    cps_api_qualifier_OBSERVED);
    cps_api_key_t *evt_key = cps_api_object_key(evt_obj);
    if (cps_api_key_matches(&match_key, evt_key, true) != 0) {
        EV_LOGGING(INTERFACE, ERR, "INTF-C", "Not interface config event");
        return false;
    }
    cps_api_operation_types_t op = cps_api_object_type_operation(evt_key);
    if (op != cps_api_oper_SET) {
        EV_LOGGING(INTERFACE, ERR, "INTF-C", "Not interface config set event");
        return false;
    }
    cps_api_object_attr_t npu_attr =
        cps_api_object_attr_get(evt_obj, BASE_IF_PHY_IF_INTERFACES_INTERFACE_NPU_ID);
    cps_api_object_attr_t port_attr =
        cps_api_object_attr_get(evt_obj, BASE_IF_PHY_IF_INTERFACES_INTERFACE_PORT_ID);
    if (npu_attr == NULL || port_attr == NULL) {
        return false;
    }
    npu_id_t npu = cps_api_object_attr_data_u32(npu_attr);
    npu_port_t port = cps_api_object_attr_data_u32(port_attr);
    if (npu == 0 && port == 0) {
        EV_LOGGING(INTERFACE, ERR, "INTF-C", "Invalid npu and port id in event");
        return false;
    }

    interface_ctrl_t port_info;
    memset(&port_info, 0, sizeof(port_info));
    cps_api_object_attr_t if_attr = cps_api_get_key_data(evt_obj, IF_INTERFACES_INTERFACE_NAME);
    if (if_attr != NULL) {
        safestrncpy(port_info.if_name, (const char *)cps_api_object_attr_data_bin(if_attr),
                    sizeof(port_info.if_name));
        port_info.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    } else {
        if_attr = cps_api_object_attr_get(evt_obj, DELL_BASE_IF_CMN_IF_INTERFACES_INTERFACE_IF_INDEX);
        if (if_attr == NULL) {
            EV_LOGGING(INTERFACE, ERR, "INTF-C", "Could not found if name or ifindex attr in event");
            return false;
        }
        port_info.if_index = cps_api_object_attr_data_u32(if_attr);
        port_info.q_type = HAL_INTF_INFO_FROM_IF;
    }
    if (dn_hal_get_interface_info(&port_info) != STD_ERR_OK) {
        EV_LOGGING(INTERFACE, ERR, "INTF-C", "Failed to get registered interface info");
        return false;
    }

    if (port_info.npu_id == 0 && port_info.port_id == 0) {
        /* NPU port 0 means the registered interface was disassociated and became virtual */
        if (mapping_status) {
            *mapping_status = nas_int_phy_port_UNMAPPED;
        }
        return true;
    }

    if (port_info.npu_id != npu || port_info.port_id != port) {
        EV_LOGGING(INTERFACE, INFO, "INTF-C", "NPU port %d-%d is not the same as current config %d-%d",
                   npu, port, port_info.npu_id, port_info.port_id);
        /* This might happen when interface was moved from one physical port to another in very short time.
           We should treat this case as disassociate to let caller do cleanup on origin port, and there should
           be following event to indicate interface associated again. */
        if (mapping_status) {
            *mapping_status = nas_int_phy_port_UNMAPPED;
        }
        return true;
    }

    /* NPU port is not 0 and is equal to those of registered interface, that means interface was
       associated with physical port */
    if (mapping_status) {
        *mapping_status = nas_int_phy_port_MAPPED;
    }
    return true;
}
