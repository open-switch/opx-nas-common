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
 * nas_if_utils.h
 *
 *  Created on: Jul 29, 2015
 */

#ifndef NAS_COMMON_INC_NAS_IF_UTILS_H_
#define NAS_COMMON_INC_NAS_IF_UTILS_H_

#include "std_error_codes.h"
#include "ds_common_types.h"
#include "nas_ndi_obj_id_table.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    nas_int_phy_port_MAPPED,
    nas_int_phy_port_UNMAPPED
} nas_int_port_mapping_t;


/*!
 *  Function to get MAC address for an if_index
 *  \param[in] interface name
 *  \param[out] mac address associated with the interface in string format
 *  \return     std_error
 */
t_std_error dn_hal_get_intf_mac_addr_str(const char *name, char *mac);
/*!
 *  Function to get MAC address for an if_index
 *  \param[in] interface index
 *  \param[out] mac address associated with the interface
 *  \return     std_error
 */
t_std_error dn_hal_get_interface_mac(hal_ifindex_t if_index, hal_mac_addr_t mac_addr);

/*!
 *  Function to get NDI IDs for a LAG interface
 *  \param if_index    [in]  Lag interface index
 *  \param ndi_id_data [out] NDI ID Table retrieved from the LAG Opaque data
 *  \return                  std_error
 */
t_std_error dn_nas_lag_get_ndi_ids (hal_ifindex_t if_index, nas_ndi_obj_id_table_handle_t ndi_id_data);

/*!
 *  Function to get the LAG id given the ifindex.
 *  \param if_index [in] Lag interface index.
 *  \param lag_id [out]  Lag Id.
 *  \return              std_error
 */
t_std_error nas_get_lag_id_from_if_index (hal_ifindex_t if_index, lag_id_t *lag_id);

/*!
 *  Function to get the LAG ifindex given the NDI lag id.
 *  \param ndi_lag_id [in]   NDI Lag id.
 *  \param lag_if_index [out] Lag If index.
 *  \return                  std_error
 */
t_std_error nas_get_lag_if_index (nas_obj_id_t ndi_lag_id, hal_ifindex_t *lag_if_index);

/*!
 *  Function to check if a given interface is virtual port.
 *  \param if_idx [in]   Interface index.
 *  \return              true if virtual port otherwise false.
 */
bool nas_is_virtual_port(hal_ifindex_t if_idx);

/*!
 *  Function to get current physical port mapping status change of logical interface based
 *  on configuration event published
 *  \param evt_obj [in]   Event object
 *  \param mapping_status [out] Current mapping status
 *  \return               True if there is mapping change
 */
bool nas_get_phy_port_mapping_change(cps_api_object_t evt_obj,
                                     nas_int_port_mapping_t *mapping_status);

#ifdef __cplusplus
}
#endif

#endif /* NAS_COMMON_INC_NAS_IF_UTILS_H_ */
