#ifndef GROUP_CONFIG_H
#define GROUP_CONFIG_H

typedef struct limitation_node
{
    char* group_name;
    char* database;
    char* user;

    char* cpu_weight;

    char* memory_hard_limit;
    char* memory_soft_limit; 

    struct limitation_node* next;
} limitation_node_t;

extern limitation_node_t* limitation_node_init(void);
extern void limitation_list_free(limitation_node_t* node);
extern void limitation_list_print(limitation_node_t* node);

extern limitation_node_t* parse_config(void);

extern const limitation_node_t* get_group_for(const char *database, const char *user);

extern int create_cgroups(const int postmaster_pid, limitation_node_t* first);

#define NOTSTATED ""
#define IS_STATED(c) strcmp(c, NOTSTATED)

#endif /* GROUP_CONFIG_H */