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

/*!
 * \file   nas_base_ndi_utl.cpp
 * \brief  Multi-NPU NDI programming utilities used by Base Object commit.
 *   Common logic to provision and rollback objects to/from NDI
 *   with multi-NPU support
 * \date   02-2015
 */

#include "event_log.h"
#include "nas_base_utils.h"
#include "nas_base_ndi_utl.h"

namespace nas {

static void _create_obj_npulist_ndi (base_obj_t& obj_new,
                                     const npu_set_t& npu_list,
                                     rollback_trakr_t& r_trakr,
                                     bool rolling_back);

static void _delete_obj_npulist_ndi (base_obj_t& obj_old,
                                     const npu_set_t& npu_list,
                                     rollback_trakr_t& r_trakr,
                                     bool rolling_back);

static void _modify_obj_npulist_ndi (base_obj_t& obj_new,
                                     base_obj_t& obj_old,
                                     const npu_set_t& npu_list,
                                     rollback_trakr_t& r_trakr,
                                     bool rolling_back);

static void _rollback_create_obj_ndi (base_obj_t& obj_new,
                                      rollback_trakr_t& r_trakr);

static void _rollback_delete_obj_ndi (base_obj_t& obj_old,
                                      rollback_trakr_t& r_trakr);

static void _rollback_modify_obj_ndi (base_obj_t& obj_old,
                                      base_obj_t& obj_new,
                                      rollback_trakr_t& r_trakr);

void base_create_obj_ndi (base_obj_t& obj_new, bool rolling_back)

{
    rollback_trakr_t  r_trakr;

    try {

        _create_obj_npulist_ndi (obj_new, obj_new.npu_list(), r_trakr,
                                 rolling_back);

        // Mark that this object has been successfully installed in NDI.
        obj_new.mark_ndi_created();

    } catch (base_exception& e) {

        _rollback_create_obj_ndi (obj_new, r_trakr);
        throw;
    }
}

void base_delete_obj_ndi (base_obj_t& obj_old, bool rolling_back)
{
    rollback_trakr_t r_trakr;

    try {
        _delete_obj_npulist_ndi (obj_old, obj_old.npu_list(), r_trakr,
                                 rolling_back);
    } catch (base_exception& e) {

        _rollback_delete_obj_ndi (obj_old, r_trakr);
        throw;
    }
}

void base_modify_obj_ndi (base_obj_t& obj_new, base_obj_t& obj_old,
                          bool rolling_back)
{
    rollback_trakr_t       r_trakr;
    npu_set_t         npus_added;
    npu_set_t         npus_deleted;
    npu_set_t         npus_unchanged;

    try {
        obj_new.npu_list().compare (obj_old.npu_list(), npus_added, npus_deleted,
                                    npus_unchanged);

        // New NPUs in Modify request
        _create_obj_npulist_ndi (obj_new, npus_added, r_trakr, rolling_back);

        // Deleted NPUs in Modify request
        _delete_obj_npulist_ndi (obj_old, npus_deleted, r_trakr, rolling_back);

        // Existing NPUs in Modify request
        _modify_obj_npulist_ndi (obj_new, obj_old,
                                 npus_unchanged, r_trakr, rolling_back);


    } catch (base_exception& e) {

        _rollback_modify_obj_ndi (obj_old, obj_new, r_trakr);
        throw;
    }
}

static void _create_obj_npulist_ndi (base_obj_t& obj_new,
                                     const npu_set_t& npu_list,
                                     rollback_trakr_t& r_trakr,
                                     bool rolling_back)
{
    mem_alloc_helper_t mem_trakr;

    auto ndi_obj = obj_new.alloc_fill_ndi_obj (mem_trakr);

    for (auto npu_id: npu_list) {
        try {
            bool pushed;
            pushed = obj_new.push_create_obj_to_npu (npu_id, ndi_obj);
            if (!pushed) {
                // Not pushed to this NPU
                continue;
            }

        } catch (base_exception& e) {
            if (rolling_back) {
                EV_LOGGING (NAS_COM, ERR,
                            obj_new.ev_log_mod_name(),
                            "NPU %d: Rollback failed %s ErrCode: %d \n",
                            npu_id, e.err_msg.c_str(), e.err_code);
            } else {
                throw;
            }
        }

        if (!rolling_back) {
            // Upon successful NDI create, start tracking this for rollback
            rollbk_elem_t r_elem {ROLLBK_CREATE_OBJ, npu_id};
            r_trakr.push_back (r_elem);
        }
    }
}

static void _delete_obj_npulist_ndi (base_obj_t& obj_old,
                                     const npu_set_t& npu_list,
                                     rollback_trakr_t& r_trakr,
                                     bool rolling_back)
{
    int npu_count = 0;

    for (auto npu_id: npu_list) {
        try {
            bool pushed;
            pushed = obj_old.push_delete_obj_to_npu (npu_id);
            if (!pushed ) {
                // Not pushed to this NPU
                continue;
            }

        } catch (base_exception& e) {
            if (rolling_back) {
                EV_LOGGING (NAS_COM, ERR,
                            obj_old.ev_log_mod_name(),
                            "Rollback failed: NPU %d: %s ErrCode: %d \n",
                            npu_id, e.err_msg.c_str(), e.err_code);
            } else {
                throw;
            }
        }

        if (!rolling_back) {
            // Upon successful NDI delete, start tracking this for rollback
            rollbk_elem_t r_elem {ROLLBK_DELETE_OBJ, npu_id};
            r_trakr.push_back (r_elem);
        }

        npu_count++;
    }
}


static void _modify_obj_npulist_ndi (base_obj_t& obj_new,
                                     base_obj_t& obj_old,
                                     const npu_set_t& npu_list,
                                     rollback_trakr_t& r_trakr,
                                     bool rolling_back)
{
    // Existing NPUs in Modify request
    for (auto attr_id: obj_new.dirty_attr_list ()) {

        if (obj_new.is_leaf_attr (attr_id)) {

            for (auto npu_id:  npu_list) {

                try {
                    if (!obj_new.push_leaf_attr_to_npu (attr_id, npu_id))
                        // Not pushed to this NPU
                        continue;

                } catch (base_exception& e) {
                    if (rolling_back) {
                        EV_LOGGING (NAS_COM, ERR,
                                    obj_new.ev_log_mod_name(),
                                    "Rollback failed: NPU %d: %s ErrCode: %d \n",
                                    npu_id, e.err_msg.c_str(), e.err_code);
                    } else {
                        throw;
                    }
                }

                if (!rolling_back) {
                    // Upon successful NDI modification, start tracking this
                    // for rollback
                    rollbk_elem_t r_elem {ROLLBK_MODIFY_ATTR, npu_id,
                        {attr_id}};
                    r_trakr.push_back (r_elem);
                }
            }
        } else {
            obj_new.push_non_leaf_attr_ndi (attr_id, obj_old,
                                            npu_list, r_trakr, rolling_back);
        }
    }
}

static void _rollback_create_obj_in_npu (base_obj_t& obj_new, npu_id_t npu_id)
{
    try {
        obj_new.push_delete_obj_to_npu (npu_id);

    } catch (base_exception& ne) {
        EV_LOGGING (NAS_COM, ERR,
                    obj_new.ev_log_mod_name(),
                    "Rollback failed: %s: ErrCode %d\n",
                    ne.err_msg.c_str(), ne.err_code);
    }
}

static void _rollback_delete_obj_in_npu (base_obj_t& obj_old, npu_id_t npu_id,
                                         void* ndi_obj)
{
    try {
        obj_old.push_create_obj_to_npu (npu_id, ndi_obj);

    } catch (base_exception& ne) {
        EV_LOGGING (NAS_COM, ERR,
                    obj_old.ev_log_mod_name(),
                    "Rollback failed: %s: ErrCode %d\n",
                    ne.err_msg.c_str(), ne.err_code);
    }
}

static void _rollback_modify_attr_in_npu (base_obj_t& obj_old,
                                          nas_attr_id_t  attr_id,
                                          npu_id_t npu_id)
{
    try {
        obj_old.push_leaf_attr_to_npu (attr_id, npu_id);

    } catch (base_exception& ne) {
        EV_LOGGING (NAS_COM, ERR,
                    obj_old.ev_log_mod_name(),
                    "Rollback failed: %s: ErrCode %d\n",
                    ne.err_msg.c_str(), ne.err_code);
    }
}

static void _rollback_create_obj_ndi (base_obj_t& obj_new,
                                      rollback_trakr_t& r_trakr)
{
    // Reverse Loop thru the rollback tracker list
    // and handle each rollback element
    while (r_trakr.size()>0) {
        auto& r_elem = r_trakr.back();

        STD_ASSERT (r_elem.rlbk_type == ROLLBK_CREATE_OBJ);
        _rollback_create_obj_in_npu (obj_new, r_elem.npu_id);

        r_trakr.pop_back();
    }
}

static void _rollback_delete_obj_ndi (base_obj_t& obj_old,
                                      rollback_trakr_t& r_trakr)
{
    mem_alloc_helper_t mem_trakr;

    auto ndi_obj = obj_old.alloc_fill_ndi_obj (mem_trakr);

    // Reverse Loop thru the rollback tracker list
    // and handle each rollback element
    while (r_trakr.size()>0) {
        auto& r_elem = r_trakr.back();

        STD_ASSERT (r_elem.rlbk_type == ROLLBK_DELETE_OBJ);
        _rollback_delete_obj_in_npu (obj_old, r_elem.npu_id, ndi_obj);

        r_trakr.pop_back();
    }
}

static void _rollback_modify_obj_ndi (base_obj_t& obj_old,
                                      base_obj_t& obj_new,
                                      rollback_trakr_t& r_trakr)
{
    void *ndi_obj = NULL;
    mem_alloc_helper_t mem_trakr;

    // Reverse Loop thru the rollback tracker list
    // and handle each rollback element
    while (r_trakr.size()>0) {
        auto& r_elem = r_trakr.back();

        switch (r_elem.rlbk_type)
        {
            case ROLLBK_CREATE_OBJ:
                // Delete new table from NDI for NPU
                _rollback_create_obj_in_npu (obj_new, r_elem.npu_id);
                break;
            case ROLLBK_DELETE_OBJ:
                // Create old table back in NDI for NPU
                if (!ndi_obj) ndi_obj = obj_old.alloc_fill_ndi_obj (mem_trakr);
                _rollback_delete_obj_in_npu (obj_old, r_elem.npu_id, ndi_obj);
                break;
            case ROLLBK_MODIFY_ATTR:
                // Restore old object back in NDI
                if (r_elem.attr_hierarchy.size() == 1) {
                    _rollback_modify_attr_in_npu (obj_old,
                                                  r_elem.attr_hierarchy[0],
                                                  r_elem.npu_id);
                } else {
                    obj_old.rollback_non_leaf_attr_in_npu (r_elem.attr_hierarchy,
                                                           r_elem.npu_id,
                                                           obj_new);
                }
                break;
            case ROLLBK_CREATE_ATTR:
                // Restore old object back in NDI
                obj_new.rollback_create_attr_in_npu (r_elem.attr_hierarchy,
                                                     r_elem.npu_id);
                break;
            case ROLLBK_DELETE_ATTR:
                obj_old.rollback_delete_attr_in_npu (r_elem.attr_hierarchy,
                                                     r_elem.npu_id);
                break;
        }
        r_trakr.pop_back();
    }
}

}
