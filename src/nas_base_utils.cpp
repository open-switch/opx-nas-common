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
 * \file   nas_base_utils.cpp
 * \brief  NAS CPP Utility Classes implementation
 * \date   02-2015
 */

#include "nas_base_utils.h"
#include "nas_types.h"
#include <string>

/////////// attr_set_t implementation ///////////
//
void nas::attr_set_t:: add (nas_attr_id_t attr_id)
{
    _attributes.insert (attr_id);
    if (attr_id > _max_attr_id) _max_attr_id = attr_id;
}

void nas::attr_set_t::clear ()
{
    _attributes.clear ();
    _max_attr_id = 0;
}

nas::attr_set_t& nas::attr_set_t::operator+= (const nas::attr_set_t& rhs)
{
    for (auto attr_id: rhs) {
        _attributes.insert (attr_id);
    }
    _max_attr_id = (_max_attr_id > rhs._max_attr_id)
        ? _max_attr_id:rhs._max_attr_id;
    return *this;
}

void nas::attr_set_t::to_array (nas_attr_id_t attr_array[],
                                size_t in_array_len) const
{
    size_t count = 0;
    for (auto attr_id: _attributes) {
        if (count >= in_array_len) break;
        attr_array [count++] = attr_id;
    }
}

void nas::attr_set_t::to_array (nas_attr_id_t** attr_ptr,
                                size_t* out_array_len,
                                mem_alloc_helper_t& mem_guard) const
{
    auto attr_array = mem_guard.alloc<nas_attr_id_t> (_attributes.size());
    size_t count = 0;
    for (auto attr_id: _attributes) {
        attr_array [count++] = attr_id;
    }
    *out_array_len = count;
    *attr_ptr = attr_array;
}

/////////// npu_set_t implementation ///////////
//
void nas::npu_set_t::compare (const nas::npu_set_t& second,
                              nas::npu_set_t& only_mine,
                              nas::npu_set_t& only_second,
                              nas::npu_set_t& in_both) const
{
    for (auto npu_id: second) {
        if (!contains (npu_id)) {
            only_second.add (npu_id);
        } else {
            in_both.add (npu_id);
        }
    }

    for (auto npu_id: _npus) {
        if (!second.contains (npu_id)) {
            only_mine.add (npu_id);
        }
    }
}

void nas::npu_set_t::dump (std::string& str) const
{
    str += "NPUs: ";
    for (auto npu_id: _npus) {
        str += (std::to_string (npu_id) + ", ");
    }
}

/////////// id_generator_t implementation ///////////
//
nas::id_generator_t::id_generator_t (size_t max_ids)
            :_max_ids (max_ids+1)
{
    _id_bitmap = std_bitmap_create_array (_max_ids);
    if (!_id_bitmap) {
        throw std::bad_alloc{};
    }
}

nas_obj_id_t nas::id_generator_t::alloc_id ()
{
    int freepos;
    if (_pos < _max_ids) {
        if ((freepos = std_find_first_bit(_id_bitmap, _max_ids, _pos)) != -1) {
            if (freepos < (int) _max_ids) {
                _pos = freepos;
                STD_BIT_ARRAY_CLR ((uint8_t*)_id_bitmap, _pos);
                return _pos++;
            }
        }
    }
    _pos = 1;
    if ((freepos = std_find_first_bit(_id_bitmap, _max_ids, _pos)) != -1) {
        if (freepos < (int) _max_ids) {
            _pos = freepos;
            STD_BIT_ARRAY_CLR ((uint8_t*)_id_bitmap, _pos);
            return _pos++;
        }
    }
    throw nas::base_exception {NAS_BASE_E_FULL, __PRETTY_FUNCTION__, "No more free IDs"};
}

bool nas::id_generator_t::reserve_id (nas_obj_id_t id) noexcept
{
    if (id >= _max_ids) {
        return false;
    }
    if (STD_BIT_ARRAY_TEST ((uint8_t*)_id_bitmap, id)) {
        STD_BIT_ARRAY_CLR ((uint8_t*)_id_bitmap, id);
        return true;
    }
    return false;
}

void nas::id_generator_t::release_id (nas_obj_id_t id) noexcept
{
    if (id < _max_ids) {
        STD_BIT_ARRAY_SET ((uint8_t*)_id_bitmap, id);
    }
}

nas::id_generator_t::id_generator_t (nas::id_generator_t&& move) noexcept
    :_max_ids (move._max_ids), _pos (move._pos)
{
    _id_bitmap = move._id_bitmap;
    move._id_bitmap = NULL;
}


nas::id_generator_t& nas::id_generator_t::operator= (nas::id_generator_t&& move) noexcept
{
    _pos = move._pos;
    _id_bitmap = move._id_bitmap;
    move._id_bitmap = NULL;
    return *this;
}
