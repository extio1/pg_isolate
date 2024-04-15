#ifndef H_LIBCGROUPS
#define H_LIBCGROUPS

/*
 * Create cgroup with "name"
 */
extern int cg_create(const int postmaster_pid, const char *name); 

/*
 * Move process into cgroups nested directory 
 * and create it if doesn't exitst.
 */
extern int cg_enter(const int postmaster_pid, const char *name); 


/*
 * Return current cgroups path. 
 */
extern const char* cg_get_current(void);

/*
 * Writes to 'controller' file 'value'. 
 */
extern int cg_tune(const char* controller, const char* value);

#endif /* H_LIBCGROUPS */