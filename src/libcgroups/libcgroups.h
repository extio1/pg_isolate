#ifndef H_LIBCGROUPS
#define H_LIBCGROUPS

// СДЕЛАТЬ:
// добавление контроллеров для backend по запросу
// ?ограничение число форкающихся процессов через параметр конфига?
// дописать библиотеку для взаимодействия с cgroup через фс

/*
 * Must be called before usage of all function from 
 * the library.
 */
extern int cg_init(void);

/*
 * Move process into cgroups nested directory 
 * and create it if doesn't exitst.
 * 
 * e.g. process now in /1grp/2grp/
 *      cg_enter("3grp") will move to /1grp/2grp/3grp
 */
extern int cg_enter(const char *name); 


/*
 * Return current cgroups name. 
 */
extern int cg_get_current_gr(char**);

extern int cg_set(const char* controller);
extern int cg_get(const char*);

#endif /* H_LIBCGROUPS */