#include "../include/joblist.h"
#include "../include/cmd_parser.h"
#include "../include/mshell_cmd.h"
#include "../include/executor.h"
#include <unistd.h>
#include <stdio.h>
#include <fcntl.h>
#include <sys/wait.h>
#include <string.h>
#include <stdlib.h>

volatile pid_t g_fg_pid;

static int pipes_init(int pipes[][2], size_t size)
{
        for (size_t i = 0; i < size; i++) {
                if (pipe(pipes[i]) < 0) {
                        for (size_t j = 0; j < i; j++) {
                                close(pipes[j][0]);
                                close(pipes[j][1]);
                        }

                        return -1;
                }
        }

        return 0;
}

static void pipes_close(int pipes[][2], size_t size)
{
        for (size_t i = 0; i < size; i++) {
                close(pipes[i][0]);
                close(pipes[i][1]);
        }
}

static int heredoc_process(const char *eof)
{
        int pfd[2];
        if (pipe(pfd) != 0) return -1;

        char *buff = NULL;
        size_t size = 0;

        while (getline(&buff, &size, stdin) >= 0) {
                char *nl = strrchr(buff, '\n');
                if (nl) *nl = '\0';

                if (strcmp(buff, eof) == 0) {
                        break;
                }

                ssize_t size_write = strlen(buff);

                if (write(pfd[1], buff, size_write) != size_write) {
                        close(pfd[1]);
                        close(pfd[0]);
                        free(buff);
                        return -1;
                }
        }

        free(buff);

        close(pfd[1]);
        dup2(pfd[0], STDIN_FILENO);
        close(pfd[0]);

        return 0;
}

static int apply_redirs(cmd_t *cmd)
{
        if (!cmd->redirs || !cmd->redirs_count) return 0;

        for (size_t i = 0; i < cmd->redirs_count; i++) {
                redir_t redir = cmd->redirs[i];

                switch (redir.type) {
                        case CMD_REDIR_IN: {
                                int fd = open(redir.fname, O_RDONLY);
                                if (fd < 0) return -1;

                                dup2(fd, STDIN_FILENO);

                                close(fd);

                                break;
                        }
                        case CMD_REDIR_OUT:
                        case CMD_REDIR_APPEND: {
                                int fd;

                                if (redir.type == CMD_REDIR_OUT) {
                                        fd = open(redir.fname, O_WRONLY | O_TRUNC | O_CREAT, 0644);
                                }
                                else {
                                        fd = open(redir.fname, O_WRONLY | O_APPEND | O_CREAT, 0644);
                                }

                                if (fd < 0) return -1;

                                dup2(fd, STDOUT_FILENO);
                                close(fd);

                                break;
                        }
                        case CMD_REDIR_HEREDOC: {
                                char *eof = redir.fname;

                                if (heredoc_process(eof) != 0) return -1;

                                break;
                        }
                }
        }

        return 0;
}

static int pipeline_execute(struct pipelist *head)
{
        if (!head) return -1;

        if (!head->next) {
                mshell_cmd_t *builtin = mshell_findcmd(head->cmd->argv[0]);
                
                if (builtin) {
                        expand_cmd(head->cmd);
                        return builtin->func(head->cmd);
                }
        }

        size_t pipelist_size = 0;

        for (struct pipelist *ptr = head; ptr != NULL; ptr = ptr->next) {
                pipelist_size++;
        }

        int pipes[pipelist_size - 1][2];
        int pids[pipelist_size];

        if (pipes_init(pipes, pipelist_size - 1) != 0) {
                return -1;
        }

        size_t idx = 0;

        for (struct pipelist *ptr = head; ptr != NULL; ptr = ptr->next) {
                pid_t pid = fork();
                g_fg_pid = pid;

                if (pid < 0) {
                        pipes_close(pipes, pipelist_size - 1);
                        
                        for (size_t i = 0; i < idx; i++) {
                                waitpid(pids[i], NULL, 0);
                        }

                        return 127;
                }
                
                if (pid == 0) {
                        if (idx > 0) {
                                dup2(pipes[idx - 1][0], STDIN_FILENO);
                        }
                        if (idx < pipelist_size - 1) {
                                dup2(pipes[idx][1], STDOUT_FILENO);
                        }

                        pipes_close(pipes, pipelist_size - 1);
                        expand_cmd(ptr->cmd);

                        if (apply_redirs(ptr->cmd) != 0) {
                                perror("mshell");
                                _exit(127);
                        }

                        mshell_cmd_t *builtin = mshell_findcmd(ptr->cmd->argv[0]);

                        if (builtin) {
                                _exit(builtin->func(ptr->cmd));
                        }
                        
                        execvp(*ptr->cmd->argv, ptr->cmd->argv);

                        perror("mshell");
                        _exit(127);
                }

                pids[idx] = pid;
                idx++;
        }

        pipes_close(pipes, pipelist_size - 1);

        int status = 0;

        for (size_t i = 0; i < pipelist_size; i++) {
                waitpid(pids[i], &status, 0);
        }

        return (WIFEXITED(status)) ? WEXITSTATUS(status) : (WIFSIGNALED(status)) ? 128 + WTERMSIG(status) : 1;
}

static int background_execute(struct joblist *ptr)
{
        pid_t pid = fork();

        if (pid < 0) return -1;
        if (pid > 0) return 0;

        if (setsid() < 0) return -1;
        chdir("/");

        _exit(pipeline_execute(ptr->pipe));
}

int execute(struct joblist *head)
{
        if (!head) return -1;

        int last_status = 0;

        for (struct joblist *ptr = head; ptr != NULL; ptr = ptr->next) {
                if (ptr->flags & JB_AND &&   last_status) break;
                if (ptr->flags & JB_OR  &&  !last_status) break;

                if (ptr->flags & JB_BACKGROUND) {
                        return background_execute(ptr);
                }
                else {
                        last_status = pipeline_execute(ptr->pipe);
                }
        }

        return last_status;
}