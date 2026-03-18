#include "../include/lexer.h"
#include "../include/joblist.h"
#include <unistd.h>
#include <stdlib.h>
#include <string.h>

#define ARGV_DEFCAP 8
#define REDIRS_DEFCAP 4

static int job_sep[] = {
        TOK_SEP,
        TOK_BACKGROUND,
        TOK_AND,
        TOK_OR
};

static void cmd_free(cmd_t *cmd)
{
        if (!cmd) return;

        if (cmd->argv) {
                for (int i = 0; i < cmd->argc; i++) {
                        if (cmd->argv[i]) free(cmd->argv[i]);
                }

                free(cmd->argv);
        }

        if (cmd->redirs) {
                for (size_t i = 0; i < cmd->redirs_count; i++) {
                        if (cmd->redirs[i].fname) free(cmd->redirs[i].fname);
                }

                free(cmd->redirs);
        }

        free(cmd);
}

static void pipelist_free(struct pipelist *head)
{
        if (!head) return;

        for (struct pipelist *ptr = head; ptr != NULL; ) {
                struct pipelist *tmp = ptr;

                cmd_free(ptr->cmd);

                ptr = ptr->next;
                free(tmp);
        }
}

void jobs_free(struct joblist *head)
{
        if (!head) return;

        for (struct joblist *ptr = head; ptr != NULL; ) {
                struct joblist *tmp = ptr;

                pipelist_free(ptr->pipe);

                ptr = ptr->next;
                free(tmp);
        }
}

struct joblist* jobs_init(struct tokenlist *tok_head)
{
        if (!tok_head) return NULL;

        struct joblist *job_head = calloc(1, sizeof(struct joblist));
        if (!job_head) return NULL;

        struct joblist **jptr = &job_head;
        struct pipelist **pptr = &job_head->pipe;

        *pptr = calloc(1, sizeof(struct pipelist));
        if (!*pptr) goto err_exit;

        cmd_t **cmd = &(*pptr)->cmd;
        *cmd = calloc(1, sizeof(cmd_t));
        if (!*cmd) goto err_exit;

        (*cmd)->capacity = ARGV_DEFCAP;
        (*cmd)->argv = malloc((*cmd)->capacity * sizeof(char*));
        if (!(*cmd)->argv) goto err_exit;

        for (struct tokenlist *ptr = tok_head; ptr != NULL; ptr = ptr->next) {
                for (size_t i = 0; i < sizeof(job_sep) / sizeof(*job_sep); i++) {
                        if (ptr->type == job_sep[i]) {
                                if (ptr->type == TOK_BACKGROUND) (*jptr)->flags |= JB_BACKGROUND;
                                if (ptr->type == TOK_OR)         (*jptr)->flags |= JB_OR;
                                if (ptr->type == TOK_AND)        (*jptr)->flags |= JB_AND;

                                jptr = &(*jptr)->next;

                                *jptr = calloc(1, sizeof(struct joblist));
                                if (!*jptr) goto err_exit;

                                (*cmd)->argv[(*cmd)->argc] = NULL;

                                pptr = &(*jptr)->pipe;
                                *pptr = calloc(1, sizeof(struct pipelist));
                                if (!*pptr) goto err_exit;

                                cmd = &(*pptr)->cmd;
                                *cmd = calloc(1, sizeof(cmd_t));
                                if (!*cmd) goto err_exit;

                                (*cmd)->capacity = ARGV_DEFCAP;
                                (*cmd)->argv = malloc((*cmd)->capacity * sizeof(char*));
                                if (!(*cmd)->argv) goto err_exit;

                                break;
                        }
                }

                if (ptr->type == TOK_PIPE) {
                        (*cmd)->argv[(*cmd)->argc] = NULL;
                        pptr = &(*pptr)->next;

                        *pptr = calloc(1, sizeof(struct pipelist));
                        if (!*pptr) goto err_exit;

                        cmd = &(*pptr)->cmd;
                        *cmd = calloc(1, sizeof(cmd_t));
                        if (!*cmd) goto err_exit;

                        (*cmd)->capacity = ARGV_DEFCAP;
                        (*cmd)->argv = malloc((*cmd)->capacity * sizeof(char*));
                        if (!(*cmd)->argv) goto err_exit;
                }
                else {
                        if ((*cmd)->argc + 1 >= (*cmd)->capacity) {
                                char **tmp = realloc((*cmd)->argv, (*cmd)->capacity * 2 * sizeof(char*));
                                if (!tmp) goto err_exit;

                                (*cmd)->argv = tmp;
                                (*cmd)->capacity *= 2;
                        }

                        if (ptr->type == TOK_REDIR_APPEND || ptr->type == TOK_REDIR_HEREDOC
                                || ptr->type == TOK_REDIR_IN || ptr->type == TOK_REDIR_OUT
                        ) {
                                if (!(*cmd)->redirs_cap) {
                                        (*cmd)->redirs_cap = REDIRS_DEFCAP;
                                        (*cmd)->redirs = malloc(sizeof(redir_t) * (*cmd)->redirs_cap);

                                        if (!(*cmd)->redirs) goto err_exit;
                                }
                                else if ((*cmd)->redirs_count >= (*cmd)->redirs_cap) {
                                        redir_t *tmp = realloc((*cmd)->redirs, (*cmd)->redirs_cap * 2 * sizeof(redir_t));
                                        if (!tmp) goto err_exit;

                                        (*cmd)->redirs = tmp;
                                        (*cmd)->redirs_cap *= 2;
                                }

                                size_t redir_idx = (*cmd)->redirs_count;

                                (*cmd)->redirs[redir_idx].type = ptr->type;

                                if (ptr->next) (*cmd)->redirs[redir_idx].fname = strdup(ptr->next->data);
                                else            goto err_exit;

                                if (!(*cmd)->redirs[redir_idx].fname) goto err_exit;

                                ptr = ptr->next;
                                (*cmd)->redirs_count++;

                                continue;
                        }

                        if (ptr->type == TOK_WORD) {
                                (*cmd)->argv[(*cmd)->argc] = strdup(ptr->data);
                                if (!(*cmd)->argv[(*cmd)->argc]) goto err_exit;
                                (*cmd)->argc++;
                        }
                }
        }

        (*cmd)->argv[(*cmd)->argc] = NULL;

        return job_head;

err_exit:
        jobs_free(job_head);
        return NULL;
}