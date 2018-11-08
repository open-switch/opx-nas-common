/*
 * Copyright (c) 2017 Dell Inc.
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
 * nas_sw_profile.h
 *
 */

#ifndef NAS_SW_PROFILE_H_
#define NAS_SW_PROFILE_H_

#ifdef __cplusplus
extern "C" {
#endif

/* This file contains structures and function reads info
   from persistent DB (File/CPS DB) and stores it, some
   configurable(which are not effective,which might be effective
   on next save and reload) parameters like UFT mode,profile will
   be stored as next_boot* here for get purpose
*/

typedef struct {
   char name[NAS_CMN_PROFILE_NAME_SIZE+1];
}nas_cmn_profile_name_t;
typedef struct {
   nas_cmn_profile_name_t pname;
}nas_cmn_sw_profile_t;

typedef struct {
    uint32_t mode;
    uint32_t l2_table_size;
    uint32_t l3_route_table_size;
    uint32_t l3_host_table_size;
}nas_cmn_uft_info_t;

typedef struct {
    uint32_t   num_units_per_entry;
    /* app name */
    char       app_name[NAS_CMN_PROFILE_NAME_SIZE+1];
}nas_cmn_acl_profile_app_info_t;

typedef struct {
    /* ACL profile application group information */
    uint8_t         num_app;
    uint8_t         default_pool_count;
    uint8_t         current_pool_count;
   /* configured value will be active on next save and reboot */
    uint8_t         next_boot_pool_count;
    uint16_t        depth_per_pool;
    uint16_t        stage_type;
    /* application group name */
    char            app_group_name[NAS_CMN_PROFILE_NAME_SIZE+1];

    /* information on ACL profile application info for this app-group */
    nas_cmn_acl_profile_app_info_t *app_info;

}nas_cmn_acl_profile_app_group_info_t;

typedef struct {
   uint8_t num_npus;
   uint8_t num_profiles;
   uint8_t num_uft_modes;
   /* ACL profile ingress pool count */
   uint8_t acl_profile_max_ingress_pool_count;
   /* ACL profile egress pool count */
   uint8_t acl_profile_max_egress_pool_count;

   /* All supported profiles */
   nas_cmn_sw_profile_t *profiles;
   /* current active profile name*/
   nas_cmn_profile_name_t current_profile;
   /* current configured (will be active on next save and reboot) profile name*/
   nas_cmn_profile_name_t next_boot_profile;
   /* default profile name */
   nas_cmn_profile_name_t def_profile;
   /* All supported UFT modes with info, the UFT modes starts from
      1, so the array 0 is not used */
   nas_cmn_uft_info_t *uft_info;

   /* All information on ACL profile app-group info */
   nas_cmn_acl_profile_app_group_info_t *acl_profile_app_group_info;

   /* ACL profile validity flag */
   bool acl_profile_valid;

   /* current active UFT mode */
   uint32_t current_uftmode;
   /* current configured (will be active on next save and reboot) UFT mode */
   uint32_t next_boot_uftmode;
   /* default UFT mode */
   uint32_t def_uftmode;
   /* current value of "max ecmp entries per group" */
   uint32_t cur_max_ecmp_per_grp;
   /* configured value of "max ecmp entries per group",
        (will be active on next save and reboot */
   uint32_t next_boot_max_ecmp_per_grp;
   /* default value of "max ecmp entries per group" */
   uint32_t def_max_ecmp_per_grp;

   /* Is configuration required to support IPv6 extended prefix routes in LPM " */
   uint32_t is_ipv6_ext_prefix_cfg_req;

   /* Default value of supported "IPv6 extended prefix routes in LPM" */
   uint32_t def_ipv6_ext_prefix_routes;

   /* Max value of supported "IPv6 extended prefix routes in LPM" */
   uint32_t max_ipv6_ext_prefix_routes;

   /* current value of "IPv6 extended prefix routes in LPM" */
   uint32_t cur_ipv6_ext_prefix_routes;

   /* configured value of "IPv6 extended prefix routes in LPM",
        (will be active on next save and reboot */
   uint32_t next_ipv6_ext_prefix_routes;

   /* IPv6 LPM entries will be allocated in blocks,
    *  This varibale to define block size */
   uint32_t ipv6_ext_prefix_route_blk_size;

}nas_cmn_sw_init_info_t;

t_std_error nas_switch_update_ecmp_info (std_config_node_t node);
t_std_error nas_switch_update_uft_info (std_config_node_t node);
t_std_error nas_switch_update_profile_info(std_config_node_t node);
t_std_error nas_switch_update_npu_profile_info (std_config_node_t node);
t_std_error nas_switch_update_ipv6_extended_prefix_info (std_config_node_t node);
t_std_error nas_switch_update_acl_profile_info (std_config_node_t node);
#ifdef __cplusplus
}
#endif /* extern C */

#endif /* NAS_SW_PROFILE_H_ */
