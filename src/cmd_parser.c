#include "../include/mshell.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <stdio.h>


void expand_parse_tilda(cmd_t *cmd, int pos);
void expand_parse_variable(cmd_t *cmd, int pos);


void (*g_parse_handlers[])(cmd_t*, int pos) = {
        expand_parse_tilda,
        expand_parse_variable
};


char* parse_tilda(char *str)
{
        if (!(str[0] == '~' && (str[1] == '/' || str[1] == '\0'))) return NULL;

        struct passwd *pw = getpwuid(getuid());
        if (!pw) return NULL;

        size_t fpath_size = snprintf(NULL, 0, "%s/%s", pw->pw_dir, str + 1); // 0 arg element is tilda '~'
        char *fpath       = malloc(fpath_size + 1);
        if (!fpath) return NULL;

        sprintf(fpath, "%s/%s", pw->pw_dir,  str + 1);

        return fpath;
}

char* parse_variable(char *arg)
{
        if (*arg != '$')  return NULL;

        if (arg[1] == '?' && arg[2] == '\0') {
                char res[11];
                sprintf(res, "%d", g_mshell_state.last_status_code);

                return strdup(res);
        }

        env_t *var = env_map_find(&g_mshell_state.env_map, arg + 1);

        if (!var) {
                return strdup("");
        }

        return strdup(var->value);
}

void expand_parse_tilda(cmd_t *cmd, int pos)
{
        const char *arg = cmd->argv[pos];

        if (!cmd || !arg) return;
        
        char *fpath = parse_tilda(cmd->argv[pos]);
        if (!fpath) return;

        free(cmd->argv[pos]);
        cmd->argv[pos] = fpath;
}

void expand_parse_variable(cmd_t *cmd, int pos)
{
        if (!cmd || !cmd->argv[pos]) return;
        
        char *res = parse_variable(cmd->argv[pos]);
        if (!res) return;

        free(cmd->argv[pos]);
        cmd->argv[pos] = res;
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