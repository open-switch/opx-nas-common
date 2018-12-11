#!/usr/bin/python
# Copyright (c) 2018 Dell Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License"); you may
# not use this file except in compliance with the License. You may obtain
# a copy of the License at http://www.apache.org/licenses/LICENSE-2.0
#
# THIS CODE IS PROVIDED ON AN *AS IS* BASIS, WITHOUT WARRANTIES OR
# CONDITIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING WITHOUT
# LIMITATION ANY IMPLIED WARRANTIES OR CONDITIONS OF TITLE, FITNESS
# FOR A PARTICULAR PURPOSE, MERCHANTABLITY OR NON-INFRINGEMENT.
#
# See the Apache Version 2.0 License for specific language governing
# permissions and limitations under the License.

import cps
import cps_utils
import cps_operations
import cps_object

def commit(obj, op):
    l = []
    tr_obj = {'change': obj.get(), 'operation': op}
    l.append(tr_obj)
    ret = cps.transaction(l)
    if ret:
        print "Success"
        return l
    else:
        try:
            if 'change' in l[0]:
                ret_obj = cps_object.CPSObject(obj=l[0]['change'])
            else:
                ret_obj = cps_object.CPSObject(obj=l[0])
            rs = ret_obj.get_attr_data('cps/object-group/return-string')
            print ("Error: %s") % (rs)
        except KeyError:
            print "Failed"
        return False



def create(obj):
    ret = commit(obj, "create")
    if ret:
        cps_utils.print_obj(ret[0])
    return ret


def set(obj):
    return commit(obj, "set")


def delete(obj):
    return commit(obj, "delete")


def rpc(obj):
    return commit(obj, "rpc")


def get(obj):
    get = []
    if cps.get([obj.get()], get):
        for i in get:
            cps_utils.print_obj(i)

str_to_cps_cb = {
    "create": create,
    "set": set,
    "delete": delete,
    "get": get,
    "rpc": rpc,
}


def get_cb_method(op):
    return str_to_cps_cb[op]
