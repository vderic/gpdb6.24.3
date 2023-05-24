# Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
subdir = src/backend/exx/xrg
top_builddir = ../../../..
include ../exx.mk

#OBJS = \
#        xrg_column.o \
#        generic_column.o \
#	exx_xrg.o  \
#	exx_aggref.o \
#	exx_xrg_exttab.o
#
#
OBJS = decode.o kite_extscan.o aggref_xexpr.o aggref.i.o kite_target.o kite_json.o exttab.i.o extscanstate.o

include $(top_srcdir)/src/backend/common.mk

format:
	clang-format -i *.c *.h

.PHONY : always

