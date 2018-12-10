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
 * \file   nas_base_obj.h
 * \brief  NAS CPP Switch and Object Abstract Base classes
 *  Abstracts functionality common to all NAS objects
 * \date   02-2015
 */

#ifndef _NAS_BASE_OBJ_H_
#define _NAS_BASE_OBJ_H_

#include "std_assert.h"
#include "event_log_types.h"
#include "nas_types.h"
#include "nas_base_utils.h"

#include <unordered_map>

namespace nas {

/*!
 * \class base_switch_t
 * \brief Abstract Base class representing NAS logical Switch
 *
 *  Maintains the Switch ID and the list of NPUs in a logical switch.
 *  Can be inherited to a module specific switch class to implement
 *  module specific switch data.
 *  Referred by pointer from the Base Object class.
 *
 *  This class is NOT  thread-safe.
 *  It is left to the users of this class
 *
 * \see   base_obj_t
 */
class base_switch_t
{
    public:
        ///// Constructor ////
        base_switch_t (nas_switch_id_t switch_id)
            : _switch_id{switch_id} {};
        virtual ~base_switch_t () = 0;

        ///////// Accessors ///////
        nas_switch_id_t           id() const {return _switch_id;}
        size_t             num_npus () const {return _npus.size();}
        const npu_set_t&   npu_list () const {return _npus;}

        ///////// Modifiers ///////
        // Add NPU to set of NPUs in this switch
        void add_npu (npu_id_t npu_id) {_npus.add (npu_id);}

    private:
        // Switch ID has to be supplied as initializer
        // when the switch object is created and cannot be changed later
        const nas_switch_id_t   _switch_id;
        npu_set_t       _npus;
};

/*!
 * \class base_obj_t
 * \brief Abstract Base class for all NAS CPS-API objects.
 *
 * Implements common functionality like -
 *  - Track list of changed attributes in a transaction.
 *  - Maintain list of NPUs to which this Object belongs.
 *  - Provide virtual base implementations for Commit and Rollback.
 *
 * Can be inherited and overridden to implement CPS object specific
 * commit and rollback functionality
 *
 *  This class is NOT  thread-safe.
 *  It is left to the users of this class
 *
 */
class base_obj_t
{
    public:
        ////// Constructor/Destructor /////
        base_obj_t (base_switch_t* switch_p) : _switch_p (switch_p) {};
        virtual ~base_obj_t () = 0;

        ///////////    Accessors    ///////////
        nas_switch_id_t       switch_id() const;
        base_switch_t&        get_switch() const {return *_switch_p;}

        bool                  is_created_in_ndi () const;

        // Permanent list of all attributes set/modified in this object
        const attr_set_t&     set_attr_list () const;

        // Temp list of attributes changed in a single CPS request
        // Reset at the end of every commit
        const attr_set_t&     dirty_attr_list () const;
        bool                  is_attr_dirty (nas_attr_id_t attr_id) const;

        // Track change in NPU list in a given transaction
        bool                  is_npu_list_dirty () const;

        ///////////          Virtual getters        ////////////
        //    Can be overridden for more specific behavior   //

        // List of NPUs to which this object will be pushed.
        // Used by commit() to loop through the NPUs.
        // Be default will return the NPUs added for this object
        // using add_npu() or all the NPUs from the logical switch.
        // Override Base version if more specific NPUs are needed
        virtual const npu_set_t&    npu_list () const {return _npus;}
        // Does this object have all NPUs of the logical switch
        // to which it belongs ?
        virtual bool following_switch_npus () const {return _following_switch_npus;}


        ////////     Modifiers    ////////
        void      clear_all_dirty_flags ();
        void      mark_attr_dirty (nas_attr_id_t attr_id);

        void      mark_ndi_created () {_ndi_created = true;}
        void      mark_ndi_removed () {_ndi_created = false;}

        ///////     Virtual functions to change NPU set     ////////
        /*! Add NPU to the set of NPUs, object needs to be pushed to
         * Marks the NPU list dirty.
         * Resets the original NPU set if the NPU dirty flag was
         * false coming into this function.
         * Pass param reset as false to avoid resetting prev NPU set. */
        virtual void      add_npu (npu_id_t npu_id, bool reset=true);

        virtual void      reset_npus () {_npus.clear ();}

        //////   Virtual functions called when Object   //////////
        /////            is Committed to NDI            //////////
        ///////  Override for object specific behavior  ////////

        /// Commit newly created object to NDI
        /// If NPU list is empty populates NPUs from logical switch
        /// Override Base version to perform object specific validations
        /// for mandatory attributes etc
        virtual void        commit_create (bool rolling_back);

        /// Commit all modified attributes in object to NDI.
        /// Returns the set of attributes modified in this transaction.
        virtual attr_set_t  commit_modify (base_obj_t& obj_old,
                                           bool rolling_back);
        /// Commit deleted object to NDI
        virtual void        commit_delete (bool rolling_back);

        /// Logging helper routines
        virtual const char* name () const = 0;
        virtual e_event_log_types_enums ev_log_mod_id () const = 0;
        virtual const char* ev_log_mod_name () const = 0;

