#include <stdio.h>
#include <linux/limits.h>
#include <sys/stat.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

#include "libcgroup/config.h"

/*
    argv[1] - postmaster PID
    argv[2] - UID that postgres was launched under
 */

int main(int argc, char** argv)
{
    pid_t pid;
    uid_t uid;

    char* postgres_instance_gr_path = malloc(PATH_MAX);
    char* postgres_instance_cgroup_procs_path = malloc(PATH_MAX);
    char* postgres_postmaster_gr_path = malloc(PATH_MAX);
    char* postgres_postmaster_cgroup_procs_path = malloc(PATH_MAX);
    char* postgres_instance_subtree_path = malloc(PATH_MAX);
    char* postgres_subtree_path = malloc(PATH_MAX);
    FILE *f_postmaster_gr;
    FILE *f_subtree_f;
    FILE *debug = fopen("/home/extio1/debug.txt", "w");

    if(argc < 3){
        printf("pg_isolate - wrong arguments for cgroup_tree_initialize: use "
               "$1-pid; $2-uid\n");
        return 1;
    }

    pid = atol(argv[1]);
    uid = atol(argv[2]);

    if( snprintf(postgres_instance_gr_path, PATH_MAX, 
                 "%s/%d/", cgroups_postgres_root, pid) <= 0){
        return 2;
    }
    if( snprintf(postgres_instance_cgroup_procs_path, PATH_MAX, 
                 "%s/cgroup.procs", postgres_instance_gr_path) <= 0){
        return 3;
    }
    if( snprintf(postgres_postmaster_gr_path, PATH_MAX, 
                 "%s/postmaster", postgres_instance_gr_path) <= 0){
        return 4;
    }
    if( snprintf(postgres_postmaster_cgroup_procs_path, PATH_MAX, 
                 "%s/cgroup.procs", postgres_postmaster_gr_path) <= 0){
        return 5;
    }
    if( snprintf(postgres_subtree_path, PATH_MAX, 
                "%s/cgroup.subtree_control", cgroups_postgres_root) <= 0){
        return 6;
    }
    if( snprintf(postgres_instance_subtree_path, PATH_MAX, 
                "%s/cgroup.subtree_control", postgres_instance_gr_path) <= 0){
        return 6;
    }

    // create postgres
    if( mkdir(cgroups_postgres_root, 0755) != 0 ){
        if(errno != EEXIST){
            return 7;
        }
    }
    // write to postgres/cgroup.subtree_contol
    fprintf(debug, "%s %d\n", postgres_subtree_path, atoi(argv[3]));
    f_subtree_f = fopen(postgres_subtree_path, "w");
    if(f_subtree_f == NULL){
        return 11;
    }
    for(int i = 0; i < atoi(argv[3]); ++i){
        if( fprintf(f_subtree_f, "+%s", argv[4+i]) <= 0){
            return 14;
        }
        if( fflush(f_subtree_f) != 0){
            return 23;
        }
    }
    if( fclose(f_subtree_f) != 0 ){
        fprintf(debug, "%s\n", strerror(errno));
        return 22;
    }

    //create postgres/PID delegated to UID  
    if( mkdir(postgres_instance_gr_path, 0755) != 0 ){
        return 8;
    }
    if( chown(postgres_instance_gr_path, uid, uid) != 0 ){
        return 9;
    }
    if( chown(postgres_instance_cgroup_procs_path, uid, uid) != 0 ){
        return 10;
    }

    // write to postgres/PID/cgroup.subtree_control
    f_subtree_f = fopen(postgres_instance_subtree_path, "w");
    if(f_subtree_f == NULL){
        return 11;
    }
    for(int i = 0; i < atoi(argv[3]); ++i){
        fprintf(debug, "%s\n", argv[4+i]);
        if( fprintf(f_subtree_f, "+%s", argv[4+i]) <= 0){
            return 14;
        }
        if( fflush(f_subtree_f) != 0){
            return 23;
        }
    }
    if( fclose(f_subtree_f) != 0 ){
        fprintf(debug, "%s\n", strerror(errno));
        return 20;
    }

    // create postgres/PID/postmaster
    if( mkdir(postgres_postmaster_gr_path, 0755) != 0 ){
        return 11;
    }
    if( chown(postgres_postmaster_gr_path, uid, uid) != 0 ){
        return 12;
    }

    // emplace postmaster process to postgres/PID/postmaster cgroup
    f_postmaster_gr = fopen(postgres_postmaster_cgroup_procs_path, "w");
    if(f_postmaster_gr == NULL){
        return 13;
    }
    if( fprintf(f_postmaster_gr, "%d", pid) <= 0){
        return 14;
    }
    if( fclose(f_postmaster_gr) != 0 ){
        return 15;
    }

    free(postgres_instance_gr_path);
    free(postgres_postmaster_gr_path);
    free(postgres_postmaster_cgroup_procs_path);
    free(postgres_instance_cgroup_procs_path);
    free(postgres_instance_subtree_path);
    free(postgres_subtree_path);

    return 0;
}