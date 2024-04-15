#include "postgres.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "group_config.h"
#include "json_parser/cJSON.h"
#include "../libcgroup/libcgroup.h"

static limitation_node_t *first; 

const limitation_node_t * 
get_group_for(const char *database, const char *user)
{
    limitation_node_t* curr = first;
    limitation_node_t* next;

    bool find = false;
    limitation_node_t* default_node = NULL;
    limitation_node_t* result_node = NULL;

    while(curr != NULL) {
        bool same_database = !strcmp(curr->database, database);
        bool same_user = !strcmp(curr->user, user); 
        
        if(same_database && same_user){
            result_node = curr;
            return result_node;
        } else if(same_database & !IS_STATED(curr->user) ||
                  same_user     & !IS_STATED(curr->database)  ) 
        {
            elog(LOG, "%s-%s %s-%s", curr->database, database, curr->user, user);
            find = true;
            result_node = curr;
        }

        if(!strcmp(curr->group_name, "default")){
            default_node = curr;
        }

        next = curr->next;
        curr = next;
    }
    
    if(!find && default_node){
        return default_node;
    }

    return result_node;
}

limitation_node_t*
limitation_node_init(void)
{
    limitation_node_t* node = malloc(sizeof(limitation_node_t));

    node->database   = calloc(128, sizeof(char));
    node->user       = calloc(128, sizeof(char));
    node->group_name = calloc(128, sizeof(char));

    node->cpu_weight = calloc(16, sizeof(char));
    node->memory_hard_limit = calloc(16, sizeof(char));
    node->memory_soft_limit = calloc(16, sizeof(char));

    node->next = NULL;

    return node;
}

void
limitation_list_free(limitation_node_t* node)
{
    limitation_node_t* curr = node;
    limitation_node_t* next;

    while(curr != NULL) {
        next = curr->next;
        free(curr->database);
        free(curr->user);
        free(curr->group_name);
        free(curr->cpu_weight);
        free(curr->memory_hard_limit);
        free(curr->memory_soft_limit);
        free(curr);
        curr = next;
    }
}

void
limitation_list_print(limitation_node_t* node)
{
    limitation_node_t* curr = node;
    limitation_node_t* next;

    while(curr != NULL) {
        next = curr->next;
        elog(LOG, "\n==%s==\nDatabase %s, user %s:\n memory_soft: %s\n memory_hard: %s\n cpu_weight: %s\n====\n", 
               curr->group_name,
               curr->database, curr->user, curr->memory_soft_limit, curr->memory_hard_limit,
               curr->cpu_weight);
        curr = next;
    }
}

limitation_node_t*
parse_config(void)
{
    FILE* f;
    int size;
    char* config;
    cJSON *root, *group;
    limitation_node_t* cgroup_first = limitation_node_init();
    limitation_node_t* curr_node = cgroup_first;

    f = fopen("pg_isolate.json", "r");

    fseek(f, 0, SEEK_END);
    size = ftell(f);
    fseek(f, 0, SEEK_SET);

    config = calloc(size, sizeof(char));

    if(fread(config, size, 1, f) == 0){
        if(ferror(f)){
            elog(ERROR, "pg_isolate: config file read error");
        }
        return NULL;
    }

    root = cJSON_Parse(config);
    cJSON_ArrayForEach(group, root)
    {
        if(group->string){
            cJSON* database = cJSON_GetObjectItem(group, "database"); 
            cJSON* user = cJSON_GetObjectItem(group, "user"); 
            cJSON* memory_hard = cJSON_GetObjectItem(group, "memory_hard");
            cJSON* memory_soft = cJSON_GetObjectItem(group, "memory_soft"); 
            cJSON* cpu_weigth = cJSON_GetObjectItem(group, "cpu_weight"); 

            strcpy(curr_node->group_name, group->string);

            if(cJSON_IsString(database))
                strcpy(curr_node->database, cJSON_GetStringValue(database));
           
            if(cJSON_IsString(user))
                strcpy(curr_node->user, cJSON_GetStringValue(user));

            if(cJSON_IsString(memory_hard))
                strcpy(curr_node->memory_hard_limit, cJSON_GetStringValue(memory_hard));
            else 
                strcpy(curr_node->memory_hard_limit, NOTSTATED);

            if(cJSON_IsString(memory_soft))  
                strcpy(curr_node->memory_soft_limit, cJSON_GetStringValue(memory_soft));
            else 
                strcpy(curr_node->memory_soft_limit, NOTSTATED);

            if(cJSON_IsString(cpu_weigth))
                strcpy(curr_node->cpu_weight, cJSON_GetStringValue(cpu_weigth));
            else 
                strcpy(curr_node->cpu_weight, NOTSTATED);

            if(group->next){ // if not last
                curr_node->next = limitation_node_init();
                curr_node = curr_node->next;
            }
        } else {
            printf("No name for cgroup\n");
        }
    }

    cJSON_Delete(root);
    fclose(f);

    first = cgroup_first;
    limitation_list_print(first);
    return cgroup_first;
}

int
create_cgroups(const int postmaster_pid, limitation_node_t* first)
{
    limitation_node_t* curr = first;
    limitation_node_t* next;

    while(curr != NULL) {
        next = curr->next;
        
        if( cg_create(postmaster_pid, curr->group_name) != 0 ){
            elog(WARNING, "cg_create group %s error", curr->group_name);
        }

        curr = next;
    }

    return 0;
}
