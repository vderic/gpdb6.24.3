# Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
subdir = src/backend/exx/dep
top_builddir = ../../../..

include ../exx.mk

COMMON_OBJS = arrayfuncs.i.o \
	dg_utilsfn.o \
	external.i.o \
	nodeAgg.i.o \
	planwalker.i.o \

OBJS = $(COMMON_OBJS)

include gporca.mk

include $(top_srcdir)/src/backend/common.mk

.PHONY : always
