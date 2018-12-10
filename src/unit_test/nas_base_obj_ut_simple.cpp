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
 * nas_base_obj_ut_simple.cpp
 *
 *  Created on: Apr 13, 2015
 */

#include <stdio.h>
#include "gtest/gtest.h"
#include "nas_base_ut_simple_derived_obj.h"
#include "std_error_codes.h"

#define RESET_NPU_ID 10
npu_id_t simulate_ndi_fail = RESET_NPU_ID ;
npu_id_t simulate_ndi_attr1_fail = RESET_NPU_ID;
npu_id_t simulate_ndi_attr2_fail = RESET_NPU_ID;

////////////
// NDI API stubs
////////////
t_std_error ndi_ut_obj_create (npu_id_t npu_id,
                               ndi_ut_obj_t* ndi_ut_obj_p,
                               size_t* ndi_ut_obj_id)
{
    static size_t count = 0;
    if (simulate_ndi_fail == npu_id) {
        printf ("Simulate NDI Create failure in NPU %d\r\n", npu_id);
        simulate_ndi_fail = RESET_NPU_ID ;
        return STD_ERR (NPU, FAIL, 2);
    } else {
        *ndi_ut_obj_id = ++count;
        printf ("   NDI Object Create in NPU %d\r\n", npu_id);
        printf ("     Generated NDI ID %ld\r\n", *ndi_ut_obj_id);
    }
    return STD_ERR_OK;
}

t_std_error ndi_ut_obj_delete (npu_id_t npu_id,
                               size_t ndi_ut_obj_id)
{
    if (simulate_ndi_fail == npu_id) {
        printf ("Simulate NDI Delete failure in NPU %d\r\n", npu_id);
        simulate_ndi_fail = RESET_NPU_ID ;
        return STD_ERR (NPU, FAIL, 2);
    } else {
        printf ("   NDI Object Delete in NPU %d NDI Obj Id %ld\r\n",
                npu_id, ndi_ut_obj_id);
    }
    return STD_ERR_OK;
}

t_std_error ndi_ut_obj_attr1_set (npu_id_t npu_id, size_t ndi_ut_obj_id,
                                  uint_t attr_val)
{
    if (simulate_ndi_attr1_fail == npu_id) {
        printf ("Simulate NDI Attr1 set failure in NPU %d\r\n", npu_id);
        simulate_ndi_attr1_fail = RESET_NPU_ID;
        return STD_ERR (NPU, FAIL, 2);
    } else {
        printf ("   NDI Object Attr1 Modify in NPU %d NDI Obj Id %ld Val %d\r\n",
                npu_id, ndi_ut_obj_id, attr_val);
    }
    return STD_ERR_OK;
}

t_std_error ndi_ut_obj_attr2_set (npu_id_t npu_id, size_t ndi_ut_obj_id,
                                  uint_t attr_val)
{
    if (simulate_ndi_attr2_fail == npu_id) {
        printf ("Simulate NDI Attr2 set failure in NPU %d\r\n", npu_id);
        simulate_ndi_attr2_fail = RESET_NPU_ID;
        return STD_ERR (NPU, FAIL, 2);
    } else {
        printf ("   NDI Object Attr2 Modify in NPU %d NDI Obj Id %ld Val %d\r\n",
                npu_id, ndi_ut_obj_id, attr_val);
    }
    return STD_ERR_OK;
}

derived_switch_t&  base_ut_get_switch (nas_obj_id_t swid)
{
// Example Logical Switch with ID 1
  static derived_switch_t ut_switches[] = {derived_switch_t (0),
                                          derived_switch_t (1) };

  if (swid < 2) {
      return ut_switches[swid];
  }

  throw nas::base_exception {NAS_BASE_E_PARAM, __PRETTY_FUNCTION__,
                            "Logical switch not found"};
}

/////////////////////
// Representation of CPS TLV Message
////////////////////
typedef struct _ut_cps_attrs {
    nas_attr_id_t  attr_id;
    union {
        uint_t attr_val;
        npu_id_t *npus;
    };
} ut_cps_attr_t;

