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
 * nas_base_ut_simple_derived_obj.h
 *
 *  Created on: Apr 13, 2015
 */

#include "nas_base_obj.h"
#include <stdlib.h>
#include <map>
#include <unordered_map>

typedef uint64_t ndi_obj_id_t;

////////////////
//Example Yang object with 2 leaf attributes and an optional NPU list
//////////////
typedef enum {
    BASE_UT_ATTR1 = 1,
    BASE_UT_ATTR2 = 2,
    BASE_UT_NPU_LIST = 3,
} BASE_UT_OBJ_t;

typedef struct _ndi_ut_obj
{
    uint_t attr1;
    uint_t attr2;
} ndi_ut_obj_t;

class derived_switch_t;

///////////////////
// Example Object class derived from Base class
//////////////////
class derived_obj_t: public nas::base_obj_t
{
#define MAX_ATTR1_RANGE  20
#define MAX_ATTR2_RANGE  10
    public:
        derived_obj_t (derived_switch_t* switch_p);

        nas_obj_id_t obj_id () {return _obj_id;}
        uint_t attr1() {return _obj.attr1;}
        uint_t attr2() {return _obj.attr2;}
        ndi_obj_id_t ndi_obj_id (npu_id_t npu_id) const
        { return (_ndi_obj_ids.at (npu_id)); }

        virtual const char* name () const override final {return "BASE UT Obj";}
        virtual e_event_log_types_enums ev_log_mod_id () const override final {return ev_log_t_NAS_COM;}
        virtual const char* ev_log_mod_name () const override final {return "NAS-BASE-UT";}

        void set_obj_id (nas_obj_id_t obj_id) {_obj_id = obj_id;}
        void set_attr1 (uint_t attr1);
        void set_attr2 (uint_t attr2);
        void set_ndi_obj_id (npu_id_t npu_id,
                             ndi_obj_id_t id);
        void reset_ndi_obj_id (npu_id_t npu_id) {set_ndi_obj_id (npu_id, 0);}

        void commit_create (bool rolling_back);
        nas::attr_set_t commit_modify (base_obj_t& obj_old, bool rolling_back);

        void* alloc_fill_ndi_obj (nas::mem_alloc_helper_t& mem_trakr);
        virtual bool push_create_obj_to_npu (npu_id_t npu, void* ndi_obj) override final;
        virtual bool push_delete_obj_to_npu (npu_id_t npu) override final;
        virtual bool push_leaf_attr_to_npu (nas_attr_id_t attr_id, npu_id_t npu) override final;

    private:
        ndi_ut_obj_t  _obj;
        nas_obj_id_t  _obj_id;

        ///// Typedefs /////
        typedef std::unordered_map <npu_id_t, size_t> ndi_obj_id_map_t;

        // List of mapped NDI IDs one for each NPU
        // managed by this NAS component
        ndi_obj_id_map_t          _ndi_obj_ids;
};

///////////////////
// Example Switch class derived from Base class
//////////////////
class derived_switch_t: public nas::base_switch_t
{
    public:
        derived_switch_t (nas_obj_id_t id): nas::base_switch_t (id) {};
        derived_obj_t&        get_ut_obj (nas_obj_id_t ut_obj_id);
        void      add_ut_obj (derived_obj_t& ut_obj);
        void      change_ut_obj (derived_obj_t& ut_obj);
        void      del_ut_obj (nas_obj_id_t ut_obj_id);
    private:
        typedef std::map<nas_obj_id_t, derived_obj_t> ut_obj_list_t;
        ut_obj_list_t            _ut_objs;
};

t_std_error ndi_ut_obj_create (npu_id_t npu_id,
                               ndi_ut_obj_t* ndi_ut_obj_p,
                               size_t* ndi_ut_obj_id);

t_std_error ndi_ut_obj_delete (npu_id_t npu_id,
                               size_t ndi_ut_obj_id);

t_std_error ndi_ut_obj_attr1_set (npu_id_t npu_id, size_t ndi_ut_obj_id,
                                  uint_t attr_val);

t_std_error ndi_ut_obj_attr2_set (npu_id_t npu_id, size_t ndi_ut_obj_id,
                                  uint_t attr_val);
