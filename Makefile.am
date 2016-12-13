ACLOCAL_AMFLAGS=-I m4
SUBDIRS = inc

bin_SCRIPTS = scripts/bin/*

bin_PROGRAMS = hshell
hshell_SOURCES = src/hal_shell_client.c
hshell_CPPFLAGS = -I$(top_srcdir)/opx -I$(includedir)/opx
hshell_LDADD = -lopx_common -lopx_logging

pyutilsdir=$(libdir)/opx
pyutils_SCRIPTS = scripts/lib/python/*.py

lib_LTLIBRARIES=libopx_nas_common.la

libopx_nas_common_la_SOURCES= src/hal_if_mapping.cpp src/nas_switch.c src/nas_switch_utl.cpp src/nas_base_utils.cpp src/nas_base_obj.cpp src/nas_base_ndi_utl.cpp src/nas_ndi_obj_id_table.cpp src/nas_if_utils.cpp

libopx_nas_common_la_CPPFLAGS= -D_FILE_OFFSET_BITS=64 -I$(top_srcdir)/inc/opx -I$(includedir)/opx

libopx_nas_common_la_CXXFLAGS=-std=c++11

libopx_nas_common_la_LDFLAGS=-shared -version-info 1:1:0

libopx_nas_common_la_LIBADD=-lopx_common -lopx_cps_api_common -lopx_logging

systemdconfdir=/lib/systemd/system
systemdconf_DATA = scripts/init/*.service
