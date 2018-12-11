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
 * \file   nas_base_utils.h
 * \brief  NAS CPP Utility Classes
 * \date   02-2015
 */

#ifndef _NAS_BASE_UTILS_H_
#define _NAS_BASE_UTILS_H_

#include "std_error_codes.h"
#include "event_log.h"
#include "std_type_defs.h"
#include "std_bit_masks.h"
#include "ds_common_types.h"
#include "nas_types.h"
#include <set>
#include <string>
#include <vector>
#include <algorithm>
#include <unordered_map>

namespace nas {

/*!
 * \class npu_set_t
 * \brief Collection of unique set of NPUs
 *
 * This class is NOT thread-safe. It is left to the users of this class.
 */
class npu_set_t
{
    public:
        typedef std::set<npu_id_t> npu_internal_set_t;
        typedef npu_internal_set_t::iterator npu_iter_t;
        typedef npu_internal_set_t::const_iterator const_npu_iter_t;

        void add (npu_id_t npu_id) { _npus.insert (npu_id); }
        void clear () { _npus.clear (); }
        void del (npu_id_t npu_id) {_npus.erase (npu_id);}

        bool empty () const {return _npus.empty();}
        size_t size () const {return _npus.size();}
        npu_iter_t begin () {return _npus.begin();}
        npu_iter_t end () {return _npus.end();}
        const_npu_iter_t begin () const {return _npus.begin();}
        const_npu_iter_t end () const {return _npus.end();}

        bool contains (npu_id_t npu) const {
            return (_npus.find (npu) != end());
        }
        // Copy NPU list as a string to str
        void dump (std::string& str) const;

        // Compare npus in my list with npus in the list
        // passed in as second
        void compare (const npu_set_t& second,
                      npu_set_t& only_mine,
                      npu_set_t& only_second,
                      npu_set_t& in_both) const;
    private:
        npu_internal_set_t _npus;
};

/*!
 * \class mem_alloc_helper_t
 * \brief Helper class to track and free heap memory allocation.
 *
 * An Object of this class owns pointers to the memory that it tracks
 * and frees them when the helper object is destroyed.
 *
 * Typically used when there is need to return variable length data
 * back up the function call stack.
 * The mem_alloc_helper object is passed down to the callee function
 * which allocates the heap memory for the variable length data.
 * The helper object can be declared on the stack up in the
 * caller function that consumes this variable length data so that the
 * memory is freed after the caller function goes out of scope.
 *
 * No copy or move of helper objects is allowed since it owns pointers
 * This class is NOT thread-safe. It is left to the users of this class.
 *
 * Thread safety is not an issue since this object is used on the stack.
 *
 */
class mem_alloc_helper_t
{
    public:
        mem_alloc_helper_t () {};
        ~mem_alloc_helper_t ()
        {
            for (auto ptr: ptr_list) {
                EV_LOGGING (NAS_COM, DEBUG, "MEM-ALLOC-HELPER",
                            "Free %p\r\n", ptr);
                free (ptr);
            }
        }
        /* No copy or move constructor or assignment allowed
         * since objects of this class own pointers */
        mem_alloc_helper_t (const mem_alloc_helper_t&) = delete;
        mem_alloc_helper_t& operator= (const mem_alloc_helper_t&) = delete;
        mem_alloc_helper_t (mem_alloc_helper_t&&) = delete;
        mem_alloc_helper_t& operator= (mem_alloc_helper_t&&) = delete;

        /*! Allocate heap memory and track pointer for freeing */
        template <typename T> T* alloc (size_t count)
        {
            T* ptr = (T*) calloc (sizeof (T), count);
            if (ptr == NULL) throw std::bad_alloc{};
            EV_LOGGING (NAS_COM, DEBUG, "MEM-ALLOC-HELPER",
                        "Alloc %p\r\n", ptr);
            try {
                ptr_list.push_back (ptr);
            } catch (...) {
                free (ptr);
                throw;
            }
            return ptr;
        }

        /*! Add previous externally allocated heap memory to tracker list
         *  for freeing */
        void add (void* ptr) {ptr_list.push_back (ptr);}
    private:
        std::vector<void*> ptr_list;
};

/*!
 * \class attr_set_t
 * \brief Collection of unique set of attribute IDs
 *
 *  This class is NOT thread-safe. It is left to the users of this class.
 */
class attr_set_t
{
    public:
        typedef std::set<nas_attr_id_t> attr_internal_set_t;
        typedef attr_internal_set_t::iterator attr_set_iter_t ;
        typedef attr_internal_set_t::const_iterator const_attr_set_iter_t;

