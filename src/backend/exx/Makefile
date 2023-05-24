# Copyright (c) 2018-2021, Vitesse Data Inc. All rights reserved.
subdir = src/backend/exx
top_builddir = ../../..
include ./exx.mk 

OBJS = \
	exx_init.o \
	exx_main.o \
	exx_runtime.o \

include $(top_srcdir)/src/backend/common.mk

wipe: clean
	cd adt && make clean && python pg_proc.py && python pg_oper.py
	cd qnode && make clean
	cd util && make clean
	cd dep && make clean

world: all
	cd adt && make all
	cd qnode && make all
	cd util && make all
	cd dep && make all

.PHONY : always

