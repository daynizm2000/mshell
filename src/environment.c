#include "../include/mshell.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>


static long env_map_hash(const char *key)
{
        if (!key) return -1;

        unsigned long hash = 5381;
        int c;

        while ((c = *key++)) {
                hash = ((hash << 5) + hash) + c;
        }

        return hash;
}

static int env_map_rehash(struct env_map *map)
{
        size_t new_capacity = map->capacity * 2;

        env_map_node_t **new_buckets = calloc(new_capacity, sizeof(env_map_node_t*));
        if (!new_buckets) return -1;

        for (size_t i = 0; i < map->capacity; i++) {
                env_map_node_t *current = map->buckets[i];
                
                while (current != NULL) {
                        env_map_node_t *next_node = current->next;
                
                        size_t new_index = env_map_hash(current->env.key) % new_capacity;
                
                        current->next = new_buckets[new_index];
                        new_buckets[new_index] = current;
                
                        current = next_node;
                }
        }

        free(map->buckets);
        map->buckets = new_buckets;
        map->capacity = new_capacity;

        return 0;
}


static int env_copy(env_t *dst, const env_t *src, int new)
{
        if (!dst || !src) return -1;

        if (new) {
                if (!(dst->key = strdup(src->key))) goto fail;
        }

        if (!(dst->value = strdup(src->value))) goto fail;
        
        dst->is_exported = src->is_exported;

        return 0;
fail:
        if (dst->key)   free(dst->key);
        if (dst->value) free(dst->value);
        dst->is_exported = 0;

        return -1;
}

static void env_destroy(env_t *env)
{
        if (!env) return;

        free(env->key);
        free(env->value);
        env->is_exported = 0;
}

int env_map_init(struct env_map *map)
{
        if (!map) return -1;

        map->capacity   = 128;
        map->count      = 0;
        map->buckets    = calloc(map->capacity, sizeof(env_map_node_t*));

        return (!map->buckets) ? -1 : 0;
}

int env_map_copy_envp(struct env_map *map, const char** const envp)
{
        if (!map || !envp) return -1;

        for (const char** s_env = envp; *s_env; s_env++) {
                const char *sep = strchr(*s_env, '=');
                if (!sep) continue;
                
                env_t env = {.is_exported = 1};

                if (!(env.key = strndup(*s_env, sep - *s_env))) {
                        return -1;
                }

                if (!(env.value = strdup(sep + 1))) {
                        free(env.key);
                        return -1;
                }

                if (env_map_add(map, &env) < 0) {
                        return -1;
                }

                free(env.key);
                free(env.value);
        }

        return 0;
}

char** env_map_generate_envp(struct env_map *map)
{
        if (!map) return NULL;

        size_t envp_size = 0;

        for (size_t i = 0, j = 0; i < map->capacity && j < map->count; i++) {
                env_map_node_t *node = map->buckets[i];

                while (node) {
                        if (node->env.is_exported) envp_size++;

                        node = node->next;
                        j++;
                }
        }

        char** const envp = malloc((envp_size + 1) * sizeof(char*));
        if (!envp) return NULL;

        size_t envp_idx = 0;

        for (size_t i = 0; i < map->capacity && envp_idx < envp_size; i++) {
                env_map_node_t *node = map->buckets[i];

                while (node) {
                        if (node->env.is_exported) {
                                size_t s_env_size = snprintf(NULL, 0, "%s=%s", node->env.key, node->env.value);

                                envp[envp_idx] = malloc(s_env_size + 1);

                                if (!envp[envp_idx]) {
                                        for (size_t k = 0; k < envp_idx; k++) {
                                                free(envp[k]);
                                        }

                                        free(envp);
                                        return NULL;
                                }

                                sprintf(envp[envp_idx], "%s=%s", node->env.key, node->env.value);
                                envp_idx++;
                        }

                        node = node->next;
                }
        }

        envp[envp_idx] = NULL;

        return envp;
}

int env_map_add(struct env_map *map, const env_t *env)
{
        if (!map || !env) return -1;

        if ((double)map->count / map->capacity >= 0.75) {
                if (env_map_rehash(map) < 0) return -1;
        }

        long idx = env_map_hash(env->key) % map->capacity;
        if (idx < 0) return -1;

        env_map_node_t **node = &map->buckets[idx];
        
        while (*node) {
                if (strcmp((*node)->env.key, env->key) == 0) {
                        if (env_copy(&(*node)->env, env, 0) < 0) {
                                free(*node);
                                return -1;
                        }

                        if (env->is_exported) {
                                setenv(env->key, env->value, 1);
                        }

                        return 0;
                }

                node = &(*node)->next;
        }
        
        *node = malloc(sizeof(env_map_node_t));
        if (!*node) return -1;

        (*node)->next = NULL;

        if (env_copy(&(*node)->env, env, 1) < 0) {
                free(*node);
                return -1;
        }

        if (env->is_exported) {
                setenv(env->key, env->value, 0);
        }

        map->count++;

        return 0;
}

env_t* env_map_find(struct env_map *map, const char *key)
{
        if (!map || !key) return NULL;
       
        long idx = env_map_hash(key) % map->capacity;
        if (idx < 0) return NULL;

        env_map_node_t *node = map->buckets[idx];

        while (node) {
                if (strcmp(node->env.key, key) == 0) {
                        return &node->env;
                }

                node = node->next;
        }

        return NULL;
}

int env_map_delete(struct env_map *map, const char *key)
{
        if (!map || !key) return -1;

        long idx = env_map_hash(key) % map->capacity;
        if (idx < 0) return -1;

        env_map_node_t **node = &map->buckets[idx];

        while (*node) {
                if (strcmp((*node)->env.key, key) == 0) {
                        env_map_node_t *tmp = *node;

                        *node = (*node)->next;
                        
                        if (tmp->env.is_exported) {
                                unsetenv(tmp->env.key);
                        }
                        
                        env_destroy(&tmp->env);
                        free(tmp);

                        break;
                }

                node = &(*node)->next;
        }

        return 0;
}

void env_map_print(struct env_map *map)
{
        if (!map) return;

        for (size_t i = 0; i < map->capacity; i++) {
                if (!map->buckets[i]) continue;

                for (env_map_node_t *node = map->buckets[i]; node != NULL; node = node->next) {
                        printf("%s=%s\n", node->env.key, node->env.value);
                }
        }
}

void env_map_destroy(struct env_map *map)
{
        if (!map) return;

        for (size_t i = 0, j = 0; i < map->capacity && j < map->count; i++) {
                env_map_node_t **node = &map->buckets[i];

                while (*node) {
                        env_map_node_t *tmp = *node;

                        *node = (*node)->next;
                        
                        env_destroy(&tmp->env);
                        free(tmp);
                        
                        j++;
                }
        }

        free(map->buckets);
        map->capacity = 0;
        map->count    = 0;
}