#
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
#

ACLOCAL_AMFLAGS=-I m4
SUBDIRS = inc

bin_SCRIPTS = scripts/bin/*

COMMON_HARDEN_FLAGS=-fexceptions -fstack-protector-strong -fstack-protector-all -D_FORTIFY_SOURCE=2 -Wall -Wformat -Wformat-security -Werror
C_HARDEN_FLAGS=-Wimplicit-function-declaration
LD_HARDEN_FLAGS=-Wl,-z,defs -Wl,-z,now

bin_PROGRAMS = hshell
hshell_SOURCES = src/hal_shell_client.c
hshell_CFLAGS = -I$(top_srcdir)/opx -I$(includedir)/opx $(COMMON_HARDEN_FLAGS)
hshell_LDFLAGS = $(LD_HARDEN_FLAGS)
hshell_LDADD = -lopx_common

pyutilsdir=$(libdir)/opx
pyutils_SCRIPTS = scripts/lib/python/*.py

lib_LTLIBRARIES=libopx_nas_common.la

libopx_nas_common_la_SOURCES= src/hal_if_mapping.cpp src/nas_switch.c src/nas_switch_utl.cpp src/nas_base_utils.cpp src/nas_base_obj.cpp src/nas_base_ndi_utl.cpp src/nas_ndi_obj_id_table.cpp src/nas_if_utils.cpp src/nas_switch_profile.cpp src/nas_vrf_utils.cpp

libopx_nas_common_la_CPPFLAGS= -D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/inc/opx -I$(includedir)/opx $(COMMON_HARDEN_FLAGS) -fPIC

libopx_nas_common_la_CXXFLAGS=-std=c++11

libopx_nas_common_la_CFLAGS=$(C_HARDEN_FLAGS)

libopx_nas_common_la_LDFLAGS=-lpthread -shared -version-info 1:1:0 $(LD_HARDEN_FLAGS)

libopx_nas_common_la_LIBADD=-lopx_common -lopx_cps_api_common -lopx_logging

systemdconfdir=/lib/systemd/system
systemdconf_DATA = scripts/init/*.service
