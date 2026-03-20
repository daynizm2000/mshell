#include "include/prompt.h"
#include "include/lexer.h"
#include "include/joblist.h"
#include "include/executor.h"
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

volatile sig_atomic_t mainloop_running = 1;

static void sig_handler(int sig)
{
        if (sig == SIGTERM) {
                mainloop_running = 0;
                kill(g_fg_pid, SIGTERM);
        }
        else if (sig == SIGINT) {
                kill(g_fg_pid, SIGINT);
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

int main(void)
{
        prompt_t prompt;
        prompt_init(&prompt);

        runcfg();

        struct sigaction sa = {0};
        sa.sa_handler = sig_handler;
        sigaction(SIGINT, &sa, NULL);
        sigaction(SIGTERM, &sa, NULL);

        while (mainloop_running) {
                char *uinput = readline(prompt.res);

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
                prompt_path_update(&prompt);
        }

        prompt_free(&prompt);

        return 0;
}