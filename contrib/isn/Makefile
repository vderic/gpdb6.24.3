# contrib/isn/Makefile

MODULES = isn

EXTENSION = isn
DATA = isn--1.0.sql isn--unpackaged--1.0.sql

REGRESS = isn isn_gp
REGRESS_OPTS += --init-file=$(top_srcdir)/src/test/regress/init_file

ifdef USE_PGXS
PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)
else
subdir = contrib/isn
top_builddir = ../..
include $(top_builddir)/src/Makefile.global
include $(top_srcdir)/contrib/contrib-global.mk
endif
