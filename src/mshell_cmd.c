#include "../include/mshell.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>

mshell_cmd_t mshell_commands[] = {
        {"cd",     mshell_cmd_cd},
        {"export", mshell_cmd_export},
        {"unset",  mshell_cmd_unset}
};

mshell_cmd_t* mshell_findcmd(const char *arg)
{
        if (!arg) return NULL;

        for (size_t i = 0; i < sizeof(mshell_commands) / sizeof(*mshell_commands); i++) {
                if (strcmp(mshell_commands[i].name, arg) == 0) {
                        return &mshell_commands[i];
                }
        }

        return NULL;
}

int mshell_cmd_export(cmd_t *cmd)
{
        if (!cmd) return -1;

        if (cmd->argc == 1) {
                env_map_print(&g_env_map);
                return 0;
        }

        for (int i = 1; i < cmd->argc; i++) {
                const char *sep = strchr(cmd->argv[i], '=');

                if (!sep) {
                        env_t *env = env_map_find(&g_env_map, cmd->argv[i]);
                        if (!env) continue;

                        env->is_exported = 1;
                        setenv(env->key, env->value, 0);
                        
                        continue;
                }
                
                env_t env = {.is_exported = 1};

                if (!(env.key = strndup(cmd->argv[i], sep - cmd->argv[i]))) {
                        perror("mshell");
                        continue;
                }

                if (!(env.value = strdup(sep + 1))) {
                        free(env.key);
                        perror("mshell");
                        continue;
                }

                if (env_map_add(&g_env_map, &env) < 0) {
                        perror("mshell");
                }

                free(env.key);
                free(env.value);
        }

        return 0;
}

int mshell_cmd_unset(cmd_t *cmd)
{
        if (!cmd) return -1;

        for (int i = 1; i < cmd->argc; i++) {
                env_map_delete(&g_env_map, cmd->argv[i]);
        }

        return 0;
}

int mshell_cmd_cd(cmd_t *cmd)
{
        if (!cmd) return -1;
        
        return chdir(cmd->argv[1]);
}