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
 * \file   nas_base_obj.cpp
 * \brief  NAS CPP Switch and Object Abstract Base classes implementation
 *  Abstracts functionality common to all NAS objects
 * \date   02-2015
 */

#include "nas_base_utils.h"
#include "nas_base_obj.h"
#include "nas_base_ndi_utl.h"
#include "nas_types.h"

nas::base_obj_t::~base_obj_t () {};
nas::base_switch_t::~base_switch_t () {};

void nas::base_obj_t::add_npu (npu_id_t npu_id, bool reset)
{
    const npu_set_t& all_npus = _switch_p->npu_list();

    if (!all_npus.contains (npu_id)) {
        throw nas::base_exception {NAS_BASE_E_PARAM, __PRETTY_FUNCTION__,
                              std::string{"NPU "} + std::to_string (npu_id) +
                              std::string {"does not belong to Table's Logical Switch"}};
    }

    if (!_npu_list_dirty) {

        // Clear for Overwrite
        if (reset) {
            _npus.clear ();
        }
        _npu_list_dirty = true;
    }

    _following_switch_npus = false;
    _npus.add (npu_id);
}

void nas::base_obj_t::commit_create (bool rolling_back)
{
    if (is_created_in_ndi()) {
        throw nas::base_exception {NAS_BASE_E_PARAM, __PRETTY_FUNCTION__,
                              "Cannot create object that is already pushed to NDI"};
    }

    _set_attributes = _dirty_attributes;

    if (_npus.empty()) {
        // If no NPUs are set then copy all NPUs from logical switch
        _following_switch_npus = true;
        _npus = _switch_p->npu_list();
    }

    nas::base_create_obj_ndi (*this, rolling_back);

    clear_all_dirty_flags ();
}

nas::attr_set_t nas::base_obj_t::commit_modify (nas::base_obj_t& obj_old, bool rolling_back)
{
    if (!is_created_in_ndi()) {
        throw nas::base_exception {NAS_BASE_E_PARAM, __PRETTY_FUNCTION__,
                              "Cannot modify object that is not in NDI"};
    }

    _set_attributes += _dirty_attributes;

    if (_npus.empty()) {
        // If all NPUs have been removed then copy all NPUs from logical switch
        _following_switch_npus = true;
        _npus = _switch_p->npu_list();
    }

    nas::base_modify_obj_ndi (*this, obj_old, rolling_back);

    nas::attr_set_t modified_attrs = _dirty_attributes;

    clear_all_dirty_flags ();

    return modified_attrs;
}

void nas::base_obj_t::commit_delete (bool rolling_back)
{
    if (!is_created_in_ndi()) {
        throw nas::base_exception {NAS_BASE_E_PARAM, __PRETTY_FUNCTION__,
                              "Cannot delete object that is not in NDI"};
    }

    nas::base_delete_obj_ndi (*this, rolling_back);

    clear_all_dirty_flags ();
}