        void add (nas_attr_id_t attr_id);
        void clear ();

        size_t size () const {return _attributes.size();}
        attr_set_iter_t begin () {return _attributes.begin();}
        attr_set_iter_t end () {return _attributes.end();}
        const_attr_set_iter_t begin () const {return _attributes.begin();}
        const_attr_set_iter_t end () const {return _attributes.end();}

        bool contains (nas_attr_id_t attr_id) const {
            return (_attributes.find (attr_id) != _attributes.end());
        }

        /*! Max Attribute ID in this attribute set. */
        size_t len() const {return _max_attr_id;}
        size_t count() const {return _attributes.size();}

        /*! Convert the attribute set to an array
         *  Caller has to pass in pre-allocated memory
         * \param[in,out] attr_array pre-allocated buffer which needs
         *                           to be filled
         * \param[in] array_len  length of the pre-allocated buffer
         */
        void to_array (nas_attr_id_t attr_array[], size_t in_array_len) const;

        /*! Convert the attribute set to an array
         *  This method allocates memory
         * \param[out] attr_array pointer to array allocated in this method
         * \param[out] array_len  length of the array allocated in this method
         * \param[in] mem_guard - used by caller to track freeing of memory
         *                    allocated in this method.
         */
        void to_array (nas_attr_id_t** attr_ptr, size_t* out_array_len,
                       mem_alloc_helper_t& mem_guard) const;

        /*! Union of 2 attribute sets */
        attr_set_t& operator+= (const attr_set_t& rhs);

    private:
        nas_attr_id_t               _max_attr_id = 0;
        attr_internal_set_t         _attributes;
};

/*!  Collection of non-unique list of attribute IDs */
typedef std::vector<nas_attr_id_t> attr_list_t;
typedef std::vector<hal_ifindex_t> ifindex_list_t;
typedef ifindex_list_t::iterator ifindex_iter_t;

/*!  NAS common CPP Exception */
struct base_exception
{
    t_std_error     err_code;
    std::string     err_fn;
    std::string     err_msg;
};

/*!  Type of NDI change that is being tracked for rollback */
enum rollbk_type_t {
    ROLLBK_CREATE_OBJ,
    ROLLBK_DELETE_OBJ,
    // For modify Object there will be separate rollback entries
    // for each attribute
    ROLLBK_MODIFY_ATTR,
    // For embedded attributes
    ROLLBK_CREATE_ATTR,
    ROLLBK_DELETE_ATTR,
};

/*! Details of each change committed to NDI being tracked for rollback */
struct rollbk_elem_t
{
    rollbk_type_t   rlbk_type;
    npu_id_t        npu_id;
    attr_list_t     attr_hierarchy; // Valid only for Modify
};

/*! Tracking list of things to be rolled back from NDI */
typedef std::vector<rollbk_elem_t> rollback_trakr_t;

/*! \class id_generator_t
 *  \brief Generate free IDs for NAS objects
 *
 *  Objects of this type cannot be copied. However Move is allowed.
 *  This class is NOT thread-safe. It is left to the users of this class.
 */
class id_generator_t
{
    public:
        id_generator_t (size_t max_ids);
        ~id_generator_t () noexcept
        {
            std_bitmaparray_free_data (_id_bitmap);
        }

        // Override the default copy and move
        // constructors/assignment operators
        // since objects of this class own naked pointer

        // Copying is not allowed
        id_generator_t (const id_generator_t& copy) = delete;
        id_generator_t& operator= (const id_generator_t& copy) = delete;

        // Moving is allowed
        id_generator_t (id_generator_t&& move) noexcept;
        id_generator_t& operator= (id_generator_t&& move) noexcept;

        /*! Find an unused ID and mark it as used
         * \return unused ID identified */
        nas_obj_id_t   alloc_id ();

        /*! Mark given ID as used.
         * \param[in] id - ID to be reserved
         * \return         False if already being used */
        bool           reserve_id (nas_obj_id_t id) noexcept;

        /*! Release given ID to unused list */
        void           release_id (nas_obj_id_t id) noexcept;

    private:
        const size_t _max_ids;
        size_t _pos = 1;
        void* _id_bitmap = NULL;
};

} // End namespace nas

#endif
