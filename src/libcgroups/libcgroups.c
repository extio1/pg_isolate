#include "libcgroups.h"

#include "postgres.h"
#include "storage/fd.h"

#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <linux/limits.h>

static char *cgroups_full_path;

// int cg_set(const char* controller);
// int cg_get(const char*);

/*
 * Store to cgroups_full_path current process cgroup from 
 * /proc/PID/cgroup
 */
int 
cg_init(void)
{
    char *proc_path = palloc0(PATH_MAX*sizeof(char));
    char *cgroup_path_buff = palloc0(PATH_MAX*sizeof(char));
    int proc_fd;

    snprintf(proc_path, PATH_MAX, "/proc/%d/cgroup", getpid());
    proc_fd = OpenTransientFile(proc_path, O_RDONLY);
   
    if(cgroup_path_buff == NULL){
        const char* s = strerror(errno);
        elog(ERROR, "palloc'ation error %s", s);
        goto ERR_CLEANUP;
    }

    if(proc_fd == -1){
        const char* s = strerror(errno);
        elog(ERROR, "file opening error %s", s);
        goto ERR_CLEANUP;
    }

    if( read(proc_fd, cgroup_path_buff, PATH_MAX) == -1 ){
        const char* s = strerror(errno);
        elog(ERROR, "read error %s", s);
        goto ERR_CLEANUP;
    }

    
    strcpy(cgroup_path_buff, cgroup_path_buff+3); // remove 0::
    cgroup_path_buff[strlen(cgroup_path_buff)-1] = 0; // remove \n
elog(LOG, "from proc file:%s", cgroup_path_buff);

    cgroups_full_path = palloc0(strlen(cgroup_path_buff)+strlen("/sys/fs/cgroup/")+10);
    snprintf(cgroups_full_path, PATH_MAX, "/sys/fs/cgroup/%s", cgroup_path_buff);

    elog(LOG, "Full path to cgroup initialized as :%s", cgroups_full_path);

    CloseTransientFile(proc_fd);
    pfree(proc_path);
    pfree(cgroup_path_buff);
    return 0;

ERR_CLEANUP:
    CloseTransientFile(proc_fd);
    pfree(proc_path);
    pfree(cgroup_path_buff);
    return -1;
}

int 
cg_enter(const char *name)
{
    char* new_cgroups_path = palloc0(PATH_MAX*sizeof(char));
    char* new_cgroups_path_cgroup_procs = palloc0(PATH_MAX*sizeof(char));
    #define PID_MAX_LENGTH 20
    char* pid_string = palloc0(PID_MAX_LENGTH);
    int cg_proc_fd;
    mode_t old_mask;

    snprintf(new_cgroups_path, PATH_MAX, "%s/%s", cgroups_full_path, name);
    snprintf(new_cgroups_path_cgroup_procs, PATH_MAX, "%s/%s", new_cgroups_path, "cgroup.procs");

elog(LOG, "%d %s %s %s", geteuid(), cgroups_full_path, new_cgroups_path, new_cgroups_path_cgroup_procs);
    old_mask = umask(0);

    if( mkdir(new_cgroups_path, 0755) == -1){
        if(errno != EEXIST){
            pfree(new_cgroups_path);
            pfree(new_cgroups_path_cgroup_procs);
            const char* s = strerror(errno);
            elog(ERROR, "mkdir error %s", s);
            return -1;
        }
    }

    (void) umask(old_mask);

    cg_proc_fd = open(new_cgroups_path_cgroup_procs, O_RDWR);
    if(cg_proc_fd == -1){
        pfree(new_cgroups_path);
        pfree(new_cgroups_path_cgroup_procs);
        elog(ERROR, "Isolating process error, cgroup.procs doesnt opened");
        return -1;
    }
    snprintf(pid_string, PID_MAX_LENGTH, "%d", getpid());
elog(LOG, "file opened %d. print %s", cg_proc_fd, pid_string);
    if( write(cg_proc_fd, pid_string, strlen(pid_string)) == -1 ){
        const char* s = strerror(errno);
        elog(ERROR, "write error %s", s);
    }

    if( close(cg_proc_fd) != -1){
        const char* s = strerror(errno);
        elog(ERROR, "CloseTransientFile error %s", s);
    }

    pfree(new_cgroups_path_cgroup_procs);
    pfree(cgroups_full_path);
    cgroups_full_path = new_cgroups_path;
    return 0;
}

int 
cg_get_current_gr(char** str)
{
    // *str = palloc0(strlen(cgroups_full_path));
    // if( strcpy(*str, cgroups_full_path) == NULL){
    //     return -1;
    // }
    // return 0;
}

