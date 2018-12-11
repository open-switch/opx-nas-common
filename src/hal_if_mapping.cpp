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
 * filename: hal_if_mapping.cpp
 **/

/*
 * hal_if_mapping.cpp
 */

#include "dell-base-common.h"
#include "hal_if_mapping.h"
#include "iana-if-type.h"
#include "dell-base-interface-common.h"
#include "event_log.h"
#include "std_assert.h"
#include "std_rw_lock.h"
#include "std_utils.h"
#include <string.h>
#include <stdio.h>

#include <unordered_map>
#include <set>
#include <map>
#include <vector>
#include <memory>
#include <cstring>

using _key_t = uint64_t;
using if_map_t = std::unordered_map<uint64_t,interface_ctrl_t *>;
using if_name_map_t = std::unordered_map<std::string, interface_ctrl_t *> ;

static std_rw_lock_t db_lock = PTHREAD_RWLOCK_INITIALIZER;

static std::set<interface_ctrl_t *> if_records;
static std::unordered_map<intf_info_t,if_map_t,std::hash<int32_t>> if_mappings;

static if_name_map_t if_name_map;

static _key_t INVALID_KEY = (~0);

static std::set<hal_ifindex_t> if_indexes;

static const intf_info_t _has_key_t[] = {
        HAL_INTF_INFO_FROM_PORT,
        HAL_INTF_INFO_FROM_IF,
        HAL_INTF_INFO_FROM_TAP,
        HAL_INTF_INFO_FROM_VLAN,
        HAL_INTF_INFO_FROM_LAG,
        HAL_INTF_INFO_FROM_BRIDGE_ID
};

static const size_t _has_key_t_len = sizeof(_has_key_t)/sizeof(*_has_key_t);

static const intf_info_t _all_queries_t[] = {
        HAL_INTF_INFO_FROM_PORT,
        HAL_INTF_INFO_FROM_IF,
        HAL_INTF_INFO_FROM_TAP,
        HAL_INTF_INFO_FROM_IF_NAME,
        HAL_INTF_INFO_FROM_VLAN,
        HAL_INTF_INFO_FROM_LAG,
        HAL_INTF_INFO_FROM_BRIDGE_ID

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
    case HAL_INTF_INFO_FROM_BRIDGE_ID:
        if (if_type == nas_int_type_DOT1D_BRIDGE) {
            return true;
        }
    }

    return false;
}

static _key_t _mk_key(intf_info_t type,interface_ctrl_t *rec) {
    switch(type) {
    case HAL_INTF_INFO_FROM_PORT:
        if (rec->port_mapped) {
            return _mk_key(rec->npu_id, rec->port_id);
        } else {
            return INVALID_KEY;
        }
    case HAL_INTF_INFO_FROM_LAG:
        return rec->lag_id;
    case HAL_INTF_INFO_FROM_VLAN:
        return rec->vlan_id;
    case HAL_INTF_INFO_FROM_IF:
        return _mk_key(rec->vrf_id,rec->if_index);
    case HAL_INTF_INFO_FROM_TAP:
        if (rec->port_mapped) {
            return rec->tap_id;
        } else {
            return INVALID_KEY;
        }
    case HAL_INTF_INFO_FROM_BRIDGE_ID:
        return rec->bridge_id;
    default:
        break;
    }
    return INVALID_KEY;
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
    if (k==INVALID_KEY) return NULL;
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
        if (k==INVALID_KEY) return false;
        if_mappings[type][k] = rec;
    }
    if_indexes.insert(rec->if_index);
    return true;
}

/**
 * Remove any records associated with this entry
 */
