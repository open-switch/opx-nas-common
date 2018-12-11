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

/**
 * filename: nas_ndi_obj_id_table.cpp
 **/

/*
 * nas_ndi_obj_id_table.cpp
 */

#include "nas_ndi_obj_id_table.h"
#include "event_log.h"
#include "nas_com_log.h"
#include <vector>

extern "C" {

nas_ndi_obj_id_table_handle_t nas_ndi_obj_id_table_create ()
{
    return new (std::nothrow) nas::ndi_obj_id_table_t;
}

void nas_ndi_obj_id_table_delete (nas_ndi_obj_id_table_handle_t h)
{
    auto table_p = (nas::ndi_obj_id_table_t *) h;
    delete table_p;
}

bool  nas_ndi_obj_id_table_set_id (const nas_ndi_obj_id_table_handle_t h,
                                    npu_id_t  npu_id,
                                    ndi_obj_id_t  ndi_obj_id)
{
    auto& table = *(nas::ndi_obj_id_table_t *)h;

    try {
        table[npu_id] = ndi_obj_id;
    } catch (...) {
        return false;
    }

    return true;
}

bool  nas_ndi_obj_id_table_erase_id (nas_ndi_obj_id_table_handle_t h,
                                     npu_id_t  npu_id)
{
    auto& table = *(nas::ndi_obj_id_table_t *)h;

    if (table.erase (npu_id) != 0) {
        return true;
    }
    return false;
}

bool  nas_ndi_obj_id_table_get_id (const nas_ndi_obj_id_table_handle_t h,
                                   npu_id_t  npu_id,
                                   ndi_obj_id_t* ndi_obj_id_p)
{
    auto& table = *(nas::ndi_obj_id_table_t *)h;

    try {
        *ndi_obj_id_p = table.at (npu_id);
    } catch (...) {
        return false;
    }
    return true;
}

bool nas_ndi_obj_id_table_cps_serialize (const nas_ndi_obj_id_table_handle_t h,
                                         cps_api_object_t cps_obj,
                                         cps_api_attr_id_t *attr_id_list,
                                         size_t attr_id_size)
{
    auto& table = *(nas::ndi_obj_id_table_t *)h;
    return nas::ndi_obj_id_table_cps_serialize (table, cps_obj, attr_id_list, attr_id_size);
}

bool
nas_ndi_obj_id_table_cps_unserialize (nas_ndi_obj_id_table_handle_t  h,
                                      cps_api_object_t cps_obj,
                                      cps_api_attr_id_t *attr_id_list,
                                      size_t attr_id_size)
{
    auto& table = *(nas::ndi_obj_id_table_t *)h;
    return nas::ndi_obj_id_table_cps_unserialize (table, cps_obj, attr_id_list, attr_id_size);
}


} /* End Extern C */

/* Internal Attribute IDs used to encode the opaque data in the CPS object */
static constexpr cps_api_attr_id_t OPAQUE_INST_ATTR_ID_START = 1;
static constexpr cps_api_attr_id_t OPAQUE_NPU_ATTR_ID = 1;
static constexpr cps_api_attr_id_t OPAQUE_NDI_OBJ_ATTR_ID = 2;

// C++ Serialize and  Unserialize APIs
bool nas::ndi_obj_id_table_cps_serialize (const ndi_obj_id_table_t& table,
                                          cps_api_object_t cps_obj,
                                          cps_api_attr_id_t *attr_id_list,
                                          size_t attr_id_size)
{
    std::vector<cps_api_attr_id_t> internal_ids (attr_id_list,
                                                 attr_id_list+attr_id_size);

    cps_api_attr_id_t  inst_attr_id = OPAQUE_INST_ATTR_ID_START;

    for (auto map_elem: table) {
        // Insert the List index in the CPS object
        internal_ids.push_back (inst_attr_id);

        // Insert the NPU ID in the CPS object
        internal_ids.push_back (OPAQUE_NPU_ATTR_ID);
        auto npu_id = map_elem.first;
        if (!cps_api_object_e_add(cps_obj, internal_ids.data(), internal_ids.size(),
                             cps_api_object_ATTR_T_U32, &npu_id, sizeof (npu_id))) {
            return false;
        }
        internal_ids.pop_back ();

        // Insert the NDI Obj ID in the CPS object
        internal_ids.push_back (OPAQUE_NDI_OBJ_ATTR_ID);
        auto ndi_obj_id = map_elem.second;
        NAS_COM_TRACE ("Serialize [Inst %lu] NPU-ID %d : NDI-ID 0x%lx\n",
                       inst_attr_id, npu_id, ndi_obj_id);

        if (!cps_api_object_e_add(cps_obj, internal_ids.data(), internal_ids.size(),
                                  cps_api_object_ATTR_T_U64, &ndi_obj_id,
                                  sizeof (ndi_obj_id))) {
            return false;
        }
        internal_ids.pop_back ();

        internal_ids.pop_back ();
        inst_attr_id++;
    }
    return true;
}

bool nas::ndi_obj_id_table_cps_unserialize (ndi_obj_id_table_t&  table,
                                            cps_api_object_t cps_obj,
                                            cps_api_attr_id_t *attr_id_list,
                                            size_t attr_id_size)
{
    cps_api_object_it_t it_list;

    if (!cps_api_object_it(cps_obj, attr_id_list, attr_id_size, &it_list)) {
        // Opaque data not found
        NAS_COM_ERR ("Could not find opaque attribute \n");
        return false;
    }

    int inst = 0;
    nas::ndi_obj_id_table_t  temp_table;

    for (cps_api_object_it_inside (&it_list); cps_api_object_it_valid(&it_list);
         cps_api_object_it_next (&it_list), inst++) {

        npu_id_t npu_id = 0;
        ndi_obj_id_t ndi_obj_id = 0;

        NAS_COM_TRACE ("Unserializing entry %d\n", inst+1);

        cps_api_object_it_t it_map_entry = it_list;

        for (cps_api_object_it_inside (&it_map_entry); cps_api_object_it_valid (&it_map_entry);
             cps_api_object_it_next (&it_map_entry)) {

            switch (cps_api_object_attr_id (it_map_entry.attr)) {
                case OPAQUE_NPU_ATTR_ID:
                    npu_id = cps_api_object_attr_data_u32 (it_map_entry.attr);
                    NAS_COM_TRACE ("NPU ID %d\n", npu_id);
                    break;
                case OPAQUE_NDI_OBJ_ATTR_ID:
                    ndi_obj_id = cps_api_object_attr_data_u64 (it_map_entry.attr);
                    NAS_COM_TRACE ("NDI Obj ID 0x%lx\n", ndi_obj_id);
                    break;
            }
        }
        temp_table[npu_id] = ndi_obj_id;
    }

    table = std::move (temp_table);
    return true;
}
