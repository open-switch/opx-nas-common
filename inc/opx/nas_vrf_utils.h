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
 * nas_vrf_utils.h
 */

#ifndef _NAS_VRF_UTILS_H_
#define _NAS_VRF_UTILS_H_

#include "std_error_codes.h"
#include "ds_common_types.h"
#include "nas_types.h"

#ifdef __cplusplus
extern "C++" {
#include <vector>
}
#endif

#ifdef __cplusplus
extern "C" {
#endif

#define NAS_VRF_NAME_SZ                32
#define NAS_DEFAULT_VRF_NAME           "default"
#define NAS_MGMT_VRF_NAME              "management"
#define NAS_DEFAULT_VRF_ID             0

/* Max. VRFs supported in the system  - 1024 (Default VRF (0) + 1023 DATA VRFs) */
#define NAS_MIN_VRF_ID                 NAS_DEFAULT_VRF_ID
#define NAS_MAX_DATA_VRF_ID            1023 /* 1 to 1023 (1023 DATA VRFs excluding default VRF) */
#define NAS_MGMT_VRF_ID                (NAS_MAX_DATA_VRF_ID + 1) /* VRF-id 1024 is reserved for mgmt VRF. */
#define NAS_MAX_VRF_ID                 NAS_MGMT_VRF_ID

/* VRF operation add/del */
typedef enum {
    NAS_VRF_OP_ADD = 1,
    NAS_VRF_OP_DEL,
    NAS_VRF_OP_UPD
} nas_vrf_op_t;

typedef struct _nas_vrf_ctrl_s{
    char vrf_name[NAS_VRF_NAME_SZ+1]; /* VRF name */
    hal_vrf_id_t vrf_int_id; /* Internal VRF-id mapped with VRF-name for faster look-up */
    nas_obj_id_t vrf_id; /* VRF OID given by NPU */
}nas_vrf_ctrl_t;

/*!
 *  Function to get the VRF object id from a given VRF name.
 *  \param vrf_name [in] Vrf_name
 *  \param vrf_info [out] returns the VRF object id
 *  \return              std_error
 */
t_std_error nas_get_vrf_obj_id_from_vrf_name(const char *vrf_name, nas_obj_id_t *obj_id);

/*!
 *  Function to get the VRF id from a given VRF name.
 *  \param vrf_name [in] Vrf_name
 *  \param vrf_info [out] returns the VRF id
 *  \return              std_error
 */
t_std_error nas_get_vrf_internal_id_from_vrf_name(const char *vrf_name, hal_vrf_id_t *vrf_id);

/*!
 *  Function to get the VRF ctrl block from a given VRF name.
 *  \param vrf_name [in] Vrf_name
 *  \param vrf_ctrl_blk [out] returns the VRF ctrl block
 *  \return              std_error
 */
t_std_error nas_get_vrf_ctrl_from_vrf_name(const char *vrf_name, nas_vrf_ctrl_t *vrf_ctrl_blk);

/*!
 *  Function to add/del the VRF information for the VRF name.
 *  \param op [in] VRF add/del operation.
 *  vrf_info must carry vrf_name in order to get the VRF info.
 *  \param vrf_info [out] returns the VRF information
 *  \return              std_error
 */
t_std_error nas_update_vrf_info (nas_vrf_op_t op, nas_vrf_ctrl_t *vrf_info);

#ifdef __cplusplus
}
#endif

#ifdef __cplusplus
extern "C++" {
/*!
 *  Function to return all vrf ctrl block
 *  \param op [in] vector to hold pointers to vrf ctrl block
 *  \return              none
 */
void nas_get_all_vrf_ctrl(std::vector<nas_vrf_ctrl_t> &v);
}
#endif

#endif /* _NAS_VRF_UTILS_H_ */