static void _cleanup(interface_ctrl_t *rec) {
    size_t ix = 0;
    interface_ctrl_t *_rec = nullptr;

    if (rec==nullptr) return;

    for ( ; ix < _all_queries_t_len ; ++ix ) {
        _rec  = _locate(_all_queries_t[ix],rec);
        if (_rec==nullptr) continue;
        break;
    }

    /* Interface does not exist, return */
    if (_rec==nullptr) return;

    ix = 0;
    for ( ; ix < _has_key_t_len ; ++ix ) {
        if (!_query_valid(_has_key_t[ix], _rec->int_type)) {
            continue;
        }
        _key_t k = _mk_key(_has_key_t[ix],_rec);
        if(k==INVALID_KEY) continue;
        if_mappings[_has_key_t[ix]].erase(k);
    }
    if (strlen(_rec->if_name)>0) {
        if_name_map.erase(_rec->if_name);
    }

    if (_rec->desc) {
        delete [] _rec->desc;
    }

    if_records.erase(_rec);
    if_indexes.erase(_rec->if_index);
    delete _rec;
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
    {nas_int_type_VXLAN, IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_VXLAN},
    {nas_int_type_VLANSUB_INTF, IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_VLANSUBINTERFACE},
};


static auto ietf_to_nas_types  = new std::unordered_map<std::string,nas_int_type_t>
{
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_CPU, nas_int_type_CPU},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_ETHERNETCSMACD, nas_int_type_PORT},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_L2VLAN, nas_int_type_VLAN},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_IEEE8023ADLAG, nas_int_type_LAG},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_SOFTWARELOOPBACK, nas_int_type_LPBK},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_FIBRECHANNEL, nas_int_type_FC},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_MACVLAN, nas_int_type_MACVLAN},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_MANAGEMENT, nas_int_type_MGMT},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_VXLAN, nas_int_type_VXLAN},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_VLANSUBINTERFACE, nas_int_type_VLANSUB_INTF},
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
    auto it = ietf_to_nas_types->find(std::string(ietf_type));
    if(it != ietf_to_nas_types->end()){
        *if_type = it->second;
        return true;
    }

    return false;
}

static auto ietf_to_nas_os_types  = new std::unordered_map<std::string,BASE_CMN_INTERFACE_TYPE_t>
{
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_CPU, BASE_CMN_INTERFACE_TYPE_CPU},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_ETHERNETCSMACD, BASE_CMN_INTERFACE_TYPE_L3_PORT},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_L2VLAN, BASE_CMN_INTERFACE_TYPE_VLAN},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_IEEE8023ADLAG, BASE_CMN_INTERFACE_TYPE_LAG},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_SOFTWARELOOPBACK, BASE_CMN_INTERFACE_TYPE_LOOPBACK},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_IANAIFT_FIBRECHANNEL, BASE_CMN_INTERFACE_TYPE_L3_PORT},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_MACVLAN, BASE_CMN_INTERFACE_TYPE_MACVLAN},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_MANAGEMENT, BASE_CMN_INTERFACE_TYPE_MANAGEMENT},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_VXLAN, BASE_CMN_INTERFACE_TYPE_VXLAN},
    {IF_INTERFACE_TYPE_IANAIFT_IANA_INTERFACE_TYPE_BASE_IF_VLANSUBINTERFACE, BASE_CMN_INTERFACE_TYPE_VLAN_SUBINTF},
};

//Conversion from ietf interface type to linux intf type
bool ietf_to_nas_os_if_type_get(const char *ietf_type, BASE_CMN_INTERFACE_TYPE_t *if_type)
{
    auto it = ietf_to_nas_os_types->find(std::string(ietf_type));
    if(it != ietf_to_nas_os_types->end()){
        *if_type = it->second;
        return true;
    }

    return false;
}

t_std_error dn_hal_if_register(hal_intf_reg_op_type_t reg_opt,interface_ctrl_t *detail) {
    std_rw_lock_write_guard l(&db_lock);
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
    if (p->q_type == HAL_INTF_INFO_FROM_PORT) {
        // If query interface by its NPU port, the interface
        // should be mapped interface.
        p->port_mapped = true;
    }
    std_rw_lock_read_guard l(&db_lock);
    interface_ctrl_t *_p = _locate(p->q_type,p);
    if (_p==nullptr) {
        return STD_ERR(INTERFACE,PARAM,0);
    }

    *p = *_p;
    return STD_ERR_OK;
}

