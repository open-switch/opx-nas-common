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
#include <inttypes.h>

/*  Get MAC Address ( string format) from interface control block */
t_std_error dn_hal_get_intf_mac_addr_str(const char *name, char *mac) {

    if (mac == nullptr)  {
        return STD_ERR(INTERFACE,PARAM,0);
    }
    interface_ctrl_t _intf;
    memset(&_intf, 0, sizeof(_intf));
    safestrncpy(_intf.if_name, name, sizeof(_intf.if_name));
    _intf.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    if (dn_hal_get_interface_info(&_intf) != STD_ERR_OK)  {
        return STD_ERR(INTERFACE,CFG,0);
    }
    safestrncpy(mac, (const char *)_intf.mac_addr, sizeof(_intf.mac_addr));

    return STD_ERR_OK;
}


t_std_error dn_hal_get_interface_mac(hal_ifindex_t if_index, hal_mac_addr_t mac_addr)
{

    t_std_error rc = STD_ERR_MK(e_std_err_INTERFACE, e_std_err_code_FAIL, 0);
    if(mac_addr == NULL) {
        EV_LOGGING (INTERFACE, ERR, "INTF-C","mac_addr_ptr NULL");
        return(STD_ERR_MK(e_std_err_INTERFACE, e_std_err_code_PARAM, 0));
    }

    interface_ctrl_t _intf;
    memset(&_intf, 0, sizeof(_intf));
    _intf.if_index = if_index;
    _intf.q_type = HAL_INTF_INFO_FROM_IF;
    if (dn_hal_get_interface_info(&_intf) != STD_ERR_OK)  {
        return STD_ERR(INTERFACE,CFG,0);
    }
    size_t addr_len = strlen(static_cast<char *>(_intf.mac_addr));
    if (std_string_to_mac((hal_mac_addr_t *)mac_addr, static_cast<const char *>(_intf.mac_addr), addr_len)) {
        rc = STD_ERR_OK;
    }

    EV_LOGGING (INTERFACE, INFO, "INTF-C","intf %s mac_addr is %s", _intf.if_name, _intf.mac_addr);
    return rc;
}

t_std_error nas_get_lag_id_from_if_index (hal_ifindex_t if_index, lag_id_t *lag_id)
{
    interface_ctrl_t intf_ctrl;
    t_std_error rc = STD_ERR_OK;

    memset(&intf_ctrl, 0, sizeof(interface_ctrl_t));

    intf_ctrl.q_type = HAL_INTF_INFO_FROM_IF;
    intf_ctrl.if_index = if_index;

    if((rc=dn_hal_get_interface_info(&intf_ctrl)) != STD_ERR_OK) {
        EV_LOGGING(INTERFACE, DEBUG, "INTF-C","Interface %d returned error %d",
                intf_ctrl.if_index, rc);
        return rc;
    }

    if (intf_ctrl.int_type != nas_int_type_LAG) {
        EV_LOGGING(INTERFACE, ERR, "INTF-C","Invalid Interface %d of index %d",
                        intf_ctrl.int_type, intf_ctrl.if_index);
                return STD_ERR(INTERFACE,PARAM,0);
    }
    *lag_id = intf_ctrl.lag_id;
    return rc;
}

t_std_error nas_get_lag_if_index (nas_obj_id_t ndi_lag_id, hal_ifindex_t *lag_if_index)
{
    interface_ctrl_t intf_ctrl;
    t_std_error rc = STD_ERR_OK;

    memset(&intf_ctrl, 0, sizeof(interface_ctrl_t));

    intf_ctrl.q_type = HAL_INTF_INFO_FROM_LAG;
    intf_ctrl.lag_id = static_cast<lag_id_t>(ndi_lag_id);
    intf_ctrl.int_type = nas_int_type_LAG;

    if((rc=dn_hal_get_interface_info(&intf_ctrl)) != STD_ERR_OK) {
        EV_LOGGING(INTERFACE, ERR, "INTF-C","Failed to find ifindex for lag id %" PRId64 " rc=%d",
                intf_ctrl.lag_id, rc);
        return rc;
    }

    if (intf_ctrl.int_type != nas_int_type_LAG) {
        EV_LOGGING(INTERFACE, ERR, "INTF-C","Invalid Interface type %d for lag id %" PRId64,
                        intf_ctrl.int_type, intf_ctrl.lag_id);
                return STD_ERR(INTERFACE,PARAM,0);
    }
    *lag_if_index = intf_ctrl.if_index;
    return rc;
}