        /// Allocate and fill NDI object to be reused for NDI push to each NPU.
        /// Override to avoid constructing NDI objects
        /// separately for each NPU if they are all identical
        /// NDI Object constructed here will be passed to the
        /// push_create_obj_to_npu routine for each NPU
        virtual void* alloc_fill_ndi_obj (mem_alloc_helper_t& m) {return NULL;}

        // Override to return TRUE if dependent objects also need to be
        // pushed to NDI anytime this object is created or deleted in NDI.
        virtual bool has_dependent_objs () {return false;}

        /// Push newly created object to NDI for given NPU.
        /// Will also be called for Rollback of previous Delete object.
        /// NDI object create API should be called here.
        /// If True returned then this NPU will be tracked for rollback.
        /// Return False if the NDI Create API was not called
        /// for this NPU for some valid reason.
        /// Throw a base_ndi_exception in case of failure.
        virtual bool push_create_obj_to_npu (npu_id_t npu_id,
                                             void* ndi_obj) = 0;

        /// Push delete object to NDI for given NPU.
        /// Will also be called for Rollback of previous Create object.
        /// NDI object delete API should be called here.
        /// If True returned then this NPU will be tracked for rollback.
        /// Return False if the NDI Delete API was not called
        /// for this NPU for some valid reason.
        /// Throw a base_ndi_exception incase of failure
        virtual bool push_delete_obj_to_npu (npu_id_t npu_id) = 0;

        /// Return True for simple leaf attributes in the object
        /// Override to return false for compound attributes (containers)
        /// or instantiated attributes (lists).
        virtual bool is_leaf_attr (nas_attr_id_t attr_id) {return true;}

        /// Push change in value for simple leaf attribute to NDI for given NPU.
        /// NDI attribute modify API should be called here.
        /// If True returned then this NPU will be tracked for rollback.
        /// for this NPU for some valid reason.
        /// Throw a base_ndi_exception incase of failure
        virtual bool push_leaf_attr_to_npu (nas_attr_id_t attr_id,
                                            npu_id_t npu_id) = 0;

        // The following routines SHOULD be overridden
        // if there are non-leaf read-write attributes in the object
        // which can be modified after initial creation
        //
        // For compound attributes (containers) or instantiated attributes (lists)
        // the sub-attributes need to be individually tracked for rollback
        virtual void push_non_leaf_attr_ndi (nas_attr_id_t   non_leaf_attr_id,
                                             base_obj_t&   obj_old,
                                             npu_set_t  npu_list,
                                             rollback_trakr_t& r_trakr,
                                             bool rolling_back)
        { STD_ASSERT (0); }

        virtual void rollback_non_leaf_attr_in_npu (const attr_list_t& attr_hierarchy,
                                                    npu_id_t npu_id,
                                                    base_obj_t& obj_new)
        {STD_ASSERT (0);}

        virtual void rollback_create_attr_in_npu (const attr_list_t&
                                                  attr_hierarchy,
                                                  npu_id_t npu_id)
        {STD_ASSERT (0);}
        virtual void rollback_delete_attr_in_npu (const attr_list_t&
                                                  attr_hierarchy,
                                                  npu_id_t npu_id)
        {STD_ASSERT (0);}

    protected:
        // Attribute tracking -
        // For objects with non-leaf attributes,
        // only the top-most attribute is tracked here.

        // Track set of attributes changed in a given transaction
        attr_set_t                _dirty_attributes;
        // All attributes set in this object across multiple
        // transactions
        attr_set_t                _set_attributes;

        virtual void      remove_npu (npu_id_t npu_id);
        virtual void      set_npu_list (const npu_set_t& new_npus) {_npus = new_npus;}

    private:
        /////// Private members //////
        base_switch_t*      _switch_p; // Back pointer to switch

        // Table has been created in NDI
        bool                      _ndi_created = false;

        npu_set_t                _npus;

        bool                      _npu_list_dirty = false;
        bool                      _following_switch_npus = false;
};


inline bool  base_obj_t::is_created_in_ndi () const
{
    return _ndi_created;
}

inline const attr_set_t& base_obj_t::dirty_attr_list () const
{
    return _dirty_attributes;
}

inline bool base_obj_t::is_attr_dirty (nas_attr_id_t attr_id) const
{
    return _dirty_attributes.contains (attr_id);
}

inline const attr_set_t& base_obj_t::set_attr_list () const
{
    return _set_attributes;
}

inline bool base_obj_t::is_npu_list_dirty () const
{
    return _npu_list_dirty;
}

inline void base_obj_t::remove_npu (npu_id_t npu_id)
{
    _npus.del (npu_id);
}

inline void base_obj_t::clear_all_dirty_flags ()
{
    _dirty_attributes.clear();
    _npu_list_dirty = false;
}

inline void base_obj_t::mark_attr_dirty (nas_attr_id_t attr_id)
{
    _dirty_attributes.add (attr_id);
}

inline nas_switch_id_t base_obj_t::switch_id() const
{
    return _switch_p->id();
}


}

#endif
