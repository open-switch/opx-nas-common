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

#include <stdio.h>
#include <iostream>
#include <stdlib.h>
#include "gtest/gtest.h"
#include "std_bit_masks.h"
#include "nas_base_utils.h"
#include "event_log.h"

TEST (nas_util_test, attr_set_and_mem_helper_test)
{
    nas::attr_set_t attrs;

    attrs.add (4);
    attrs.add (7);
    attrs.add (8);
    attrs.add (4);

    printf ("Walking attribute set\n");
    for (auto attr_id: attrs) {
        std::cout << attr_id << ", ";
    }
    printf ("\r\n");

    ASSERT_TRUE (attrs.contains(7));
    ASSERT_FALSE (attrs.contains(5));

    nas::mem_alloc_helper_t  m_helper;
    nas::attr_set_t attrs2;
    attrs2.add (0);
    attrs2.add (3);
    attrs2.add (5);
    attrs2.add (9);
    printf ("Walking attribute set 2\n");
    for (auto attr_id: attrs2) {
        std::cout << attr_id << ", ";
    }
    printf ("\r\n");


    attrs += attrs2;
    printf ("Walking union attribute set\n");
    for (auto attr_id: attrs) {
        std::cout << attr_id << ", ";
    }
    printf ("\r\n");

    ASSERT_TRUE (attrs.contains(7));
    ASSERT_TRUE (attrs.contains(0));
    ASSERT_TRUE (attrs.contains(5));
    ASSERT_TRUE (attrs.contains(9));

    nas_attr_id_t* attr_id_p;
    size_t num_attr_ids;
    attrs.to_array (&attr_id_p, &num_attr_ids, m_helper);
    std::cout << "Attr Id List (Len " << num_attr_ids << "): " << std::endl;
    for (uint_t i=0; i< num_attr_ids; i++) {
        std::cout << attr_id_p[i] << ", ";
    }
    printf ("\r\n");
    nas_attr_id_t*attr_array = m_helper.alloc<nas_attr_id_t> (attrs.count());
    attrs.to_array (attr_array, attrs.count());
    std::cout << "Attr Id List (Len " << attrs.count() << "): " << std::endl;
    for (uint_t i=0; i< attrs.count(); i++) {
        std::cout << attr_array[i] << ", ";
    }

    printf ("\r\n");
}

TEST (nas_util_test, npu_set_test)
{
    nas::npu_set_t npus1;

    npus1.add (4);
    npus1.add (7);
    npus1.add (15);
    npus1.add (4);
    npus1.add (9);

    printf ("NPU set 1 contains\n");
    std::string str;
    npus1.dump (str);
    std::cout << str << std::endl;

    nas::npu_set_t npus2;

    npus2.add (4);
    npus2.add (8);
    npus2.add (10);
    npus2.add (9);

    printf ("NPI set 2 contains\n");
    str = "";
    npus2.dump (str);
    std::cout << str << std::endl;

    nas::npu_set_t npus_mine, npus_second, npus_both;
    npus1.compare (npus2, npus_mine, npus_second, npus_both);

    ASSERT_TRUE (npus_mine.contains(7));
    ASSERT_TRUE (npus_mine.contains(15));
    ASSERT_TRUE (npus_second.contains(8));
    ASSERT_TRUE (npus_second.contains(10));
    ASSERT_TRUE (npus_both.contains(4));
    ASSERT_TRUE (npus_both.contains(9));

    printf ("Only in NPU set 1\n");
    str = "";
    npus_mine.dump (str);
    std::cout << str << std::endl;

    printf ("Only in NPU set 2\n");
    str = "";
    npus_second.dump (str);
    std::cout << str << std::endl;

    printf ("In both NPU sets\n");
    str = "";
    npus_both.dump (str);
    std::cout << str << std::endl;
}

TEST (nas_util_test, id_gen_test)
{
    for (size_t max_id=6; max_id<=18; max_id++)
    {
        printf ("\r\n==== Generating ID with Max ID = %ld\r\n", max_id);
    nas::id_generator_t id_gen{max_id};

    ASSERT_TRUE (id_gen.reserve_id(max_id));
    ASSERT_FALSE (id_gen.reserve_id(max_id+1));
    ASSERT_FALSE (id_gen.reserve_id(max_id+5));
    id_gen.release_id(max_id);

    for (size_t i=1;i<=max_id;i++)
        printf ("Generated ID = %ld\n", id_gen.alloc_id());

    try {
        printf ("Generated ID = %ld\n", id_gen.alloc_id());
        ASSERT_TRUE (0);
        printf ("Generated ID = %ld\n", id_gen.alloc_id());
    } catch (nas::base_exception& e) {
        printf ("%s\n", e.err_msg.c_str());
    }

    ASSERT_FALSE (id_gen.reserve_id(4));

    printf ("Releasing IDs 4 and 6\n");
    id_gen.release_id (4);
    id_gen.release_id (6);

    ASSERT_TRUE (id_gen.reserve_id(4));
    ASSERT_FALSE (id_gen.reserve_id(4));
    id_gen.release_id (4);

    printf ("Generated ID = %ld\n", id_gen.alloc_id());
    printf ("Generated ID = %ld\n", id_gen.alloc_id());

    try {
        printf ("Generated ID = %ld\n", id_gen.alloc_id());
        ASSERT_TRUE (0);
        printf ("Generated ID = %ld\n", id_gen.alloc_id());
    } catch (nas::base_exception& e) {
        printf ("%s", e.err_msg.c_str());
    }
    }
}

int main(int argc, char **argv) {
  ::testing::InitGoogleTest(&argc, argv);
  return RUN_ALL_TESTS();
}


