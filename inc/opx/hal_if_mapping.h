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

#include "dell-base-common.h"
#include "dell-base-interface-common.h"
#include "std_error_codes.h"
#include "ds_common_types.h"
#include "nas_types.h"
#include "nas_vrf_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MAX_INTF_DESC_LEN 256 //Max length for intf description

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
    nas_int_type_MACVLAN=7,
    nas_int_type_VLANSUB_INTF=8,
    nas_int_type_VXLAN=9,
    nas_int_type_DOT1D_BRIDGE=10,
    nas_int_type_INVALID=256,
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

    /* intf info from VLAN ID */
    HAL_INTF_INFO_FROM_VLAN,

    /* intf info from LAG ID */
    HAL_INTF_INFO_FROM_LAG,

    /* intf info from BRIDGE ID */
    HAL_INTF_INFO_FROM_BRIDGE_ID,

} intf_info_t;

typedef enum {
    /* intf info from unit and port */
    HAL_INTF_OP_REG = 1,
    HAL_INTF_OP_DEREG = 2,
} hal_intf_reg_op_type_t;


typedef struct {
    /* The below two fields are used to get
     * the router interface in VRF context <-> Parent interface */
    hal_vrf_id_t vrf_id;    //! VRF-id of the L3 interface
    hal_ifindex_t if_index;    //! Interface index of the L3 interface
}l3_intf_info_t;

/*!
 * Interface control structure for if_index <-> npu port mapping
 */
#define MAC_STR_SZ 18
typedef struct _interface_ctrl_s{
    char mac_addr[MAC_STR_SZ];  //MAC address in string format "ab:cd:ef:00:11:22"
    char if_name[HAL_IF_NAME_SZ];    //! the name of the interface
    char *desc;     //Pointer to interface description
    union {
        struct {
            bool port_mapped;               //! indicate if interface mapped to physical port
            npu_port_t port_id;             //! the port id (valid only if port_mapped is true)
        };
        struct {
            hal_vlan_id_t vlan_id;          //!the vlan id
        };
        struct {
            lag_id_t lag_id;                //!the lag object id
        };
        struct {
            nas_bridge_id_t bridge_id;                //the bridge object id created in the NDI/NPU
        };
    };
    intf_info_t q_type;    //! type of the query being done or 0

    //these fields should be filled in
    nas_int_type_t int_type;    //!the interface type

    //the sub_type value is of type BASE_IF_XXX_TYPE_t based on int_type
    unsigned int int_sub_type;  //!the interface sub type

    //must be unique
    hal_vrf_id_t vrf_id;    //! VRF id
    char vrf_name[NAS_VRF_NAME_SZ];    //! the VRF name with
                                       //! which this interface is associated
    hal_ifindex_t if_index;    //!if index under the VRF

    //the following fields are optional based on what is being mapped
    npu_id_t npu_id;    //! the npu id
    int tap_id;            //!virtual interface id
    port_t sub_interface;    //! the sub interface for the port if necessary
    /* The below fields are used for VRF operations. */
    l3_intf_info_t l3_intf_info;
} interface_ctrl_t;


/*!
 *  Register interface.  "npu_id" and "port_id" must be set in the "details"
 *  \param[in] reg_opt operation to perform (register/deregister)
 *  \param[in] details interface details
 *  \return    std_error
 */
t_std_error dn_hal_if_register(hal_intf_reg_op_type_t reg_opt,interface_ctrl_t *details);

bool nas_to_ietf_if_type_get(nas_int_type_t if_type, char *ietf_type, size_t size);

bool ietf_to_nas_os_if_type_get(const char *ietf_type, BASE_CMN_INTERFACE_TYPE_t *if_type);

bool ietf_to_nas_if_type_get(const char *ietf_type, nas_int_type_t *if_type);

/*!
 *  Function to get interface info.
 *  Must provide if_index and qtype in p_intf_ctrl when calling the function
 *  \param[out] interface_ctrl_t All fields populated with interface information
 *  \return     std_error
 */
t_std_error dn_hal_get_interface_info(interface_ctrl_t *p_intf_ctrl);

/*!
 *  Update MAC address in the interface control block
 *  \param[in] interface index
 *  \param[in] mac address associated with the interface in string format
 *  \return     std_error
 */
t_std_error dn_hal_update_intf_mac(hal_ifindex_t ifx, const char *mac);

/*!
 *  Update router interface (non-default VRF context) for lower layer interface (default VRF context).
 *  \param[in] vrf-id - VRF identifier of lower layer interface
 *  \param[in] ifx - Interface index of lower layer interface
 *  \param[in] info - Router interface information for operation in VRF context.
 *  \return     std_error
 */

t_std_error nas_cmn_update_router_intf_info(hal_vrf_id_t vrf_id, hal_ifindex_t ifx,
                                            l3_intf_info_t *info);

/*!
 *  Update description in the interface control block
 *  \param[in] interface control block
 *  \param[in] description of interface in string format
 *  \return     std_error
 */
t_std_error dn_hal_update_intf_desc(interface_ctrl_t *p, const char *desc);

/**
 * Debug print of the entire interface mapping table
 */
void dn_hal_dump_interface_mapping(void);

/*!
 *  Get the next available interface index
 *  \param[in] pointer to input interface index
 *  \param[out] pointer to the next available ifindex; in case of null pointer input, gets the lowest available ifindex on the system
 *  \return     std error code
 */
t_std_error dn_hal_get_next_ifindex(hal_ifindex_t *ifindex, hal_ifindex_t *next_ifindex);

/**
 * @}
 */
#ifdef __cplusplus
}
#endif


#endif
