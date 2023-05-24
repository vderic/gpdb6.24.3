# Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
subdir = src/backend/exx/util
top_builddir = ../../../..
include ../exx.mk 

COMMON_OBJS = \
	exx_required.o \
	exx_sarg.o \
	decimal.o \
	xexpr_types.o  \
	stringbuffer.o 

X86_OBJS =

ifeq ($(uname_p),aarch64)
OBJS = $(COMMON_OBJS)
else
OBJS = $(COMMON_OBJS) $(X86_OBJS)
endif


all: SUBSYS.o 

include $(top_srcdir)/src/backend/common.mk

format:
	clang-format -i *.c *.h

.PHONY : always

