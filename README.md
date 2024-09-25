# pg_isolate

### Installation

```shell
make install
make cgroup_init
```

### Configuration

Extension will try to find `pg_isolate.json` file in desired PGDATA directory.      
Config has following structure
```json
{
    "example" : {
        "database": "postgres"
    },

    "example_administator" : {
        "database": "postgres",
        "user": "extio1"
    },

    "default" : {
    }
}
```

- `example` - name of cgroup
- Backend created for `database` and `user` will be placed into following cgroup 
- If `default` specified, backends that hasn't match to some cgroup will be places here

### Launch 


```shell
postgres -D $PGDATA -c shared_preload_libraries=pg_isolate
```