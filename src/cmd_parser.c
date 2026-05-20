#include "../include/mshell.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <stdio.h>


void parse_tilda(cmd_t *cmd, int pos);
void parse_variable(cmd_t *cmd, int pos);


static void (*g_parse_handlers[])(cmd_t*, int pos) = {
        parse_tilda,
        parse_variable
};


void parse_tilda(cmd_t *cmd, int pos)
{
        const char *arg = cmd->argv[pos];

        if (!cmd || !arg) return;
        if (!(arg[0] == '~' && (arg[1] == '/' || arg[1] == '\0'))) return;

        struct passwd *pw = getpwuid(getuid());
        if (!pw) return;

        size_t fpath_size = snprintf(NULL, 0, "%s/%s", pw->pw_dir, arg + 1); // 0 arg element is tilda '~'
        char *fpath = malloc(fpath_size + 1);
        if (!fpath) return;

        sprintf(fpath, "%s/%s", pw->pw_dir,  arg + 1);

        free(cmd->argv[pos]);
        cmd->argv[pos] = fpath;
}

void parse_variable(cmd_t *cmd, int pos)
{
        const char *arg = cmd->argv[pos];

        if (!cmd || !arg) return;
        if (*arg != '$')  return;

        env_t *var = env_map_find(&g_env_map, arg + 1);

        if (!var) {
                free(cmd->argv[pos]);
                cmd->argv[pos] = strdup("");

                return;
        }

        free(cmd->argv[pos]);
        cmd->argv[pos] = strdup(var->value);
}

int expand_cmd(cmd_t *cmd)
{
        if (!cmd) return -1;

        for (int i = 0; i < cmd->argc; i++) {
                for (size_t j = 0; j < sizeof(g_parse_handlers) / sizeof(*g_parse_handlers); j++) {
                        g_parse_handlers[j](cmd, i);
                }
        }

        return 0;
}