t_std_error dn_hal_get_next_ifindex(hal_ifindex_t *ifindex, hal_ifindex_t *next_ifindex) {
    std_rw_lock_read_guard l(&db_lock);
    if (ifindex == nullptr) {
        std::set<hal_ifindex_t>::iterator it = if_indexes.begin();
        *next_ifindex = *it;
        return STD_ERR_OK;
    }

    auto it = if_indexes.upper_bound(*ifindex);
    if( it != if_indexes.end()) {
        *next_ifindex = *it;
        return STD_ERR_OK;
    }

    return STD_ERR(INTERFACE,PARAM,0);
}

/*  Update only non- Key attributes like MAC address */
static t_std_error dn_hal_update_interface(interface_ctrl_t *p) {
    STD_ASSERT(p!=NULL);
    std_rw_lock_write_guard l(&db_lock);
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

/* Update router interface (non-default VRF context) for lower layer interface (default VRF context) */
t_std_error nas_cmn_update_router_intf_info(hal_vrf_id_t vrf_id, hal_ifindex_t ifx,
                                            l3_intf_info_t *info) {

    STD_ASSERT(info!=NULL);
    std_rw_lock_write_guard l(&db_lock);
    interface_ctrl_t _intf;
    memset(&_intf, 0, sizeof(_intf));
    _intf.vrf_id = vrf_id;
    _intf.if_index = ifx;
    _intf.q_type = HAL_INTF_INFO_FROM_IF;
    interface_ctrl_t *_p = _locate(_intf.q_type, &_intf);
    if (_p==nullptr) {
        return STD_ERR(INTERFACE,PARAM,0);
    }
    memcpy(&(_p->l3_intf_info), info, sizeof(l3_intf_info_t));

    EV_LOGGING(INTERFACE,INFO,"NAS-IF-UPDATE",
               "Update router interface vrf-id:%d if-index:%d for parent interface vrf-id:%d if-index:%d",
               vrf_id, ifx, info->vrf_id, info->if_index);
    return STD_ERR_OK;
}

t_std_error dn_hal_update_intf_desc(interface_ctrl_t *p, const char *desc) {

    STD_ASSERT(desc!=NULL);
    STD_ASSERT(p!=NULL);

    if (strlen(desc) > MAX_INTF_DESC_LEN) {
        return STD_ERR(INTERFACE, PARAM, 0);
    }

    size_t desc_len = strlen(desc);

    std_rw_lock_write_guard l(&db_lock);
    interface_ctrl_t *_p = _locate(p->q_type, p);
    if (_p == nullptr) {
        return STD_ERR(INTERFACE, PARAM, 0);
    }

    if (_p->desc != nullptr) {
        delete[] _p->desc;
    }

    /**
      * We check the length of description and ensure the first character
      * is alphanumeric because CPS uses '/036' for a blank string
      * instead of '/0'. Blank or empty string means the attribute is going
      * to be cleared.
      *
    **/

    if (desc_len == 0 || !isalnum(desc[0])) {
        _p->desc = nullptr;
    } else {
        try {
            /* +1 to allocation size to account for '\0' character */
            _p->desc = new char[desc_len + 1];
        } catch (std::bad_alloc& ba) {
            EV_LOGGING(INTERFACE,ERR,"NAS-IF-REG",
                "Memory allocation fail for interface description.");
            return STD_ERR(INTERFACE, PARAM, 0);
        }

        safestrncpy(_p->desc, desc, desc_len + 1);
    }

    EV_LOGGING(INTERFACE,INFO,"NAS-IF-REG","OS update description for intf %s changed to: %s", _p->if_name, desc);
    return STD_ERR_OK;
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
    std_rw_lock_read_guard l(&db_lock);
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

    printf("Dumping BRIDGE ID mapping...\n");
    dump_tree(HAL_INTF_INFO_FROM_BRIDGE_ID);


}

}