///////////////
// Start Test
//////////////
TEST (nas_base_obj_test, test)
{
    ////////////////////
    // Create Obj test
    ////////////////////
    printf ("CREATE TEST\r\n\r\n");
    for (int i=0;i<3;i++) {

        int max_attrs = i+1;
        ut_cps_attr_t  cps_attr1 = {BASE_UT_ATTR2, (uint_t) i*2};
        ut_cps_attr_t  cps_attr2 = {BASE_UT_ATTR1, (uint_t) i*4};
        ut_cps_attr_t  cps_attr3; cps_attr3.attr_id = BASE_UT_NPU_LIST;
        npu_id_t npus[] = {1}; cps_attr3.npus = npus;

        ut_cps_attr_t*  cps_attrs[] = {&cps_attr1, &cps_attr2, &cps_attr3};

        try {
            derived_switch_t& logical_switch = base_ut_get_switch(0);
            derived_obj_t  obj_new {&logical_switch};  // Create new instance

            for (int attr = 0; attr< max_attrs; attr++) {

                switch (cps_attrs[attr]->attr_id) {
                    case BASE_UT_ATTR1:
                        obj_new.set_attr1 (cps_attrs[attr]->attr_val);
                        break;
                    case BASE_UT_ATTR2:
                        obj_new.set_attr2 (cps_attrs[attr]->attr_val);
                        break;
                    case BASE_UT_NPU_LIST:
                        obj_new.add_npu (cps_attrs[attr]->npus[0]);
                        break;
                }
            }

            obj_new.commit_create (false);
            EXPECT_TRUE (max_attrs != 1);

            if (max_attrs < 3) {
                EXPECT_TRUE (obj_new.npu_list().size() == 3);
            } else {
                EXPECT_TRUE (obj_new.npu_list().size() == 1);
            }
            logical_switch.add_ut_obj (obj_new);

            printf ("Create-Obj %d SUCCESS\r\n", i);
        } catch (nas::base_exception& e) {
            EXPECT_TRUE (max_attrs == 1);
            printf ("Create-Obj %d FAILED: %s, Function: %s ErrCode: %d\r\n",
                    i, e.err_msg.c_str(), e.err_fn.c_str(), e.err_code);
        }
    }

    ////////////////////
    // Modify Obj test
    ////////////////////
    printf ("MODIFY TEST\r\n\r\n");
    for (int i=0;i<2;i++) {
        int max_attrs = 2;
        ut_cps_attr_t  cps_attr1;
        if (i==0) {
            cps_attr1.attr_id = BASE_UT_ATTR2;
            cps_attr1.attr_val = 9;
        } else {
            cps_attr1.attr_id = BASE_UT_ATTR2;
            cps_attr1.attr_val = 3;
        }
        ut_cps_attr_t  cps_attr2; cps_attr2.attr_id = BASE_UT_NPU_LIST;
        npu_id_t npus[] = {0}; cps_attr2.npus = npus;

        ut_cps_attr_t*  cps_attrs[] = {&cps_attr1, &cps_attr2};

        try {
            derived_switch_t& logical_switch = base_ut_get_switch(0);
            derived_obj_t& obj_orig = logical_switch.get_ut_obj (1);
            derived_obj_t  obj_modify {obj_orig};  // Create copy of instance

            for (int attr = 0; attr< max_attrs; attr++) {

                switch (cps_attrs[attr]->attr_id) {
                    case BASE_UT_ATTR1:
                        obj_modify.set_attr1 (cps_attrs[attr]->attr_val);
                        break;
                    case BASE_UT_ATTR2:
                        obj_modify.set_attr2 (cps_attrs[attr]->attr_val);
                        break;
                    case BASE_UT_NPU_LIST:
                        obj_modify.add_npu (cps_attrs[attr]->npus[0]);
                        break;
                }
            }

            obj_modify.commit_modify (obj_orig, false);
            EXPECT_TRUE (obj_modify.npu_list().size() == 1);
            EXPECT_TRUE (i != 0);

            logical_switch.change_ut_obj (obj_modify);

            printf ("Modify-Obj 1 SUCCESS\r\n");
        } catch (nas::base_exception& e) {
            EXPECT_TRUE (i == 0);
            printf ("Modify-Obj 1 FAILED: %s, Function: %s ErrCode: %d\r\n",
                    e.err_msg.c_str(), e.err_fn.c_str(), e.err_code);
        }
    }

    ////////////////////
    // Delete Obj test
    ////////////////////
    printf ("DELETE TEST\r\n\r\n");
    for (int i=1;i<=2;i++) {
        try {
            derived_switch_t& logical_switch = base_ut_get_switch(0);
            derived_obj_t& obj_del = logical_switch.get_ut_obj (i);
            obj_del.commit_delete (false);

            logical_switch.del_ut_obj (i);

            printf ("Del-Obj %d SUCCESS\r\n", i);
        } catch (nas::base_exception& e) {
            EXPECT_TRUE (0);
            printf ("Del-Obj %d FAILED: %s, Function: %s ErrCode: %d\r\n",
                    i, e.err_msg.c_str(), e.err_fn.c_str(), e.err_code);
        }
    }
}

