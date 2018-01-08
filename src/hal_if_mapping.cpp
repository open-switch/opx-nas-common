/*
 * Copyright (c) 2016 Dell Inc.
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
 * filename: hal_if_mapping.cpp
 **/

/*
 * hal_if_mapping.cpp
 */

#include "hal_if_mapping.h"
#include "iana-if-type.h"
#include "dell-base-interface-common.h"
#include "event_log.h"
#include "std_assert.h"
#include "std_mutex_lock.h"
#include "std_utils.h"
#include <string.h>
#include <stdio.h>

#include <unordered_map>
#include <set>
#include <map>
#include <vector>
#include <memory>

using _key_t = uint64_t;
using if_map_t = std::unordered_map<uint64_t,interface_ctrl_t *>;
using if_name_map_t = std::unordered_map<std::string, interface_ctrl_t *> ;

static std_mutex_lock_create_static_init_rec(db_locks);

static std::set<interface_ctrl_t *> if_records;
static std::unordered_map<intf_info_t,if_map_t,std::hash<int32_t>> if_mappings;

static if_name_map_t if_name_map;

static const intf_info_t _has_key_t[] = {
        HAL_INTF_INFO_FROM_PORT,
        HAL_INTF_INFO_FROM_IF,
        HAL_INTF_INFO_FROM_TAP,
        HAL_INTF_INFO_FROM_VLAN,
        HAL_INTF_INFO_FROM_LAG,
};

static const size_t _has_key_t_len = sizeof(_has_key_t)/sizeof(*_has_key_t);

static const intf_info_t _all_queries_t[] = {
        HAL_INTF_INFO_FROM_PORT,
        HAL_INTF_INFO_FROM_IF,
        HAL_INTF_INFO_FROM_TAP,
        HAL_INTF_INFO_FROM_IF_NAME,
        HAL_INTF_INFO_FROM_VLAN,
        HAL_INTF_INFO_FROM_LAG

};
static const size_t _all_queries_t_len = sizeof(_all_queries_t)/sizeof(*_all_queries_t);


static void print_record(interface_ctrl_t*p) {
    printf("Name:%s, NPU:%d, Port:%d, SubPort:%d, "
            "TapID:%d, VRF:%d, IFIndex:%d MAC %s\n",
            p->if_name,p->npu_id, p->port_id,
            p->sub_interface,p->tap_id,p->vrf_id,p->if_index, p->mac_addr);


}

inline _key_t _mk_key(uint32_t lhs, uint32_t rhs) {
    return  ((_key_t)(lhs))<<32 | rhs;
}

static inline bool _query_valid(intf_info_t q_type, nas_int_type_t if_type)
{
    switch(q_type) {
    case HAL_INTF_INFO_FROM_IF:
    case HAL_INTF_INFO_FROM_IF_NAME:
        // ifindex and name query for all interfaces
        return true;
    case HAL_INTF_INFO_FROM_PORT:
    case HAL_INTF_INFO_FROM_TAP:
        if (if_type == nas_int_type_PORT || if_type == nas_int_type_CPU ||
            if_type == nas_int_type_FC) {
            return true;
        }
        break;
    case HAL_INTF_INFO_FROM_VLAN:
        if (if_type == nas_int_type_VLAN) {
            return true;
        }
        break;
    case HAL_INTF_INFO_FROM_LAG:
        if (if_type == nas_int_type_LAG) {
            return true;
        }
        break;
    }

    return false;
}

static _key_t _mk_key(intf_info_t type,interface_ctrl_t *rec) {
    switch(type) {
    case HAL_INTF_INFO_FROM_PORT:
        return _mk_key(rec->npu_id, rec->port_id);
    case HAL_INTF_INFO_FROM_LAG:
        return rec->lag_id;
    case HAL_INTF_INFO_FROM_VLAN:
        return rec->vlan_id;
    case HAL_INTF_INFO_FROM_IF:
        return _mk_key(rec->vrf_id,rec->if_index);
    case HAL_INTF_INFO_FROM_TAP:
        return rec->tap_id;
    default:
        break;
    }
    return 0;
}

/**
 * Search for the record - fill it in based on the search parameters and return it
 */
