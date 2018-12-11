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
 * filename: nas_types.h
 */


/*!
 * Common NAS types
 */

#ifndef _NAS_TYPES_H_
#define _NAS_TYPES_H_


#include "ds_common_types.h"
#include "std_error_codes.h"
#include "std_error_ids.h"
#include "cps_api_object_attr.h"
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** ID type for any NAS object returned to the CPS application */
typedef uint64_t nas_obj_id_t;

typedef uint32_t nas_switch_id_t;

/** ID type for any NAS attribute */
typedef cps_api_attr_id_t nas_attr_id_t;

typedef uint64_t  ndi_obj_id_t;

typedef nas_obj_id_t lag_id_t;

typedef ndi_obj_id_t nas_bridge_id_t;
#define NAS_INVALID_BRIDGE_ID  ((nas_bridge_id_t)~0x0)

/** NAS CPS handler common Error codes */
#define    NAS_BASE_E_NONE           (int)STD_ERR_OK
#define    NAS_BASE_E_MEM            (int)STD_ERR (HALCOM, NOMEM, 0)
#define    NAS_BASE_E_FAIL           (int)STD_ERR (HALCOM, FAIL, 1) // Runtime failure (eg: NDI provisioning)
#define    NAS_BASE_E_FULL           (int)STD_ERR (HALCOM, FAIL, 2)
#define    NAS_BASE_E_CREATE_ONLY    (int)STD_ERR (HALCOM, PARAM, 1) // Create Only param
#define    NAS_BASE_E_PARAM          (int)STD_ERR (HALCOM, PARAM, 2) // Wrong value for param
#define    NAS_BASE_E_UNSUPPORTED    (int)STD_ERR (HALCOM, PARAM, 3)
#define    NAS_BASE_E_DUPLICATE      (int)STD_ERR (HALCOM, PARAM, 4) // Attribute duplicated in CPS object

#ifdef __cplusplus
}
#endif
#endif /* NAS_TYPES_H_ */
