MODULE_big = pg_isolate
OBJS = src/main.o src/libcgroup/libcgroup.o src/config/group_config.o src/config/json_parser/cJSON.o
EXTENSION = pg_isolate

PG_CONFIG = pg_config
PGXS := $(shell $(PG_CONFIG) --pgxs)
include $(PGXS)

BINDIR = $(shell $(PG_CONFIG) --bindir)
cgroup_init:
	gcc src/cgroup_initialize.c -o $(BINDIR)/cgroup_initialize
	sudo chown root:root $(BINDIR)/cgroup_initialize
	sudo chmod u+s $(BINDIR)/cgroup_initialize	