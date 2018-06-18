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
 * nas_base_ut_intf_desc.cpp
 *
 *  Created on: Mar 22, 2018
 */



#include "hal_if_mapping.h"
#include "std_utils.h"

#include <gtest/gtest.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>

TEST(nas_if_desc, simple) {
    interface_ctrl_t r;
    memset(&r,0,sizeof(r));
    r.port_mapped = true;
    r.npu_id = 1;
    r.port_id = 2;
    r.if_index = 3;
    r.vrf_id = 4;
    r.desc = NULL;
    safestrncpy(r.if_name,"intf1",sizeof(r.if_name));

    ASSERT_TRUE(dn_hal_if_register(HAL_INTF_OP_REG,&r)==STD_ERR_OK);

    //reset 
    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    ASSERT_TRUE(dn_hal_get_interface_info(&r)==STD_ERR_OK);
    
    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    const char *intf_desc = "intf_desc";
    ASSERT_TRUE(dn_hal_update_intf_desc(&r, intf_desc)==STD_ERR_OK);
    
    ASSERT_TRUE(dn_hal_get_interface_info(&r)==STD_ERR_OK);
    ASSERT_TRUE(strlen(r.desc) == strlen(intf_desc));
    
    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    ASSERT_TRUE(dn_hal_update_intf_desc(&r, "")==STD_ERR_OK);
    
    ASSERT_TRUE(dn_hal_get_interface_info(&r)==STD_ERR_OK);
    ASSERT_TRUE(r.desc == nullptr);

    char excess_len_desc[300]; /* length exceed max description length 256 character */
    std::fill_n(excess_len_desc, 300, 'a');
    excess_len_desc[299] = '\0';
    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    ASSERT_FALSE(dn_hal_update_intf_desc(&r, excess_len_desc)==STD_ERR_OK);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}

