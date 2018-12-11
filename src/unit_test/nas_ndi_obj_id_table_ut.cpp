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
 * filename: nas_ndi_acl_ut.cpp
 */
#include <iostream>
#include "nas_ndi_obj_id_table.h"
#include <gtest/gtest.h>

TEST(ndi_obj_id_map_test, cpluplus) {
    nas::ndi_obj_id_table_t  ndi_obj_ids;

    ndi_obj_ids[1] = 4;
    ndi_obj_ids[2] = 6;

    auto cps_obj = cps_api_object_create ();
    cps_api_attr_id_t  attr_id_list[] = {5};
    nas::ndi_obj_id_table_cps_serialize (ndi_obj_ids, cps_obj,
                                         attr_id_list, 1);

    nas::ndi_obj_id_table_t  ndi_obj_ids2;

    nas::ndi_obj_id_table_cps_unserialize (ndi_obj_ids2, cps_obj,
                                           attr_id_list, 1);

    EXPECT_TRUE (ndi_obj_ids == ndi_obj_ids2);
}

TEST(ndi_obj_id_map_test, c_api) {
    nas::ndi_obj_id_table_t  ndi_obj_ids;

    auto handle = nas_ndi_obj_id_table_create ();
    nas_ndi_obj_id_table_set_id (handle, 49,4);
    nas_ndi_obj_id_table_set_id (handle, 99,6);

    auto cps_obj = cps_api_object_create ();
    cps_api_attr_id_t  attr_id_list[] = {5,6};
    nas_ndi_obj_id_table_cps_serialize (handle, cps_obj,
                                        attr_id_list, 2);

    auto handle2 = nas_ndi_obj_id_table_create ();

    nas_ndi_obj_id_table_cps_unserialize (handle2, cps_obj,
                                          attr_id_list, 2);

    auto t1 = (nas::ndi_obj_id_table_t*) handle;
    auto t2 = (nas::ndi_obj_id_table_t*) handle2;

    EXPECT_TRUE (*t1 == *t2);
    ndi_obj_id_t  ndi_id;
    EXPECT_TRUE (nas_ndi_obj_id_table_get_id (handle, 49, &ndi_id));
    EXPECT_TRUE (ndi_id == 4);
    EXPECT_FALSE (nas_ndi_obj_id_table_get_id (handle, 48, &ndi_id));
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}
