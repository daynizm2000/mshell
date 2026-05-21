#include "include/mshell.h"
#include <stdio.h>
#include <pwd.h>
#include <string.h>
#include <signal.h>
#include <fcntl.h>
#include <stdlib.h>
#include <unistd.h>
#include <readline/readline.h>
#include <readline/history.h>

#define CFG_FNAME ".mshellrc"
#define RDF_DEFCAP 4096

struct mshell_state          g_mshell_state;
static volatile sig_atomic_t g_mainloop_running = 1;

static void sig_handler(int sig)
{
        if (sig == SIGTERM) {
                g_mainloop_running = 0;
                kill(g_mshell_state.fg_pid, SIGTERM);
        }
        else if (sig == SIGINT) {
                if (g_mshell_state.fg_pid > 0) {
                        kill(g_mshell_state.fg_pid, SIGINT);
                        g_mshell_state.fg_pid = -1;
                }
        }
}

static char* read_file(int fd)
{
        if (fd < 0) return NULL;

        size_t capacity = RDF_DEFCAP;

        char *buffer = malloc(capacity);
        if (!buffer) return NULL;

        ssize_t read_bytes = 0;
        size_t total_bytes = 0;

        while ((read_bytes = read(fd, buffer + total_bytes, capacity - total_bytes)) > 0) {
                total_bytes += read_bytes;

                if (total_bytes + 1 >= capacity) {
                        char *tmp = realloc(buffer, capacity * 2);

                        if (!tmp) {
                                free(buffer);
                                return NULL;
                        }

                        buffer = tmp;
                        capacity *= 2;
                }
        }

        if (read_bytes < 0) {
                free(buffer);
                return NULL;
        }

        buffer[total_bytes] = '\0';

        return buffer;
}

int mshell_exec(char *args)
{
        struct tokenlist *tok_head = tokenize(args);

        if (!tok_head) {
                return -1;
        }

        struct joblist *jb_head = jobs_init(tok_head);

        if (!jb_head) {
                toklist_free(tok_head);
                return -1;
        }

        toklist_free(tok_head);

        int res = execute(jb_head);

        jobs_free(jb_head);

        return (res) ? -1 : 0;
}

int runcfg(void)
{
        struct passwd *pw = getpwuid(getuid());
        if (!pw) return -1;

        char path[strlen(pw->pw_dir) + strlen(CFG_FNAME) + 2];
        sprintf(path, "%s/%s", pw->pw_dir, CFG_FNAME);

        int fd = open(path, O_RDONLY);
        if (fd < 0) return -1;

        char *data = read_file(fd);

        if (!data) {
                close(fd);
                return -1;
        }

        close(fd);
        
        return mshell_exec(data);
}

int environment_init(const char** const envp)
{
        if (env_map_init(&g_mshell_state.env_map) < 0) {
                return -1;
        }

        if (env_map_copy_envp(&g_mshell_state.env_map, envp) < 0) {
                env_map_destroy(&g_mshell_state.env_map);
                return -1;
        }

        return 0;
}

int mshell_state_init(struct mshell_state *state, const char** const envp)
{
        if (!state) return -1;

        state->fg_pid           = -1;
        state->last_status_code = 0;
        state->last_cd_path     = NULL;

        if (environment_init(envp) < 0)           goto fail;
        if (prompt_init(&state->prompt) < 0)      goto fail;
        if (!(state->last_cd_path = strdup("~"))) goto fail;

        return 0;
fail:
        env_map_destroy(&state->env_map);
        prompt_destroy(&state->prompt);

        if (state->last_cd_path) free(state->last_cd_path);
        state->last_cd_path = NULL;

        return -1;
}

void mshell_state_destroy(struct mshell_state *state)
{
        if (!state) return;

        env_map_destroy(&state->env_map);
        prompt_destroy(&state->prompt);

        if (state->last_cd_path) free(state->last_cd_path);
        
        state->last_status_code = 0;
        state->fg_pid           = -1;
}

int main(int argc __attribute__((unused)), const char** const argv __attribute__((unused)), const char** const envp)
{
        if (mshell_state_init(&g_mshell_state, envp) < 0) {
                perror("mshell");
                return 1;
        }

        runcfg();

        struct sigaction sa = {0};
        sa.sa_handler = sig_handler;
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);

        while (g_mainloop_running) {
                char *uinput = readline(g_mshell_state.prompt.res);

                if (!uinput) {
                        break;
                }

                if (strcmp(uinput, "exit") == 0) {
                        free(uinput);
                        break;
                }

                add_history(uinput);

                mshell_exec(uinput);

                free(uinput);
        }

        mshell_state_destroy(&g_mshell_state);

        return 0;
}