MODULES=gplibpq
PG_CONFIG=pg_config

subdir = src/test/walrep/
top_builddir = ../../..

USE_MODULE_DB=1
include $(top_builddir)/src/Makefile.global

REGRESS = setup
REGRESS += replication_views_mirrored missing_xlog walreceiver generate_ao_xlog generate_aoco_xlog
REGRESS_OPTS = --load-extension=gp_inject_fault --init-file=$(top_builddir)/src/test/regress/init_file

NO_PGXS = 1
include $(top_srcdir)/src/makefiles/pgxs.mk

# Ease the top-level Makefile's job a little bit, and install supporting
# libraries as part of installcheck.
installcheck: install
