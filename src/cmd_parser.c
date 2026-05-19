#include "../include/mshell.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <stdio.h>

static char* generate_full_path(const char *path)
{
        if (!path) return NULL;

        struct passwd *pw = getpwuid(getuid());
        if (!pw) return NULL;

        char *fpath = malloc(strlen(pw->pw_dir) + strlen(path) + 2);
        if (!fpath) return NULL;

        sprintf(fpath, "%s/%s", pw->pw_dir,  path + 1); // 0 element is tilda '~'

        return fpath;
}

int expand_cmd(cmd_t *cmd)
{
        if (!cmd) return -1;

        for (int i = 0; i < cmd->argc; i++) {
                if (cmd->argv[i][0] == '~' && (cmd->argv[i][1] == '/' || cmd->argv[i][1] == '\0')) {
                        char *fpath = generate_full_path(cmd->argv[i]);
                        if (!fpath) continue;

                        free(cmd->argv[i]);
                        cmd->argv[i] = fpath;
                }
        }

        return 0;
}