static interface_ctrl_t *_locate(intf_info_t type, interface_ctrl_t *rec) {
    if (!_query_valid(type, rec->int_type)) {
        return NULL;
    }
    if (type == HAL_INTF_INFO_FROM_IF_NAME) {
        if (if_name_map.find(rec->if_name)==if_name_map.end()) return NULL;
        return if_name_map.at(rec->if_name);
    }
    uint64_t k = _mk_key(type,rec);
    if (k==0) return NULL;
    auto it = if_mappings[type].find(k);
    if (it==if_mappings[type].end()) return NULL;
    return it->second;
}

static bool _add(intf_info_t type, interface_ctrl_t *rec) {
    if (!_query_valid(type, rec->int_type)) {
        return false;
    }
    if (type == HAL_INTF_INFO_FROM_IF_NAME) {
        if (strlen(rec->if_name)==0) return false;
        if_name_map[rec->if_name] = rec;
    } else {
        _key_t k = _mk_key(type,rec);
        if (k==0) return false;
        if_mappings[type][k] = rec;
    }
    return true;
}

/**
 * Remove any records associated with this entry
 */
static void _cleanup(interface_ctrl_t *rec) {
    size_t ix = 0;
    for ( ; ix < _all_queries_t_len ; ++ix ) {
        interface_ctrl_t *_rec  = _locate(_all_queries_t[ix],rec);
        if (_rec==nullptr) continue;
        rec = _rec;
        break;
    }

    if (rec==nullptr) return ;

    ix = 0;
    for ( ; ix < _has_key_t_len ; ++ix ) {
        if (!_query_valid(_has_key_t[ix], rec->int_type)) {
            continue;
        }
        _key_t k = _mk_key(_has_key_t[ix],rec);
        if(k==0) continue;
        if_mappings[_has_key_t[ix]].erase(k);
    }
    if (strlen(rec->if_name)>0) {
        if_name_map.erase(rec->if_name);
    }
    if_records.erase(rec);
    delete rec;
}

