#pragma once

#include <stddef.h>
#include <stdint.h>
#include "lexer.h"

#define CMD_REDIR_OUT           TOK_REDIR_OUT
#define CMD_REDIR_IN            TOK_REDIR_IN
#define CMD_REDIR_HEREDOC       TOK_REDIR_HEREDOC
#define CMD_REDIR_APPEND        TOK_REDIR_APPEND

#define JB_BACKGROUND (1 << 0)
#define JB_OR         (1 << 1)
#define JB_AND        (1 << 2)

typedef struct {
        int type;
        char *fname;
} redir_t;

typedef struct {
        char **argv;
        int argc;
        int capacity;

        redir_t *redirs; // array
        size_t redirs_cap;
        size_t redirs_count;
} cmd_t;

struct pipelist {
        cmd_t *cmd;
        struct pipelist *next;
};

struct joblist {
        struct pipelist *pipe;
        struct joblist *next;

        uint16_t flags;
};

void jobs_free(struct joblist *head);
struct joblist* jobs_init(struct tokenlist *tok_head);