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
 * nas_switch_unittest.cpp
 *
 *  Created on: Apr 1, 2015
 */


#include <gtest/gtest.h>

#include "nas_switch.h"

int main() {
    nas_switch_init();
    const nas_switches_t * switches = nas_switch_inventory();
    printf ("Switch count is %d\n",(int)switches->number_of_switches);
    size_t ix = 0;
    size_t mx = switches->number_of_switches;
    for ( ; ix < mx ; ++ix ) {
        printf("Switch id %d\n",(int)switches->switch_list[ix]);
        const nas_switch_detail_t * sd = nas_switch((nas_switch_id_t)ix );
        if (sd==NULL) exit(1);
        size_t sw_ix = 0;
        for ( ; sw_ix < sd->number_of_npus; ++sw_ix) {
            printf("NPU %d\n",sd->npus[sw_ix]);
        }
    }

    int npu_id;
    nas_switch_id_t switch_id;
    for (npu_id = 0; npu_id < 8; npu_id++) {
        if (nas_find_switch_id_by_npu(npu_id, &switch_id) == true) {
            printf("NPU_id %d is found on switch %d\n", npu_id, switch_id);
        }
        else {
            printf("NPU_id %d is not found on any switch\n", npu_id);
        }
    }

    return 0;
}