TEST (nas_base_obj_test, npu_rollback_test)
{
    ////////////////////
    // Create Obj test
    ////////////////////
    printf ("CREATE TEST\r\n\r\n");
    for (int i=0;i<2;i++) {

        int max_attrs = 2;
        ut_cps_attr_t  cps_attr1 = {BASE_UT_ATTR2, (uint_t) i*2};
        ut_cps_attr_t  cps_attr2 = {BASE_UT_ATTR1, (uint_t) i*4};

        ut_cps_attr_t*  cps_attrs[] = {&cps_attr1, &cps_attr2};

        try {
            derived_switch_t& logical_switch = base_ut_get_switch(0);
            derived_obj_t  obj_new {&logical_switch};  // Create new instance

            for (int attr = 0; attr< max_attrs; attr++) {

                switch (cps_attrs[attr]->attr_id) {
                    case BASE_UT_ATTR1:
                        obj_new.set_attr1 (cps_attrs[attr]->attr_val);
                        break;
                    case BASE_UT_ATTR2:
                        obj_new.set_attr2 (cps_attrs[attr]->attr_val);
                        break;
                    case BASE_UT_NPU_LIST:
                        obj_new.add_npu (cps_attrs[attr]->npus[0]);
                        break;
                }
            }

            if (i==0)
            simulate_ndi_fail = 2;

            obj_new.commit_create (false);

            EXPECT_TRUE (i!=0);
            EXPECT_TRUE (obj_new.npu_list().size() == 3);
            logical_switch.add_ut_obj (obj_new);

            printf ("Create-Obj %d SUCCESS\r\n", i);
        } catch (nas::base_exception& e) {
            EXPECT_TRUE (i==0);
            printf ("Create-Obj %d FAILED: %s, Function: %s ErrCode: %d\r\n",
                    i, e.err_msg.c_str(), e.err_fn.c_str(), e.err_code);
        }
    }

    ////////////////////
    // Modify Obj test
    ////////////////////
    printf ("MODIFY TEST\r\n\r\n");
    for (int i=0;i<3;i++) {
        int max_attrs = 1;
        ut_cps_attr_t  cps_attr1;
        if (i==0) {
            cps_attr1.attr_id = BASE_UT_ATTR1;
            cps_attr1.attr_val = 5;
        } else {
            cps_attr1.attr_id = BASE_UT_ATTR2;
            cps_attr1.attr_val = 3;
        }
        ut_cps_attr_t  cps_attr2;
        npu_id_t npus[] = {0,1};
        if (i < 2) {
            max_attrs++;
        cps_attr2.attr_id = BASE_UT_NPU_LIST;
        cps_attr2.npus = npus;
        }
        ut_cps_attr_t*  cps_attrs[] = {&cps_attr1, &cps_attr2};

        try {
            derived_switch_t& logical_switch = base_ut_get_switch(0);
            derived_obj_t& obj_orig = logical_switch.get_ut_obj (3);
            derived_obj_t  obj_modify {obj_orig};  // Create copy of instance

            for (int attr = 0; attr< max_attrs; attr++) {

                switch (cps_attrs[attr]->attr_id) {
                    case BASE_UT_ATTR1:
                        obj_modify.set_attr1 (cps_attrs[attr]->attr_val);
                        break;
                    case BASE_UT_ATTR2:
                        obj_modify.set_attr2 (cps_attrs[attr]->attr_val);
                        break;
                    case BASE_UT_NPU_LIST:
                        obj_modify.add_npu (cps_attrs[attr]->npus[0]);
                        obj_modify.add_npu (cps_attrs[attr]->npus[1]);
                        break;
                }
            }

            switch (i)
            {
                case 0:
                    simulate_ndi_attr1_fail = 1;
                    break;
                case 1:
                    simulate_ndi_attr2_fail = 1;
                    break;
                case 2:
                    simulate_ndi_attr2_fail = 2;
                    break;
            }
            obj_modify.commit_modify (obj_orig, false);

            EXPECT_TRUE (0); // All 3 cases should FAIL

            logical_switch.change_ut_obj (obj_modify);

            printf ("Modify-Obj 3 SUCCESS\r\n");
        } catch (nas::base_exception& e) {
            printf ("Modify-Obj 3 FAILED: %s, Function: %s ErrCode: %d\r\n",
                    e.err_msg.c_str(), e.err_fn.c_str(), e.err_code);
        }
    }

    ////////////////////
    // Delete Obj test
    ////////////////////
    printf ("DELETE TEST\r\n\r\n");
    for (int i=3;i<=3;i++) {
    //for (auto: obj: logical_switch.obj_list()) {
        try {
            derived_switch_t& logical_switch = base_ut_get_switch(0);
            derived_obj_t& obj_del = logical_switch.get_ut_obj (i);
            simulate_ndi_fail = 2;
            obj_del.commit_delete (false);
            EXPECT_TRUE (0); // should FAIL

            logical_switch.del_ut_obj (i);

            printf ("Del-Obj %d SUCCESS\r\n", i);
        } catch (nas::base_exception& e) {
            printf ("Del-Obj %d FAILED: %s, Function: %s ErrCode: %d\r\n",
                    i, e.err_msg.c_str(), e.err_fn.c_str(), e.err_code);
        }
    }
}

int main(int argc, char **argv) {
    derived_switch_t& logical_switch = base_ut_get_switch(0);
    logical_switch.add_npu (0);
    logical_switch.add_npu (1);
    logical_switch.add_npu (2);

    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}


