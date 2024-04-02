#include <stdio.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <errno.h>

/*
    argv[1] - postmaster PID
    argv[2] - UID that postgres was launched under
 */

int main(int argc, char** argv)
{
    pid_t pid;
    uid_t uid;
    char* postmaster_instance_gr = malloc(PATH_MAX);
    char* postmaster_instance_gr_procs = malloc(PATH_MAX);
    const char* root_postgres_gr = "/sys/fs/cgroup/postgres";
    FILE* f_postmaster_gr;

    if(argc < 3){
        printf("pg_isolate - wrong arguments for cgroup_tree_initialize: use "
               "$1-pid; $2-uid\n");
        return -1;
    }

    pid = atol(argv[1]);
    uid = atol(argv[2]);

    if( snprintf(postmaster_instance_gr, PATH_MAX, 
                 "%s/%d/", root_postgres_gr, pid) <= 0){
        return -1;
    }

    if( snprintf(postmaster_instance_gr_procs, PATH_MAX, 
                 "%s/cgroup.procs", postmaster_instance_gr) <= 0){
        return -1;
    }

    if( mkdir(root_postgres_gr, 0755) != 0 ){
        if(errno != EEXIST){
            return -1;
        }
    }

    if( mkdir(postmaster_instance_gr, 0755) != 0 ){
        return -1;
    }
    if( chown(postmaster_instance_gr, uid, uid) != 0 ){
        return -1;
    }

    f_postmaster_gr = fopen(postmaster_instance_gr_procs, "w");
    if(f_postmaster_gr == NULL){
        return -1;
    }
    if( fprintf(f_postmaster_gr, "%d", pid) <= 0){
        return -1;
    }
    if( fclose(f_postmaster_gr) != 0 ){
        return -1;
    }

    free(postmaster_instance_gr);
    free(postmaster_instance_gr_procs);

    return 0;
}