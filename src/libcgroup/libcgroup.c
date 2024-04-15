#include "libcgroup.h"
#include "config.h"

#include "postgres.h"
#include "storage/fd.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/limits.h>

#define PID_MAX_LENGTH 20

static char* curr_group_path; 

int 
cg_enter(const int postmaster_pid, const char *name)
{
    char* new_cgroups_path = palloc0(PATH_MAX*sizeof(char));
    char* new_cgroups_path_cgroup_procs = palloc0(PATH_MAX*sizeof(char));
    char* pid_string = palloc0(PID_MAX_LENGTH);
    FILE* cg_proc_fd;

    snprintf(new_cgroups_path, PATH_MAX, "%s/%d/%s", cgroups_postgres_root, postmaster_pid, name);
    snprintf(new_cgroups_path_cgroup_procs, PATH_MAX, "%s/%s", new_cgroups_path, "cgroup.procs");

    if( mkdir(new_cgroups_path, 0755) == -1){
        if(errno != EEXIST){
            const char* s = strerror(errno);
            elog(ERROR, "mkdir(%s) error %s", new_cgroups_path, s);
            pfree(new_cgroups_path);
            pfree(new_cgroups_path_cgroup_procs);
            return -1;
        }
    }

    cg_proc_fd = fopen(new_cgroups_path_cgroup_procs, "w");
    if(cg_proc_fd == NULL){
        pfree(new_cgroups_path);
        pfree(new_cgroups_path_cgroup_procs);
        elog(ERROR, "Isolating process error, cgroup.procs doesnt opened");
        return -1;
    }

    snprintf(pid_string, PID_MAX_LENGTH, "%d", getpid());
    if( fprintf(cg_proc_fd, "%d", getpid()) == -1 ){
        const char* s = strerror(errno);
        elog(ERROR, "write error %s", s);
    }

    if( fclose(cg_proc_fd) != 0){
        const char* s = strerror(errno);
        elog(ERROR, "close error %s", s);
    }

    curr_group_path = new_cgroups_path;
    pfree(new_cgroups_path_cgroup_procs);
    pfree(pid_string);
    return 0;
}

const char*
cg_get_current(void)
{
    return curr_group_path;
}

int 
cg_tune(const char* controller, const char* value)
{
    char* controller_path = palloc0(PATH_MAX*sizeof(char));
    FILE* controller_fd;

    snprintf(controller_path, PATH_MAX, "%s/%s", curr_group_path, controller);
    elog(LOG, "Tune %s: %s-%s", controller_path, controller, value);

    if((controller_fd = fopen(controller_path, "w")) == NULL){
        pfree(controller_path);
        elog(ERROR, "controller file \"%s\" open error", controller_path);
        return -1;
    }

    if( fprintf(controller_fd, "%s", value) == -1 ){
        const char* s = strerror(errno);
        elog(ERROR, "write error %s", s);
    }

    if( fclose(controller_fd) != 0){
        const char* s = strerror(errno);
        elog(ERROR, "close error %s", s);
        pfree(controller_path);
    }

    pfree(controller_path);
    return 0;
}

int 
cg_create(const int postmaster_pid, const char *name)
{
    char* new_cgroups_path = palloc0(PATH_MAX*sizeof(char));
    snprintf(new_cgroups_path, PATH_MAX, "%s/%d/%s", cgroups_postgres_root, postmaster_pid, name);

    if( mkdir(new_cgroups_path, 0755) != 0 ){
        const char* s = strerror(errno);
        elog(WARNING, "pg_isolate, cg_create mkdir(%s) error %s", new_cgroups_path, s);
        pfree(new_cgroups_path);
        return -1;
    }

    pfree(new_cgroups_path);
    return 0;
}