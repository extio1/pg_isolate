#include <unistd.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/limits.h>

#include "postgres.h"
#include "common/file_perm.h"
#include "libpq/auth.h"
#include "fmgr.h"

#include "libcgroups/libcgroups.h"

PG_MODULE_MAGIC;

ClientAuthentication_hook_type old_auth_hook; 

void _PG_init(void);  

static void backend_auth_isolate(Port *port, int n);
static int init_cgroup(int pid, int uid);

void
_PG_init(void) // is called in postmaster
{
	elog(LOG, "module launched");

	old_auth_hook = ClientAuthentication_hook; 
	ClientAuthentication_hook = backend_auth_isolate;

	if( init_cgroup(getpid(), getuid()) != 0 ){
		elog(ERROR, "cgroups initialization error");
	} else {
		elog(LOG, "cgroups succesfully initialized");
	}

	elog(LOG, "module main end");	
}

int 
init_cgroup(int pid, int uid)
{
	pid_t fork_pid = fork();
	if(fork_pid == -1){
		elog(ERROR, "pg_isolate: fork for cgroup_init error");
		return -1;
	}

	if(fork_pid == 0){
		#define max_length 32
		char* args[4];
		char pid_arg[max_length];
		char uid_arg[max_length];
		mode_t old_mask;

		snprintf(pid_arg, max_length, "%d", pid); 
		snprintf(uid_arg, max_length, "%d", uid); 

		args[0] = "cgroups_initialize";
		args[1] = pid_arg;
		args[2] = uid_arg;
		args[3] = NULL;

		old_mask = umask(0);

		execvp("cgroups_initialize", args);

		(void) umask(old_mask);

		elog(ERROR, "pg_isolate cgroups_initialize. Have you written 'make cgroups_init'?");
	} else {
		int status = 0;
		wait(&status);
		if(WEXITSTATUS(status) != 0){
			elog(ERROR, "pg_isolate: cgroup_init forked proccess error");
			return -1;
		}
	}

	return 0;
}

void backend_auth_isolate(Port *port, int status)
{
	if(old_auth_hook){
		old_auth_hook(port, status);
	}

	elog(LOG, "[%d] NEW BACKEND:"
              "\ndatabase: %s\nuser: %s\naplication_name %s"
              "\nremote_host %s\nremote_hostname: %s\n, n param from hook: %d\n", 
        getpid(),
        port->database_name,
        port->user_name,
        port->application_name,
        port->remote_host,
        port->remote_hostname,
		status
	);

	if( cg_init() != 0){
		elog(ERROR, "pg_isolate: libcgroups initialization error");
	} else {
		elog(DEBUG1, "pg_isolate: libcgroups succesfully initialized");
	}

	cg_enter("test_backend");
	char* curr_cgroup;
	
	cg_get_current_gr(&curr_cgroup);
	elog(LOG, "curr cgroup %s", curr_cgroup);

	// char *backend_cgroup_path = malloc(strlen(cgroup_dir)+10); // 10 for pid
	// char *backend_cgroup_path_procs = malloc(strlen(cgroup_dir)+strlen("/cgroup.procs")+10);
	
	// sprintf(backend_cgroup_path, "%s/%d", cgroup_dir, getpid());

	// if( mkdir(backend_cgroup_path, 0755) != 0 ){
	// 	elog(LOG, "mkdir error");
	// 	return;
	// }

	// sprintf(backend_cgroup_path_procs, "%s/%d/cgroup.procs", cgroup_dir, getpid());

	// FILE *f = fopen(backend_cgroup_path_procs, "w");

	// if(f == NULL){
	// 	elog(LOG, "fopen error");
	// 	return;
	// }

	// fprintf(f, "%d", getpid());

	// fclose(f);
	// free(backend_auth_isolate);
	// free(backend_cgroup_path_procs);
}