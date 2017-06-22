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
 * filename: hal_intf_mapping.h
 **/

/**
 * \file hal_intf_mapping.h
 * \brief Interface information management.
 **/

#ifndef __HAL_INTF_COM_H_
#define __HAL_INTF_COM_H_

#include "std_error_codes.h"
#include "ds_common_types.h"
#include "dell-base-interface-common.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @defgroup NDIBaseIntfCommon NDI Common - Interface Mapping
 * Functions to retrieve and manage port - interface mapping
 *
 * @{
 */

typedef enum  {
    nas_int_type_PORT=0,
    nas_int_type_VLAN=1,
    nas_int_type_LAG=2,
    nas_int_type_CPU=3,
    nas_int_type_LPBK=4,
    nas_int_type_FC=5,
    nas_int_type_MGMT=6, /* Management interface type */
}nas_int_type_t;

/*!
 *  Interface information types (used in various operations).
 */
typedef enum intf_info_e{
    /* intf info from unit and port */
    HAL_INTF_INFO_FROM_PORT = 1,

    /* intf info from if index */
    HAL_INTF_INFO_FROM_IF,

    /* intf info from tap index */
    HAL_INTF_INFO_FROM_TAP,

    /* intf info from if name */
    HAL_INTF_INFO_FROM_IF_NAME,

} intf_info_t;

typedef enum {
    /* intf info from unit and port */
    HAL_INTF_OP_REG = 1,
    HAL_INTF_OP_DEREG = 2,
} hal_intf_reg_op_type_t;

/*!
 * Interface control structure for if_index <-> npu port mapping
 */
typedef struct _interface_ctrl_s{
    intf_info_t q_type;    //! type of the query being done or 0

    //these fields should be filled in
    nas_int_type_t int_type;    //!the interface type

    //the sub_type value is of type BASE_IF_XXX_TYPE_t based on int_type
    unsigned int int_sub_type;  //!the interface sub type

    //must be unique
    hal_vrf_id_t vrf_id;    //! VRF id
    hal_ifindex_t if_index;    //!if index under the VRF

    //the following fields are optional based on what is being mapped
    npu_id_t npu_id;    //! the npu id
    npu_port_t port_id;    //! the port id
    int tap_id;            //!virtual interface id
    bool sub_port;        //! if port is split into smaller ports
    port_t sub_interface;    //! the sub interface for the port if necessary
    char if_name[HAL_IF_NAME_SZ];    //! the name of the interface
    hal_vlan_id_t vlan_id;         //!the vlan id
    nas_lag_id_t lag_id;         //!the lag id

}interface_ctrl_t;


/*!
 *  Register interface.  "npu_id" and "port_id" must be set in the "details"
 *  \param[in] reg_opt operation to perform (register/deregister)
 *  \param[in] details interface details
 *  \return    std_error
 */
t_std_error dn_hal_if_register(hal_intf_reg_op_type_t reg_opt,interface_ctrl_t *details);

bool nas_to_ietf_if_type_get(nas_int_type_t if_type, char *ietf_type, size_t size);

bool ietf_to_nas_if_type_get(const char *ietf_type, nas_int_type_t *if_type);
/*!
 *  Function to get interface info.
 *  Must provide if_index and qtype in p_intf_ctrl when calling the function
 *  \param[out] interface_ctrl_t All fields populated with interface information
 *  \return     std_error
 */
t_std_error dn_hal_get_interface_info(interface_ctrl_t *p_intf_ctrl);

/**
 * Debug print of the entire interface mapping table
 */
void dn_hal_dump_interface_mapping(void);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif


#endif
