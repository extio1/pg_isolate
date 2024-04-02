MODULE_big = pg_isolate
OBJS = src/main.o src/libcgroups/libcgroups.o
EXTENSION = pg_isolate

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

BINDIR = $(shell $(PG_CONFIG) --bindir)
cgroups_init:
	gcc src/cgroups_initialize.c -o $(BINDIR)/cgroups_initialize
	sudo chown root:root $(BINDIR)/cgroups_initialize
	sudo chmod u+s $(BINDIR)/cgroups_initialize	