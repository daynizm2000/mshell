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
        int     type;
        char    *fname;
} redir_t;

typedef struct {
        char            **argv;
        int             argc;
        int             capacity;

        redir_t         *redirs; // array
        size_t          redirs_cap;
        size_t          redirs_count;
} cmd_t;

struct pipelist {
        cmd_t                   *cmd;
        struct pipelist         *next;
};

struct joblist {
        struct pipelist         *pipe;
        struct joblist          *next;

        uint16_t                flags;
};






struct tokenlist {
        int                     type;
        char                    *data;
        int                     is_free;

        struct tokenlist        *next;
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






typedef struct {
        char    *key;
        char    *value;

        int     is_exported;
} env_t;

typedef struct env_map_node {
        env_t                   env;
        struct env_map_node     *next;
} env_map_node_t;

struct env_map {
        env_map_node_t          **buckets;
        size_t                  capacity;
        size_t                  count;
};






extern volatile pid_t   g_fg_pid;
extern struct env_map   g_env_map;






// prompt.c

int prompt_init(prompt_t *prompt);
void prompt_print(prompt_t prompt);
void prompt_path_update(prompt_t *prompt);
void prompt_destroy(prompt_t *prompt);




// mshell_cmd.c

mshell_cmd_t* mshell_findcmd(const char *arg);
int mshell_cmd_export(cmd_t *cmd);
int mshell_cmd_unset(cmd_t *cmd);
int mshell_cmd_cd(cmd_t *cmd);




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




// environment.c

int env_map_init(struct env_map *map);
int env_map_copy_envp(struct env_map *map, const char** const envp);
char** env_map_generate_envp(struct env_map *map);

int env_map_add(struct env_map *map, const env_t *env);
env_t* env_map_find(struct env_map *map, const char *key);
int env_map_delete(struct env_map *map, const char *key);
void env_map_print(struct env_map *map);
void env_map_destroy(struct env_map *map);