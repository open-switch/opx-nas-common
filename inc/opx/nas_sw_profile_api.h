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
 * nas_switch_profile_api.h
 *
 */

#ifdef __cplusplus
extern "C" {
#endif

#define NAS_CMN_PROFILE_NAME_SIZE 64
#define NAS_CMN_NPU_PROFILE_ATTR_SIZE 256
#define NAS_CMN_MAX_ECMP_ENTRIES_PER_GRP_DEF 64
#define NAS_CMN_MAX_ECMP_ENTRIES_PER_GRP_MAX 1024
#define NAS_CMN_MAX_ECMP_ENTRIES_PER_GRP_MIN 2

#define NAS_CMN_DEFAULT_SWITCH_ID 0

/**
 * switch profile API's can be used by
 * -CPS get/set from nas-l2 and
 * -from nas-ndi to read init parametrs
 */

/**
 * This function gets all the supported profiles.
 * @param sw_id, switch or unit id
 * @param cps_obj, ptr to hold the profiles
 * @return standard error
 */
t_std_error nas_sw_profile_supported_profiles_get(uint32_t sw_id, cps_api_object_t);

/**
 * This function verifies the given profile with
 * supported profiles.
 * @param sw_id, switch or unit id
 * @param conf_profile , ptr to configured profile name
 * @return true if profile supported , otherwise false.
 */
bool nas_sw_profile_supported(uint32_t sw_id, char *conf_profile);

/**
 * This function reads switch elements from persistent DB during boot up
 * and initialize local data structure
 * @param sw_id, switch or unit id
 * @return standard error
 */
t_std_error nas_sw_parse_db_and_update(uint32_t sw_id);

/**
 * This function gets current profile
 * @param sw_id, switch or unit id
 * @param current_profile[out], buffer to store the profile
 * @param len, lenth of the buffer passed
 * @return standard error
 */
t_std_error nas_sw_profile_current_profile_get(uint32_t sw_id, char *current_profile,
                                                uint32_t len);
/**
 * This function gets default profile
 * @param sw_id, switch or unit id
 * @param default_profile[out], buffer to store the profile
 * @param len, lenth of the buffer passed
 * @return standard error
 */
t_std_error nas_sw_profile_default_profile_get(uint32_t sw_id,
                                                char *default_profile,
                                                int len);

/**
 * This function gets configured(effective on next boot) profile
 * @param sw_id, switch or unit id
 * @param conf_profile[out], buffer to store the profile
 * @param len, lenth of the buffer passed
 * @return standard error
 */
t_std_error nas_sw_profile_conf_profile_get(uint32_t sw_id, char *conf_profile,
                                            int len);

/**
 * This function sets the next boot profile, effective on next boot
 * @param sw_id, switch or unit id
 * @param conf_profile, pointer to the profile
 * @param op_type, cps api operation type(create,delete,set etc.)
 * @return standard error
 */
t_std_error nas_sw_profile_conf_profile_set(uint32_t sw_id, char *conf_profile,
                                            cps_api_operation_types_t op_type);

/**
 * This function gets number of profiles supported on switch, it is the
 * values present in the "switch.xml"
 * @param sw_id, switch or unit id
 * @param[out] num_profiles, pointer to the value
 * @return standard error
 */
t_std_error nas_sw_profile_num_profiles_get(uint32_t sw_id,
                                            uint32_t *num_profiles);

/**
 * UFT related get/set and other API's
*/

/**
 * This function verifies the given UFT mode with supported modes.
 * @param mode
 * @return true if mode supported , otherwise false.
 */
bool nas_sw_uft_mode_supported(uint32_t mode);

/**
 * This function gets current uft mode
 * @param current_uft[out], pointer to store the mode
 * @return standard error
 */
t_std_error nas_sw_profile_current_uft_get(uint32_t *current_uft);

/**
 * This function gets the configured/next boot uft mode
 * @param conf_uft[out], pointer to store the mode
 * @return standard error
 */
t_std_error nas_sw_profile_conf_uft_get(uint32_t *conf_uft);

/**
 * This function sets the configured/next boot uft mode, which
 * will be effective on next boot if its saved
 * @param conf_uft, mode value
 * @param op_type, cps api operation type(create,delete,set etc.)
 * @return standard error
 */
t_std_error nas_sw_profile_conf_uft_set(uint32_t conf_uft,
                                        cps_api_operation_types_t op_type);

/**
 * This function gets the different table values for the UFT mode
 * @param uft_mode, mode value
 * @param l2_size[out], MAC FDB table size for the given mode
 * @param l3_size[out], L3 ROUTE table size for the given mode
 * @param host_size[out], L3 HOST/ARP table size for the given mode
 * @return standard error
 */
t_std_error nas_sw_profile_uft_info_get(uint32_t uft_mode,
                                        uint32_t *l2_size,
                                        uint32_t *l3_size,
                                        uint32_t *host_size);

/**
 * This function gets the number of UFT modes supported, the value is
 * same as the one mentioned in "switch.xml"
 * @param num_uft_modes[out], number of modes supported
 * @return standard error
 */
t_std_error nas_sw_profile_uft_num_modes_get(uint32_t *num_uft_modes);

/**
 * This function gets the default UFT mode and different table
 * values for the default UFT mode
 * @param def_uft_mode[out], default mode value
 * @param l2_size[out], MAC FDB table size for the default mode
 * @param l3_size[out], L3 ROUTE table size for the default mode
 * @param host_size[out], L3 HOST/ARP table size for the default mode
 * @return standard error
 */
t_std_error nas_sw_profile_uft_default_mode_info_get(uint32_t *def_uft_mode,
                                                     uint32_t *l2_size,
                                                     uint32_t *l3_size,
                                                     uint32_t *host_size);


/**
 * ECMP relaed API's
 */

/**
 * This function verifies the given ecmp-max_ecmp_per_grp with supported value.
 * @param max_ecmp_per_grp
 * @return true if supported , otherwise false.
 */
bool nas_sw_max_ecmp_per_grp_supported(uint32_t max_ecmp_per_grp);

/**
 * This function gets current max_ecmp_per_grp value
 * @param cur_max_ecmp_per_grp[out], pointer to store the value
 * @return standard error
 */
t_std_error nas_sw_profile_cur_max_ecmp_per_grp_get(uint32_t *cur_max_ecmp_per_grp);

/**
 * This function sets max_ecmp_per_grp value
 * @param conf_max_ecmp_per_grp value
 * @param op_type, cps api operation type(create,delete,set etc.)
 * @return standard error
 */
t_std_error nas_sw_profile_conf_max_ecmp_per_grp_set(uint32_t conf_max_ecmp_per_grp,
                                        cps_api_operation_types_t op_type);
/**
 * This function gets configured(next_boot) value of max_ecmp_per_grp
 * @param conf_max_ecmp_per_grp[out], pointer to store the value
 * @return standard error
 */
t_std_error nas_sw_profile_conf_max_ecmp_per_grp_get(uint32_t *conf_max_ecmp_per_grp);

/**
 * This function gets next key/value pair from Switch NPU Profile populated from
 * the switch.xml file
 * @param Pointer to the next Key Value pair
 * NOTE : This API get the Key/Value attributes that are of size 256
 *        (NAS_CMN_NPU_PROFILE_ATTR_SIZE). Please use this API with caution by
 *        passing Array of size 256 (Eg: nas_ndi_populate_cfg_key_value_pair ())
 * @return standard error
 */
t_std_error nas_switch_npu_profile_get_next_value(char* variable,
                                                  char* value);
/**
 * This function gets cur_ipv6_ext_prefix_routes value
 * @param cur_ipv6_ext_prefix_routes[out], pointer to store the value
 * @return standard error
 */
t_std_error nas_sw_profile_cur_ipv6_ext_prefix_routes_get(uint32_t *cur_ipv6_ext_prefix_routes);

/**
 * This function gets conf_ipv6_ext_prefix_routes value
 * @param conf_ipv6_ext_prefix_routes[out], pointer to store the value
 * @return standard error
 */
t_std_error nas_sw_profile_conf_ipv6_ext_prefix_routes_get(uint32_t *conf_ipv6_ext_prefix_routes);

/**
 * This function sets conf ipv6 ext prefix routes value
 * @param conf_ipv6_ext_prefix_routes value
 * @param op_type, cps api operation type(create,delete,set etc.)
 * @return standard error
 */
t_std_error nas_sw_profile_conf_ipv6_ext_prefix_routes_set(uint32_t conf_ipv6_ext_prefix_routes,
                                        cps_api_operation_types_t op_type);
/**
 * This function to get ipv6 ext prefix configuration required or not
 * @return true or false
 */
bool nas_sw_profile_is_ipv6_ext_prefix_config_required();

/**
 * This function gets max_ipv6_ext_prefix_routes value
 * @param max_ipv6_ext_prefix_routes[out], pointer to store the value
 * @return standard error
 */
t_std_error nas_sw_profile_max_ipv6_ext_prefix_routes_get(uint32_t *max_ipv6_ext_prefix_routes);

/**
 * This function gets ipv6_ext_prefix_route_blk_size value
 * @param ipv6_ext_prefix_route_blk_size[out], pointer to store the value
 * @return standard error
 */
t_std_error nas_sw_profile_ipv6_ext_prefix_route_lpm_blk_size_get(uint32_t *ipv6_ext_prefix_route_blk_size);

t_std_error nas_sw_acl_profile_num_db_get (uint32_t *num_acl_db);

t_std_error nas_sw_acl_profile_db_get_next(const char *app_group_name,
                                           char *app_group_name_next, uint32_t *num_pools_req);

t_std_error nas_sw_acl_profile_info_get (cps_api_object_t obj);

t_std_error nas_sw_acl_profile_app_group_info_get (const char *app_group_name, bool is_get_all,
                                                   cps_api_object_list_t list, cps_api_qualifier_t qualifier);

t_std_error nas_sw_acl_profile_app_group_info_set (const char *app_group_name, uint32_t next_boot_pool_req,
                                                   cps_api_operation_types_t op_type);

t_std_error nas_switch_upd_acl_profile_info_to_running_cps_db (uint32_t sw_id);

/**
 * This function gets current deep buffer mode value
 * @param cur_deep_buffer_mode[out], pointer to store the value
 * @return standard error
 */
t_std_error nas_sw_profile_cur_deep_buffer_mode_get(bool *cur_deep_buffer_mode);

/**
 * This function gets configured deep buffer mode value
 * @param conf_deep_buffer_mode[out], pointer to store the value
 * @return standard error
 */
t_std_error nas_sw_profile_conf_deep_buffer_mode_get(bool *conf_deep_buffer_mode);

/**
 * This function sets deep buffer mode based on the op_type
 * @param enable, boolean to enable or disable deep buffer mode
 * @return standard error
 */
t_std_error nas_sw_profile_conf_deep_buffer_mode_set(bool enable);

#ifdef __cplusplus
} /* extern C */
#endif