extern "C" {

static std::map<nas_int_type_t, const char *> nas_to_ietf_if_types  =
{
    {nas_int_type_CPU, IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_CPU},
    {nas_int_type_PORT, IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_ETHERNETCSMACD},
    {nas_int_type_VLAN, IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_L2VLAN},
    {nas_int_type_LAG, IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_IEEE8023ADLAG},
    {nas_int_type_LPBK, IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_SOFTWARELOOPBACK},
    {nas_int_type_FC, IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_FIBRECHANNEL},
    {nas_int_type_MACVLAN, IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_MACVLAN},
    {nas_int_type_MGMT, IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_MANAGEMENT},
};

bool nas_to_ietf_if_type_get(nas_int_type_t if_type,  char *ietf_type, size_t size)
{
    auto it = nas_to_ietf_if_types.find(if_type);
    if (it == nas_to_ietf_if_types.end() || ietf_type == NULL) {
        return false;
    }
    safestrncpy(ietf_type, it->second, size);
    return true;
}

//Conversion from ietf interface type to dell intf type
bool ietf_to_nas_if_type_get(const char *ietf_type, nas_int_type_t *if_type)
{
    bool ret = true;

    if (strncmp(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_ETHERNETCSMACD, ietf_type,
            strlen(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_ETHERNETCSMACD)) == 0) {
        *if_type =  nas_int_type_PORT;

    } else if (strncmp(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_L2VLAN, ietf_type,
            strlen(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_L2VLAN)) == 0) {
        *if_type =  nas_int_type_VLAN;
    } else if(strncmp(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_IEEE8023ADLAG, ietf_type,
            strlen(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_IEEE8023ADLAG)) == 0) {
        *if_type =  nas_int_type_LAG;
    } else if(strncmp(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_CPU, ietf_type,
            strlen(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_CPU)) == 0) {
        *if_type =  nas_int_type_CPU;
    } else if(strncmp(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_SOFTWARELOOPBACK, ietf_type,
            strlen(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_SOFTWARELOOPBACK)) == 0) {
        *if_type =  nas_int_type_LPBK;
    } else if(strncmp(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_FIBRECHANNEL, ietf_type,
            strlen(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_FIBRECHANNEL)) == 0) {
        *if_type =  nas_int_type_FC;
    } else if(strncmp(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_MACVLAN, ietf_type,
            strlen(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_MACVLAN)) == 0) {
        *if_type =  nas_int_type_MACVLAN;
    } else if(strncmp(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_MANAGEMENT, ietf_type,
            strlen(IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_MANAGEMENT)) == 0) {
        *if_type =  nas_int_type_MGMT;
    } else {
        ret = false;
    }

    return ret;
}

t_std_error dn_hal_if_register(hal_intf_reg_op_type_t reg_opt,interface_ctrl_t *detail) {
    std_mutex_simple_lock_guard l(&db_locks);
    if (reg_opt==HAL_INTF_OP_DEREG) {
        _cleanup(detail);
        return STD_ERR_OK;
    }

    size_t ix = 0;
    for ( ; ix < _all_queries_t_len ; ++ix ) {
        if (_locate(_all_queries_t[ix],detail)!=nullptr) {
            return STD_ERR(INTERFACE,PARAM,0);
        }
    }

    std::unique_ptr<interface_ctrl_t> p(new (std::nothrow) interface_ctrl_t);

    if (p.get()==nullptr) {
        return STD_ERR(INTERFACE,PARAM,0);
    }

    *p = *detail;
    if_records.insert(p.get());
    ix = 0;
    bool added = false;
    for ( ; ix < _all_queries_t_len ; ++ix ) {
        if (_add(_all_queries_t[ix],p.get())) {
            added = true;
        }
    }
    if (!added) {
        if_records.erase(p.get());
        return STD_ERR(INTERFACE,PARAM,0);
    }
    p.release();
    return STD_ERR_OK;
}

t_std_error dn_hal_get_interface_info(interface_ctrl_t *p) {
    STD_ASSERT(p!=NULL);
    std_mutex_simple_lock_guard l(&db_locks);
    interface_ctrl_t *_p = _locate(p->q_type,p);
    if (_p==nullptr) {
        return STD_ERR(INTERFACE,PARAM,0);
    }

    *p = *_p;
    return STD_ERR_OK;
}
/*  Update only non- Key attributes like MAC address */
static t_std_error dn_hal_update_interface(interface_ctrl_t *p) {
    STD_ASSERT(p!=NULL);
    std_mutex_simple_lock_guard l(&db_locks);
    interface_ctrl_t *_p = _locate(p->q_type,p);
    if (_p==nullptr) {
        return STD_ERR(INTERFACE,PARAM,0);
    }

    /* only MAC can be updated in the DB. DEREG and REG should be done for other items. */
    safestrncpy(_p->mac_addr, (const char *)p->mac_addr, sizeof(_p->mac_addr));
    return STD_ERR_OK;
}

/* Update MAC address in string Format "11:11:11:11:11:11" */
t_std_error dn_hal_update_intf_mac(hal_ifindex_t ifx, const char *mac) {

    interface_ctrl_t _intf;
    memset(&_intf, 0, sizeof(_intf));
    _intf.if_index = ifx;
    _intf.q_type = HAL_INTF_INFO_FROM_IF;
    safestrncpy(_intf.mac_addr, mac, sizeof(_intf.mac_addr));

    EV_LOGGING(INTERFACE,INFO,"NAS-IF-REG","OS update for MAC intf  %s MAC %s", _intf.if_name, mac);
    return(dn_hal_update_interface(&_intf));
}

static void dump_tree(intf_info_t q_type) {
    auto it = if_mappings.find(q_type);
    if (it == if_mappings.end()) {
        return;
    }
    auto& map = it->second;
    for (auto map_it: map) {
        printf("idx %llu ",(unsigned long long)map_it.first);
        print_record(map_it.second);
    }

}

static void dump_tree_ifname(if_name_map_t &map) {
    auto it = map.begin();
    auto end = map.end();

    for ( ; it !=end ; ++it ) {
        printf("ifname:%s ",(it->first).c_str());
        print_record(it->second);
    }
}

void dn_hal_dump_interface_mapping(void) {
    std_mutex_simple_lock_guard l(&db_locks);
    printf("Dumping NPU/Port mapping...\n");
    dump_tree(HAL_INTF_INFO_FROM_PORT);

    printf("Dumping NPU/Port ifname mapping...\n");
    dump_tree_ifname(if_name_map);

    printf("Dumping interface index mapping...\n");
    dump_tree(HAL_INTF_INFO_FROM_IF);

    printf("Dumping tap index mapping...\n");
    dump_tree(HAL_INTF_INFO_FROM_TAP);

    printf("Dumping VLAN ID mapping...\n");
    dump_tree(HAL_INTF_INFO_FROM_VLAN);

    printf("Dumping LAG ID mapping...\n");
    dump_tree(HAL_INTF_INFO_FROM_LAG);

}

}
