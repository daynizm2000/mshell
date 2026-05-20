#include "../include/mshell.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pwd.h>
#include <unistd.h>
#include <fcntl.h>
#include <ctype.h>

static void delete_spaces(char *str)
{
        if (!str) return;

        char *src  = str;
        char *dest = str;

        while (*src) {
                if (isspace(*src)) {
                        src++;
                }
                else {
                        *dest++ = *src++;
                }
        }

        *dest = '\0';
}

static char *get_distr(void)
{
        int fd = open("/etc/os-release", O_RDONLY);
        if (fd < 0) return NULL;

        char buffer[4096];
        ssize_t read_bytes     = 0;
        size_t  total_bytes    = 0;

        while ((read_bytes = read(fd, buffer + total_bytes, sizeof(buffer) - total_bytes)) > 0) {
                if (total_bytes + read_bytes >= sizeof(buffer)) {
                        if (total_bytes > (sizeof("NAME=") - 1)) {
                                memmove(buffer, buffer + total_bytes - (sizeof("NAME=") - 1), sizeof("NAME=") - 1);
                        }

                        total_bytes = sizeof("NAME=") - 1;
                }
                else {
                        total_bytes += read_bytes;
                }

                const char *dname_field = memmem(buffer, total_bytes, "NAME=", sizeof("NAME=") - 1);
                if (!dname_field || (dname_field != buffer && dname_field[-1] != '\n')) continue;

                if (dname_field) {
                        dname_field += sizeof("NAME=") - 1; // we shift it so that the beginning is after the '=' sign

                        if (*dname_field == '"' || *dname_field == '\'') {
                                dname_field++;
                        }

                        size_t copy_count = 0;

                        while (&dname_field[copy_count] != &buffer[total_bytes] && dname_field[copy_count] != '\'' 
                                && dname_field[copy_count] != '"' && dname_field[copy_count] != '\n') {
                                        copy_count++;
                                }

                        char *res = strndup(dname_field, copy_count);

                        if (!res) {
                                close(fd);
                                return NULL;
                        }

                        delete_spaces(res);
                        close(fd);

                        return res;
                }
        }

        close(fd);
        return NULL;
}

static char *get_uname(void)
{
        struct passwd *pwd = getpwuid(getuid());

        return (pwd && pwd->pw_name) ? strdup(pwd->pw_name) : NULL;
}

static char* get_home_path(void)
{
        struct passwd *pwd = getpwuid(getuid());

        if (!pwd || !pwd->pw_name) return NULL;
                
        size_t rlen = snprintf(NULL, 0, "/home/%s", pwd->pw_name);
        char *res = malloc(rlen + 1);
        if (!res) return NULL;

        sprintf(res, "/home/%s", pwd->pw_name);
                
        return res;
}

#define GET_PATH__GO_EXIT(ret, retval) \
        do { \
                ret = retval; \
                goto cleanup; \
        } while (0);

static char* get_path(void)
{
        char *ret = NULL;

        char *path = getcwd(NULL, 0);
        if (!path) GET_PATH__GO_EXIT(ret, NULL);

        char *home_path = get_home_path();
        if (!home_path) GET_PATH__GO_EXIT(ret, NULL);

        size_t len_home = strlen(home_path);

        if (strncmp(path, home_path, len_home) == 0 && (path[len_home] == '/' || path[len_home] == '\0')) {
                if (path[len_home] == '\0') GET_PATH__GO_EXIT(ret, strdup("~"));

                size_t newlen = snprintf(NULL, 0, "~/%s", path + len_home + 1);
                char *res = malloc(newlen + 1);
                if (!res) GET_PATH__GO_EXIT(ret, NULL);

                sprintf(res, "~/%s", path + len_home + 1);
                GET_PATH__GO_EXIT(ret, res);
        }
        else {
                GET_PATH__GO_EXIT(ret, strdup(path));
        }

cleanup:
        if (path)       free(path);
        if (home_path)  free(home_path);

        return ret;
}

static char* get_res(const prompt_t *prompt)
{
        char *res = malloc(strlen(prompt->distr) + strlen(prompt->uname) + strlen(prompt->path) + 16);
        if (!res) return NULL;

        sprintf(res, "%s@%s %s> ", prompt->distr, prompt->uname, prompt->path);
        return res;
}

int prompt_init(prompt_t *prompt)
{
        if (!prompt) return -1;

        if (!(prompt->distr = get_distr()))     goto fail;
        if (!(prompt->uname = get_uname()))     goto fail;
        if (!(prompt->path  = get_path()))      goto fail;
        if (!(prompt->res   = get_res(prompt))) goto fail;

        return 0;
fail:
        prompt_destroy(prompt);
        return -1;
}

void prompt_print(prompt_t prompt)
{
        printf("%s@%s %s> ", prompt.distr, prompt.uname, prompt.path);
}

void prompt_path_update(prompt_t *prompt)
{
        if (!prompt) return;

        if (prompt->path) free(prompt->path);
        if (prompt->res)  free(prompt->res);

        prompt->path = get_path();
        prompt->res  = get_res(prompt);
}

void prompt_destroy(prompt_t *prompt)
{
        if (!prompt) return;

        if (prompt->distr) free(prompt->distr);
        if (prompt->path)  free(prompt->path);
        if (prompt->uname) free(prompt->uname);
        if (prompt->res)   free(prompt->res);
}