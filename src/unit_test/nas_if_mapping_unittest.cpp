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
 * nas_if_mapping_unittest.cpp
 *
 *  Created on: Jul 29, 2015
 */



#include "hal_if_mapping.h"
#include "std_utils.h"

#include <gtest/gtest.h>
#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include <string.h>


TEST(nas_if_mapping, npu_port) {
    interface_ctrl_t r;
    memset(&r,0,sizeof(r));
    r.port_mapped = true;
    r.npu_id = 1;
    r.port_id = 2;
    r.if_index = 3;
    r.vrf_id = 4;
    strncpy(r.if_name,"cliff",sizeof(r.if_name)-1);

    ASSERT_TRUE(dn_hal_if_register(HAL_INTF_OP_REG,&r)==STD_ERR_OK);
    ASSERT_FALSE(dn_hal_if_register(HAL_INTF_OP_REG,&r)==STD_ERR_OK);

    // reset for query
    memset(&r,0,sizeof(r));
    r.npu_id = 1;
    r.port_id = 2;
    r.if_index = 3;
    r.vrf_id = 4;
    strncpy(r.if_name,"cliff",sizeof(r.if_name)-1);

    r.q_type = HAL_INTF_INFO_FROM_PORT;
    ASSERT_TRUE(dn_hal_get_interface_info(&r)==STD_ERR_OK);

    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    ASSERT_TRUE(dn_hal_get_interface_info(&r)==STD_ERR_OK);

    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    r.if_name[0]='x';
    ASSERT_FALSE(dn_hal_get_interface_info(&r)==STD_ERR_OK);

    //reset
    r.q_type = HAL_INTF_INFO_FROM_PORT;
    ASSERT_TRUE(dn_hal_get_interface_info(&r)==STD_ERR_OK);

    r.q_type = HAL_INTF_INFO_FROM_PORT;
    r.port_id = 0;
    ASSERT_FALSE(dn_hal_get_interface_info(&r)==STD_ERR_OK);

    r.q_type = HAL_INTF_INFO_FROM_PORT;
    r.port_id = 2;
    r.npu_id = 0;
    ASSERT_FALSE(dn_hal_get_interface_info(&r)==STD_ERR_OK);

    //reset
    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    ASSERT_TRUE(dn_hal_get_interface_info(&r)==STD_ERR_OK);

    r.if_index = -1;
    r.q_type = HAL_INTF_INFO_FROM_IF;
    ASSERT_FALSE(dn_hal_get_interface_info(&r)==STD_ERR_OK);

    //reset
    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    ASSERT_TRUE(dn_hal_get_interface_info(&r)==STD_ERR_OK);

    ASSERT_TRUE(dn_hal_if_register(HAL_INTF_OP_DEREG,&r)==STD_ERR_OK);

    r.q_type = HAL_INTF_INFO_FROM_PORT;
    ASSERT_TRUE(dn_hal_get_interface_info(&r)!=STD_ERR_OK);

    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    ASSERT_TRUE(dn_hal_get_interface_info(&r)!=STD_ERR_OK);
}

TEST(nas_if_mapping, limits) {

    interface_ctrl_t r;
    memset(&r,0,sizeof(r));
    size_t ix = 0;
    size_t mx = 1000;
    for ( ; ix < mx ; ++ix ) {
        r.npu_id = 0;
        r.port_id = ix+1;
        r.if_index = ix+1;
        r.vrf_id = 0;
        r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
        snprintf(r.if_name,sizeof(r.if_name),"cliff%d",(int)ix);

        ASSERT_TRUE(dn_hal_if_register(HAL_INTF_OP_REG,&r)==STD_ERR_OK);
    }

    ix = 0;
    mx = 1000;
    for ( ; ix < mx ; ++ix ) {
        memset(&r,0,sizeof(r));
        r.if_index = ix+1;
        r.vrf_id = 0;

        r.q_type = HAL_INTF_INFO_FROM_IF;
        ASSERT_TRUE(dn_hal_get_interface_info(&r)==STD_ERR_OK);
    }
}

TEST(nas_if_mapping, virtual_port) {
    interface_ctrl_t r;
    memset(&r,0,sizeof(r));
    r.port_mapped = false;
    r.vrf_id = 1;
    safestrncpy(r.if_name,"virt_intf",sizeof(r.if_name));

    ASSERT_TRUE(dn_hal_if_register(HAL_INTF_OP_REG,&r)==STD_ERR_OK);
    ASSERT_FALSE(dn_hal_if_register(HAL_INTF_OP_REG,&r)==STD_ERR_OK);

    memset(&r,0,sizeof(r));
    r.vrf_id = 1;
    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    safestrncpy(r.if_name,"unknown",sizeof(r.if_name));
    ASSERT_FALSE(dn_hal_get_interface_info(&r)==STD_ERR_OK);

    memset(&r,0,sizeof(r));
    r.vrf_id = 1;
    r.q_type = HAL_INTF_INFO_FROM_IF_NAME;
    safestrncpy(r.if_name,"virt_intf",sizeof(r.if_name));
    ASSERT_TRUE(dn_hal_get_interface_info(&r)==STD_ERR_OK);
    ASSERT_TRUE(dn_hal_if_register(HAL_INTF_OP_DEREG,&r)==STD_ERR_OK);
    ASSERT_TRUE(dn_hal_if_register(HAL_INTF_OP_DEREG,&r)==STD_ERR_OK);
}

TEST(nas_if_mapping, get_next_ifindex) {
    hal_ifindex_t *current_ifindex = nullptr;
    hal_ifindex_t next_ifindex;
    t_std_error ret;

    std::cout << "IFIndexes in the system: " << std::endl;
    do {
        ret = dn_hal_get_next_ifindex(current_ifindex, &next_ifindex);
        std::cout << next_ifindex << std::endl;
        current_ifindex = &next_ifindex;
    } while(ret == STD_ERR_OK);
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