bool nas_is_virtual_port(hal_ifindex_t if_idx)
{
    interface_ctrl_t _port;
    memset(&_port, 0, sizeof(_port));

    _port.if_index = if_idx;
    _port.q_type = HAL_INTF_INFO_FROM_IF;

    if (dn_hal_get_interface_info(&_port) != STD_ERR_OK) {
        EV_LOGGING(INTERFACE, DEBUG,"INTF-C","Failed to get if_info");
        return false;
    }

    if (_port.int_type != nas_int_type_PORT) {
        return false;
    }

    return !_port.port_mapped;
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
        EV_LOGGING(INTERFACE, DEBUG, "INTF-C", "Not interface config set event");
        return false;
    }
    cps_api_object_attr_t npu_attr =
        cps_api_object_attr_get(evt_obj, BASE_IF_PHY_IF_INTERFACES_INTERFACE_NPU_ID);
    cps_api_object_attr_t port_attr =
        cps_api_object_attr_get(evt_obj, BASE_IF_PHY_IF_INTERFACES_INTERFACE_PORT_ID);
    if (npu_attr == NULL || port_attr == NULL) {
        EV_LOGGING(INTERFACE, DEBUG, "INTF-C", "No npu port attribute found in event object");
        return false;
    }
    npu_id_t npu = cps_api_object_attr_data_u32(npu_attr);
    npu_port_t port = cps_api_object_attr_data_u32(port_attr);

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

    if (port_info.port_mapped && (port_info.npu_id != npu || port_info.port_id != port)) {
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

    /* NPU port is equal to those of registered interface, association status of interface would be got from
       interface db */
    if (mapping_status) {
        *mapping_status = port_info.port_mapped ? nas_int_phy_port_MAPPED : nas_int_phy_port_UNMAPPED;
    }
    return true;
}

t_std_error nas_com_get_if_index_to_name(hal_ifindex_t if_index, char * name, size_t len, hal_vrf_id_t vrf_id)
{
    interface_ctrl_t intf_ctrl;
    t_std_error rc = STD_ERR_OK;

    memset(&intf_ctrl, 0, sizeof(interface_ctrl_t));

    intf_ctrl.q_type = HAL_INTF_INFO_FROM_IF;
    intf_ctrl.if_index = if_index;
    intf_ctrl.vrf_id = vrf_id;

    if((rc= dn_hal_get_interface_info(&intf_ctrl)) != STD_ERR_OK) {
        EV_LOGGING(INTERFACE, DEBUG, "INT-C",
                   "Interface %d returned error %d", \
                    intf_ctrl.if_index, rc);

        return STD_ERR(INTERFACE,FAIL, rc);
    }
    safestrncpy(name, intf_ctrl.if_name, len);
    return STD_ERR_OK;
}
t_std_error nas_com_get_name_to_if_index(const char *name, hal_ifindex_t *if_index) {

    interface_ctrl_t intf_ctrl;
    t_std_error rc = STD_ERR_OK;

    memset(&intf_ctrl, 0, sizeof(interface_ctrl_t));

    intf_ctrl.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    safestrncpy(intf_ctrl.if_name, name, sizeof(intf_ctrl.if_name));

    if((rc= dn_hal_get_interface_info(&intf_ctrl)) != STD_ERR_OK) {
        EV_LOGGING(INTERFACE, DEBUG, "INT-C",
                   "Interface %s mapping not present %d", intf_ctrl.if_name, rc);
        return STD_ERR(INTERFACE,FAIL, rc);
    }

    *if_index = intf_ctrl.if_index;
    return STD_ERR_OK;
}

t_std_error nas_com_get_if_type(const char *name, nas_int_type_t *type)
{
    interface_ctrl_t intf_ctrl;
    t_std_error rc = STD_ERR_OK;

    memset(&intf_ctrl, 0, sizeof(interface_ctrl_t));

    intf_ctrl.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    safestrncpy(intf_ctrl.if_name, name, sizeof(intf_ctrl.if_name));

    if((rc= dn_hal_get_interface_info(&intf_ctrl)) != STD_ERR_OK) {
        EV_LOGGING(INTERFACE, DEBUG, "NAS-INT",
                   "Interface %d returned error %d", \
                    intf_ctrl.if_index, rc);

        return STD_ERR(INTERFACE,FAIL, rc);
    }

    *type = intf_ctrl.int_type;
    return STD_ERR_OK;
}

