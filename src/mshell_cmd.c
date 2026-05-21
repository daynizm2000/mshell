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
                env_map_print(&g_mshell_state.env_map);
                return 0;
        }

        for (int i = 1; i < cmd->argc; i++) {
                const char *sep = strchr(cmd->argv[i], '=');

                if (!sep) {
                        env_t *env = env_map_find(&g_mshell_state.env_map, cmd->argv[i]);
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

                if (env_map_add(&g_mshell_state.env_map, &env) < 0) {
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
                env_map_delete(&g_mshell_state.env_map, cmd->argv[i]);
        }

        return 0;
}

int mshell_cmd_cd(cmd_t *cmd)
{
        if (!cmd || !cmd->argv[1]) return -1;

        char *new_path = cmd->argv[1];

        if (cmd->argv[1][0] == '-' && cmd->argv[1][1] == '\0') {
                if (g_mshell_state.last_cd_path) {
                        printf("%s\n", g_mshell_state.last_cd_path);

                        char *fpath = parse_tilda(g_mshell_state.last_cd_path);

                        if (fpath) {
                                free(g_mshell_state.last_cd_path);
                                g_mshell_state.last_cd_path = fpath;
                        }

                        new_path = g_mshell_state.last_cd_path;
                }
        }

        char *last_path = strdup(g_mshell_state.prompt.path);
        if (!last_path) return -1;

        int res = chdir(new_path);

        if (res < 0) {
                free(last_path);
                return res;
        }

        if (g_mshell_state.last_cd_path) {
                free(g_mshell_state.last_cd_path);
        }

        g_mshell_state.last_cd_path = last_path;
        prompt_path_update(&g_mshell_state.prompt);
        
        return res;
}