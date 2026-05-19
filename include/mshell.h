#pragma once

#include <stddef.h>
#include <stdint.h>
#include <sys/types.h>






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






struct tokenlist {
        int type;
        char *data;
        int is_free;

        struct tokenlist *next;
};

enum {
        TOK_WORD,
        TOK_PIPE,               // |
        TOK_REDIR_OUT,          // >
        TOK_REDIR_IN,           // <
        TOK_REDIR_HEREDOC,      // <<
        TOK_REDIR_APPEND,       // >>
        TOK_BACKGROUND,         // &
        TOK_AND,                // &&
        TOK_OR,                 // ||
        TOK_SEP,                // ;
};






typedef struct {
        char *res;
        char *distr;
        char *path;
        char *uname;
} prompt_t;






typedef struct {
        char *name;
        int (*func)(cmd_t*);
} mshell_cmd_t;






extern volatile pid_t g_fg_pid;






// prompt.c

int prompt_init(prompt_t *prompt);
void prompt_print(prompt_t prompt);
void prompt_path_update(prompt_t *prompt);
void prompt_free(prompt_t *prompt);




// mshell_cmd.c

mshell_cmd_t* mshell_findcmd(const char *arg);
int mshell_cd(cmd_t *cmd);




// lexer.c

void toklist_free(struct tokenlist *head);
struct tokenlist* tokenize(const char *str);




// joblist.c

void jobs_free(struct joblist *head);
struct joblist* jobs_init(struct tokenlist *tok_head);




// executor.c

int execute(struct joblist *head);




// cmd_parser.c

int expand_cmd(cmd_t *cmd);