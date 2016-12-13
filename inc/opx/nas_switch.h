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
 * nas_switch.h
 *
 *  Created on: Mar 31, 2015
 */

#ifndef NAS_COMMON_INC_NAS_SWITCH_H_
#define NAS_COMMON_INC_NAS_SWITCH_H_

#include "ds_common_types.h"
#include "std_error_codes.h"
#include "nas_types.h"


#ifdef __cplusplus
extern "C" {
#endif

/** \defgroup NASCommon Common utilities within NAS
*
* \{
*/

#define NAS_SWITCH_CFG_DEFAULT_PATH "/etc/opt/dell/os10/switch.xml"

/**
 * \file This file contains the standard switch mapping.  The switch mapping will be static in the initial release but
 * in future releases there will likely be some partitioning.
 **/

/**
 * The structure that describes all switches in the NAS.
 */
typedef struct {
    size_t number_of_switches;    //number of switches in the switch list
    nas_switch_id_t *switch_list;     //the list of switches
}nas_switches_t ;

/**
 * For each virtual switch the following structure will be created.
 */
typedef struct {
    size_t number_of_npus;
    npu_id_t * npus;
} nas_switch_detail_t ;

/**
 * Initialize the internal data stuctures that contain the switch mapping.
 * @return a STD_ERR_OK if successful
 */
t_std_error nas_switch_init(void);


/**
 * Returns the details about which switches are available in the NAS
 * @return constant a pointer to the switch details
 */
const nas_switches_t * nas_switch_inventory();

/**
 * Get the list of NPUs for a given switch
 * @param switch_id for the specified switch ID, return the list of NPUs and the length of the list.
 * @return
 */
const nas_switch_detail_t *nas_switch(nas_switch_id_t switch_id);

/**
 * Get the Switch id by npu-id
 * @param npu-id
 * @param[out] switch_id if found
 * @return true if found, otherwise false
 */
bool nas_find_switch_id_by_npu(uint_t npu_id, nas_switch_id_t *switch_id);

/**
 * Get the maximum number of NPUs that are available in the system.
 *
 * @return return the total number of NPUs in all switches product
 */
size_t nas_switch_get_max_npus(void);


/**
 * @brief   Returns allocated BASE MAC address of the system
 * @param   pointer to the mac address.  This MAC address must be precisely 6 bytes long
 * @return  std error code.
 */
t_std_error nas_switch_get_sys_mac_base(hal_mac_addr_t *mac_base);

/**
 * Query the switch module to get the system mac and wait for it to respond.
 *
 * @param mac_base[out] the MAC address
 */
void nas_switch_wait_for_sys_base_mac(hal_mac_addr_t *mac_base);

/**
 * Gets base mac from PAS
 */

t_std_error nas_get_platform_base_mac_address(hal_mac_addr_t *mac_base);
/**
* \}
*/

#ifdef __cplusplus
}
#endif


#endif /* NAS_COMMON_INC_NAS_SWITCH_H_ */
