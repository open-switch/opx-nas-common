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
 * nas_switch_unittest.cpp
 *
 *  Created on: Apr 1, 2015
 */


#include <gtest/gtest.h>

#include "nas_switch.h"
#include "std_config_node.h"
#include "cps_api_operation.h"
#include "nas_sw_profile_api.h"
#include "nas_sw_profile.h"
#include "dell-base-switch-element.h"

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

    /* switch profile UT */

    char current_profile[65] = {0};
    uint32_t profile_count, l2_size, l3_size, host_size, i;
    t_std_error rc;

    profile_count = l2_size = l3_size = host_size = i = 0;

    if (nas_sw_profile_current_profile_get(0, current_profile, sizeof(current_profile) -1)
                                            == STD_ERR_OK)
    {
       printf("Get Current active switch profile: %s\n", current_profile);
    }
    memset(current_profile, 0, sizeof(current_profile));
    if (nas_sw_profile_conf_profile_get(0, current_profile, sizeof(current_profile) - 1)
                                         == STD_ERR_OK)
    {
       printf("Get Current configured switch profile: %s\n", current_profile);
    }
    memset(current_profile, 0, sizeof(current_profile));

    strncpy(current_profile, "unified", strlen("unified"));

    if(nas_sw_profile_supported(0, current_profile) == 1)
    {
       printf("switch profile %s supported \n", current_profile);
    }
    else
    {
       printf("switch profile %s not supported \n", current_profile);
    }
    memset(current_profile, 0, sizeof(current_profile));

    strncpy(current_profile, "unified-50G", strlen("unified-50G"));

    if(nas_sw_profile_supported(0, current_profile) == 1)
    {
       printf("switch profile %s supported \n", current_profile);
    }
    else
    {
       printf("switch profile %s not supported \n", current_profile);
    }

    memset(current_profile, 0, sizeof(current_profile));
    strncpy(current_profile, "unified-50G", strlen("unified-50G"));
    if (nas_sw_profile_conf_profile_set(0, current_profile,
                                        cps_api_oper_SET) == STD_ERR_OK)
    {
       printf("Set switch profile: %s\n", current_profile);
    }
    memset(current_profile, 0, sizeof(current_profile));
    if (nas_sw_profile_current_profile_get(0, current_profile, sizeof(current_profile) -1)
                                            == STD_ERR_OK)
    {
       printf("Get Current active switch profile: %s\n", current_profile);
    }
    memset(current_profile, 0, sizeof(current_profile));
    if (nas_sw_profile_conf_profile_get(0, current_profile, sizeof(current_profile) - 1)
                                         == STD_ERR_OK)
    {
       printf("Get Current configured switch profile: %s\n", current_profile);
    }
    if (nas_sw_profile_num_profiles_get(0, &profile_count) == STD_ERR_OK)
    {
       printf("Get Current number of switch profiles supported : %d\n", profile_count);
    }
    profile_count = 0;
    if (nas_sw_uft_mode_supported(profile_count) == false)
    {
        printf(" UFT mode %d not supported \r\n", profile_count);
    }
    else
    {
        printf(" UFT mode %d supported \r\n", profile_count);
    }
    profile_count = 5;
    if (nas_sw_uft_mode_supported(profile_count) == false)
    {
        printf(" UFT mode %d not supported \r\n", profile_count);
    }
    else
    {
        printf(" UFT mode %d supported \r\n", profile_count);
    }
    profile_count = 3;
    if (nas_sw_uft_mode_supported(profile_count) == false)
    {
        printf(" UFT mode %d not supported \r\n", profile_count);
    }
    else
    {
        printf(" UFT mode %d supported \r\n", profile_count);
    }
    profile_count = 0;
    if (nas_sw_profile_current_uft_get(&profile_count) == STD_ERR_OK)
    {
       printf("Get Current active UFT mode : %d\n", profile_count);
    }
    profile_count = 0;
    if (nas_sw_profile_conf_uft_get(&profile_count) == STD_ERR_OK)
    {
       printf("Get Current configured UFT mode : %d\n", profile_count);
    }
    profile_count = 3;
    if (nas_sw_profile_conf_uft_set(profile_count,
                                    cps_api_oper_SET) == STD_ERR_OK)
    {
       printf("set UFT mode : %d\n", profile_count);
    }
    profile_count = 0;
    if (nas_sw_profile_current_uft_get(&profile_count) == STD_ERR_OK)
    {
       printf("Get Current active UFT mode after set : %d\n", profile_count);
    }
    profile_count = 0;
    if (nas_sw_profile_conf_uft_get(&profile_count) == STD_ERR_OK)
    {
       printf("Get Current configured UFT mode after set : %d\n", profile_count);
    }
    profile_count = 0;
    if (nas_sw_profile_uft_num_modes_get(&profile_count) == STD_ERR_OK)
    {
       printf("Get supported UFT mode count : %d\n", profile_count);
    }
    i = 1;
    for (i = BASE_SWITCH_UFT_MODE_MIN ; i <= BASE_SWITCH_UFT_MODE_MAX;i++)
    {
        l2_size = l3_size = host_size = 0;
        if (nas_sw_profile_uft_info_get(i, &l2_size,
                                        &l3_size, &host_size) == STD_ERR_OK)
        {
            printf("Get UFT info  : \r\n");
            printf("      UFT mode: %d\r\n", i);
            printf("      l2 size: %d\r\n", l2_size);
            printf("      l3 size: %d\r\n", l3_size);
            printf("      host size: %d\r\n", host_size);
        }
    }
    profile_count = 0;
    if (nas_sw_profile_uft_default_mode_info_get(&profile_count, &l2_size,
                                                 &l3_size, &host_size) == STD_ERR_OK)
    {
       printf("Get default UFT mode info : \r\n");
       printf("      UFT mode: %d\r\n", profile_count);
       printf("      l2 size: %d\r\n", l2_size);
       printf("      l3 size: %d\r\n", l3_size);
       printf("      host size: %d\r\n", host_size);
    }

    profile_count = 0;
    if (nas_sw_max_ecmp_per_grp_supported(profile_count) == false)
    {
        printf("\r\nmax_ecmp_per_grp %d not supported \r\n", profile_count);
    }
    else
    {
        printf("max_ecmp_per_grp %d supported \r\n", profile_count);
    }
    profile_count = NAS_CMN_MAX_ECMP_ENTRIES_PER_GRP_MAX + 2 ;
    if (nas_sw_max_ecmp_per_grp_supported(profile_count) == false)
    {
        printf("max_ecmp_per_grp %d not supported \r\n", profile_count);
    }
    else
    {
        printf("max_ecmp_per_grp %d supported \r\n", profile_count);
    }
    profile_count = 256;
    if (nas_sw_max_ecmp_per_grp_supported(profile_count) == false)
    {
        printf("max_ecmp_per_grp %d not supported \r\n", profile_count);
    }
    else
    {
        printf("max_ecmp_per_grp %d supported \r\n", profile_count);
    }
    profile_count = 0;
    if (nas_sw_profile_cur_max_ecmp_per_grp_get(&profile_count) == STD_ERR_OK)
    {
       printf("Get Current active max_ecmp_per_grp : %d\n", profile_count);
    }
    profile_count = 0;
    if (nas_sw_profile_conf_max_ecmp_per_grp_get(&profile_count) == STD_ERR_OK)
    {
       printf("Get Current configured max_ecmp_per_grp : %d\n", profile_count);
    }
    profile_count = 128;
    if (nas_sw_profile_conf_max_ecmp_per_grp_set(profile_count,
                                    cps_api_oper_SET) == STD_ERR_OK)
    {
       printf("max_ecmp_per_grp mode : %d\n", profile_count);
    }
    profile_count = 0;
    if (nas_sw_profile_cur_max_ecmp_per_grp_get(&profile_count) == STD_ERR_OK)
    {
       printf("Get Current active max_ecmp_per_grp after set : %d\n", profile_count);
    }
    profile_count = 0;
    if (nas_sw_profile_conf_max_ecmp_per_grp_get(&profile_count) == STD_ERR_OK)
    {
       printf("Get Current configured max_ecmp_per_grp after set : %d\n", profile_count);
    }

    rc = nas_sw_parse_db_and_update(0);
    if ((rc != e_std_err_code_NEXIST) && (rc != STD_ERR_OK))
    {
        printf("\r\nreading sw elementsfrom DB failed\r\n");
    }
    return 0;
}
