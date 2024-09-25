#include <unistd.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <linux/limits.h>

#include "postgres.h"
#include "common/file_perm.h"
#include "libpq/auth.h"
#include "fmgr.h"
#include "postmaster/bgworker.h"

#include "libcgroup/libcgroup.h"
#include "config/group_config.h"


#include <syscall.h>
#include <sched.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/mount.h>
#include <sys/wait.h>
#include <errno.h>

PG_MODULE_MAGIC;

ClientAuthentication_hook_type old_auth_hook; 
BgworkerJustBeforeMain_hook_type old_bgworker_hook;


void _PG_init(void);  

static void bgworker_isolate(BackgroundWorker *bgworkerEntry);
static void backend_auth_isolate(Port *port, int n);
static int init_cgroup(int pid, int uid);

static int postmaster_instance_pid;

void
_PG_init(void) 
{
	limitation_node_t* first_node;
	elog(LOG, "module launched");

	// This hook is going to be called in the end of backend initialization 
	// in purpose to insert code moving the process into other cgroup.
	old_auth_hook = ClientAuthentication_hook; 
	ClientAuthentication_hook = backend_auth_isolate;

	old_bgworker_hook = BgworkerJustBeforeMain_hook;
	BgworkerJustBeforeMain_hook = bgworker_isolate;

	// _PG_init is called in postmaster, so we can get pid of it. 
	postmaster_instance_pid = getpid();

	if( init_cgroup(getpid(), getuid()) != 0 ){
		elog(ERROR, "cgroups initialization error");
	} else {
		elog(LOG, "cgroups succesfully initialized");
	}

	if((first_node = parse_config()) == NULL)
	{
		elog(LOG, "pg_isolate: config parse error");
	}

	create_cgroups(postmaster_instance_pid, first_node);

	elog(LOG, "module main end");	
}


/*
 * Creates a new process that have enough privileges to create new cgroup 
 * and move postmaster into it. This executable are created in build script
 * and has suid bit.
 */
int 
init_cgroup(int pid, int uid)
{
	pid_t fork_pid = fork();
	mode_t old_mask;

	if(fork_pid == -1){
		elog(ERROR, "pg_isolate: fork for cgroup_init error");
		return -1;
	}

	old_mask = umask(0);

	if(fork_pid == 0){
		#define max_length 32
		char* args[7];
		char pid_arg[max_length];
		char uid_arg[max_length];

		snprintf(pid_arg, max_length, "%d", pid); 
		snprintf(uid_arg, max_length, "%d", uid); 

		args[0] = "cgroup_initialize";
		args[1] = pid_arg;
		args[2] = uid_arg;
		args[3] = "2";		//  controllers that must
		args[4] = "memory"; //  be enabled
		args[5] = "cpu";    //  in child cgroup
		args[6] = NULL;

		execvp("cgroup_initialize", args);

		elog(ERROR, "pg_isolate cgroup_initialize. Have you written 'make cgroup_init'?");
	} else {
		int status = 0;

		wait(&status);
		if(WEXITSTATUS(status) != 0){
			elog(ERROR, "pg_isolate: cgroups_initialize forked proccess error %d", WEXITSTATUS(status));
			return -1;
		}
	}

	(void) umask(old_mask);

	return 0;
}

void backend_auth_isolate(Port *port, int status)
{
	const limitation_node_t* group;

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

	group = get_group_for(port->database_name, port->user_name);
	if(group){
		cg_enter(postmaster_instance_pid, group->group_name);
		elog(LOG, "pg_isolate: curr cgroup %s", cg_get_current());
	}
}

void 
bgworker_isolate(BackgroundWorker *bgw)
{
	if(old_bgworker_hook){
		old_bgworker_hook(bgw);
	}

	elog(LOG, "new bgworker (%s\n%s\n%s\n%s) euid: %d, uid:%d. pid %d ppid %d", 
		bgw->bgw_name, bgw->bgw_type, bgw->bgw_library_name, bgw->bgw_function_name,
		geteuid(), getuid(),
		getpid(), getppid()
	);

	if(strcmp(bgw->bgw_library_name, "test_module") == 0) {
		elog(LOG, "isolating %s", bgw->bgw_library_name);
		if(unshare(CLONE_NEWNS)) {
			elog(ERROR, "unshare");
			return;
		} 

		if (syscall(SYS_pivot_root, "/home/extio1/alpine", "/home/extio1/alpine/oldroot") != 0) {
			char* error = strerror(errno);
			elog(ERROR, "pivot_root failed %s", error);
			return;
		}

		if (chdir("/") != 0){
			elog(ERROR, "chdir failed");
			return;
		}
    }
}