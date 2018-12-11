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
 * nas_base_ut_simple_derived_obj.cpp
 *
 *  Created on: Apr 13, 2015
 */

#include "nas_base_ut_simple_derived_obj.h"

derived_obj_t&  derived_switch_t::get_ut_obj (nas_obj_id_t ut_obj_id)
{
    try {
        return _ut_objs.at (ut_obj_id);
    } 
    catch (std::out_of_range) { 
        throw nas::base_exception {NAS_BASE_E_PARAM, __PRETTY_FUNCTION__,
                                   "No such UT Obj"};
    }
}

void derived_switch_t::add_ut_obj (derived_obj_t& ut_obj)
{
    static nas_obj_id_t new_id = 0;
    ut_obj.set_obj_id (++new_id);
    _ut_objs.insert (std::make_pair (new_id, ut_obj));
    printf ("Added Obj ID %ld to Switch\r\n", new_id); 
}

void derived_switch_t::del_ut_obj (nas_obj_id_t ut_obj_id)
{
    _ut_objs.erase (ut_obj_id);
}

void derived_switch_t::change_ut_obj (derived_obj_t& ut_obj)
{
    _ut_objs.at (ut_obj.obj_id()) = ut_obj;
}
///////////////////
// Example Object class implementation
////////////////// 
derived_obj_t::derived_obj_t (derived_switch_t* switch_p)
            : nas::base_obj_t (switch_p)
{}

bool derived_obj_t::push_leaf_attr_to_npu (nas_attr_id_t attr_id,
                                           npu_id_t npu_id)
{
    t_std_error rc = STD_ERR_OK;

    switch (attr_id)
    {
        case BASE_UT_ATTR1:
            if ((rc = ndi_ut_obj_attr1_set (npu_id, ndi_obj_id(npu_id),
                                            attr1()))
                != STD_ERR_OK)
            {
                throw nas::base_exception {rc, __PRETTY_FUNCTION__,
                                          std::string {"NDI Fail: Set Attr1 in NPU "}
                                         + std::to_string (npu_id)};
            }
            break;
        case BASE_UT_ATTR2:
            if ((rc = ndi_ut_obj_attr2_set (npu_id, ndi_obj_id(npu_id),
                                            attr2()))
                != STD_ERR_OK)
            {
                throw nas::base_exception {rc, __PRETTY_FUNCTION__,
                                          std::string {"NDI Fail: Set Attr2 in NPU "}
                                         + std::to_string (npu_id)};
            }
            break;
        default:
            STD_ASSERT (0);
    }

    return true;
}

void derived_obj_t::set_ndi_obj_id (npu_id_t npu_id,
                                    ndi_obj_id_t id)
{
    _ndi_obj_ids [npu_id] = id;
}

void derived_obj_t::set_attr1 (uint_t attr1)
{
    if (attr1 > MAX_ATTR1_RANGE) {
        throw nas::base_exception {NAS_BASE_E_PARAM, __PRETTY_FUNCTION__,
                              "Invalid value for attr1"};
    }

    _obj.attr1 = attr1;
    mark_attr_dirty (BASE_UT_ATTR1);
}

void derived_obj_t::set_attr2 (uint_t attr2)
{
    if (attr2 > MAX_ATTR2_RANGE) {
        throw nas::base_exception {NAS_BASE_E_PARAM, __PRETTY_FUNCTION__,
                              "Invalid value for attr2"};
    }

    _obj.attr2 = attr2;
    mark_attr_dirty (BASE_UT_ATTR2);
}

void derived_obj_t::commit_create (bool rolling_back)
{
    if (!is_attr_dirty (BASE_UT_ATTR1)) {
        throw nas::base_exception {NAS_BASE_E_PARAM, __PRETTY_FUNCTION__,
            "Mandatory attribute ATTR1 not present"};
    }

    nas::base_obj_t::commit_create(rolling_back);
}

nas::attr_set_t derived_obj_t::commit_modify (base_obj_t& obj_old, bool rolling_back)
{
    if (_obj.attr2 > _obj.attr1) {
        throw nas::base_exception {NAS_BASE_E_PARAM, __PRETTY_FUNCTION__,
            std::string {"Attr2 val "} + std::to_string (_obj.attr2) +
            std::string {" cannot be greater than Attr1 val "} + 
            std::to_string (_obj.attr1)};
    }

    return nas::base_obj_t::commit_modify(obj_old, rolling_back);
}

void* derived_obj_t::alloc_fill_ndi_obj (nas::mem_alloc_helper_t& mem_trakr)
{
    ndi_ut_obj_t* ndi_ut_obj_p = mem_trakr.alloc<ndi_ut_obj_t> (1);

    ndi_ut_obj_p->attr1 = attr1();
    ndi_ut_obj_p->attr2 = attr2();

    return ndi_ut_obj_p;
}

bool derived_obj_t::push_create_obj_to_npu (npu_id_t npu_id, void* ndi_obj)
{
    ndi_obj_id_t ndi_ut_obj_id;
    t_std_error rc;

    auto ndi_ut_obj_p = static_cast<ndi_ut_obj_t*> (ndi_obj);

    if ((rc = ndi_ut_obj_create (npu_id, ndi_ut_obj_p, &ndi_ut_obj_id))
            != STD_ERR_OK)
    {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
                       std::string {"NDI Fail: UT Obj Create Failed for NPU "}
                       + std::to_string (npu_id)};
    }
    // Cache the new Table ID generated by NDI
    set_ndi_obj_id (npu_id, ndi_ut_obj_id);

    return true;
}

bool derived_obj_t::push_delete_obj_to_npu (npu_id_t npu_id)
{
    t_std_error rc;

    if ((rc = ndi_ut_obj_delete (npu_id, ndi_obj_id (npu_id)))
            != STD_ERR_OK)
    {
        throw nas::base_exception {rc, __PRETTY_FUNCTION__,
                       std::string {"NDI Fail: UT Obj Delete Failed for NPU "}
                       + std::to_string (npu_id)};
    }
    reset_ndi_obj_id (npu_id);

    return true;
}

