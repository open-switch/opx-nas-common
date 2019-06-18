/*
 * Copyright (c) 2019 Dell Inc.
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
 * filename: nas_vrf_utils.cpp
 **/

#include "nas_vrf_utils.h"
#include "event_log.h"
#include "std_assert.h"
#include "std_rw_lock.h"
#include "std_utils.h"

#include <unordered_map>
#include <memory>

struct _vrf_key_hash {
    size_t operator()(const char *key) const {
        size_t _hash = 0;
        while (*key!='\0') {
            _hash ^= *key++ << 1;
        }
        return _hash;
    }
};

struct _vrf_key_equal {
    bool operator ()(const char *a, const char *b) const {
        return strcmp(a,b)==0;
    }
};

static auto vrf_map = new std::unordered_map<const char *,nas_vrf_ctrl_t *, _vrf_key_hash,_vrf_key_equal>;

static std_rw_lock_t rw_lock = PTHREAD_RWLOCK_INITIALIZER;

static inline void print_record(const nas_vrf_ctrl_t *p) {
    EV_LOGGING(NAS_VRF,ERR,"VRF-INFO","VRF:%s vrf_id:0x%lx",
               p->vrf_name,p->vrf_id);
}

/**
 * Search for the record - fill it in based on the search parameters and return it
 */
static nas_vrf_ctrl_t *_locate(const char *vrf_name) {
    auto it = vrf_map->find(vrf_name);
    if (it == vrf_map->end()) return NULL;
    return it->second;
}

static bool _add(nas_vrf_ctrl_t *rec) {
    char *key = (char*)malloc(NAS_VRF_NAME_SZ+1);

    if (key == NULL) {
        EV_LOGGING(NAS_VRF,ERR,"VRF-ADD","Memory alloc. failed for VRF:%s",
                   rec->vrf_name);
        return false;
    }
    memset(key, 0, NAS_VRF_NAME_SZ+1);

    safestrncpy(key, rec->vrf_name, NAS_VRF_NAME_SZ+1);
    nas_vrf_ctrl_t *p = new nas_vrf_ctrl_t;

    if (p == nullptr) {
        EV_LOGGING(NAS_VRF,ERR,"VRF-ADD","Memory alloc. failed for VRF:%s info",
                   rec->vrf_name);
        free(key);
        return false;
    }
    memcpy(p, rec, sizeof(nas_vrf_ctrl_t));
    vrf_map->insert(std::make_pair((const char*)key, p));
    EV_LOGGING(NAS_VRF, INFO, "VRF-ADD", "VRF:%s 0x%lx info added successfully!",
               rec->vrf_name, rec->vrf_id);
    return true;
}

static bool _update(nas_vrf_ctrl_t *rec) {
    /* Update the internal VRF-id on the existing info. */
    nas_vrf_ctrl_t *_p = _locate(rec->vrf_name);
    if (_p == nullptr) {
        EV_LOGGING(NAS_VRF,ERR,"VRF-ADD","VRF info. update when entry doesnt exist for:%s",
                   rec->vrf_name);
        return false;
    }
    if (rec->vrf_int_id != 0) {
        _p->vrf_int_id = rec->vrf_int_id;
    }
    return true;
}

/**
 * Remove any records associated with this entry
 */
static void _cleanup(nas_vrf_ctrl_t *rec) {
    auto it = vrf_map->find((const char*)rec->vrf_name);
    if (it == vrf_map->end()) {
        EV_LOGGING(NAS_VRF,ERR,"VRF-DEL","VRF:%s info does not exist!",
                   rec->vrf_name);
        return;
    }
    nas_vrf_ctrl_t *tmp_rec = it->second;
    free((char*)it->first);
    vrf_map->erase(it);
    delete tmp_rec;
    EV_LOGGING(NAS_VRF, INFO, "VRF-DEL","VRF:%s info deleted successfully!",
               rec->vrf_name);
}

extern "C" {

t_std_error nas_update_vrf_info (nas_vrf_op_t op, nas_vrf_ctrl_t *p) {
    STD_ASSERT(p!=NULL);
    std_rw_lock_write_guard l(&rw_lock);
    if (op == NAS_VRF_OP_ADD) {
        _add(p);
    } else if (op == NAS_VRF_OP_UPD) {
        _update(p);
    } else if (op == NAS_VRF_OP_DEL) {
        _cleanup(p);
    }
    return STD_ERR_OK;
}

t_std_error nas_get_vrf_obj_id_from_vrf_name(const char *vrf_name, nas_obj_id_t *obj_id) {
    STD_ASSERT(vrf_name!=NULL);
    std_rw_lock_read_guard l(&rw_lock);
    nas_vrf_ctrl_t *_p = _locate(vrf_name);
    if (_p==nullptr) {
        return STD_ERR(COM,PARAM,0);
    }

    *obj_id = _p->vrf_id;
    EV_LOGGING(NAS_VRF, INFO, "VRF-GET","VRF:%s - VRF_id 0x%lx successful!",
               vrf_name, _p->vrf_id);
    return STD_ERR_OK;
}

t_std_error nas_get_vrf_internal_id_from_vrf_name(const char *vrf_name, hal_vrf_id_t *vrf_id) {
    STD_ASSERT(vrf_name!=NULL);
    std_rw_lock_read_guard l(&rw_lock);
    nas_vrf_ctrl_t *_p = _locate(vrf_name);
    if (_p==nullptr) {
        return STD_ERR(COM,PARAM,0);
    }

    *vrf_id = _p->vrf_int_id;
    EV_LOGGING(NAS_VRF, INFO, "VRF-GET","VRF:%s - VRF_id: %d successful!",
               vrf_name, _p->vrf_int_id);
    return STD_ERR_OK;
}

t_std_error nas_get_vrf_ctrl_from_vrf_name(const char *vrf_name, nas_vrf_ctrl_t *vrf_ctrl_blk) {
    STD_ASSERT(vrf_name != NULL);
    std_rw_lock_read_guard l(&rw_lock);
    nas_vrf_ctrl_t *tmp = _locate(vrf_name);
    if (tmp == nullptr) {
        return STD_ERR(COM, PARAM, 0);
    }
    *vrf_ctrl_blk = *tmp;
    return STD_ERR_OK;
}
}

void nas_get_all_vrf_ctrl(std::vector<nas_vrf_ctrl_t> &v) {
    std_rw_lock_read_guard l(&rw_lock);
    auto it = vrf_map->begin();
    auto end = vrf_map->end();

    for ( ; it != end ; ++it ) {
        v.push_back(*(it->second));
    }
}

void dump_tree_vrf() {
    std_rw_lock_read_guard l(&rw_lock);
    auto it = vrf_map->cbegin();
    auto end = vrf_map->cend();

    for ( ; it != end ; ++it ) {
        print_record(it->second);
    }
}

