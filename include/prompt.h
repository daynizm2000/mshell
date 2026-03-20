#pragma once

typedef struct {
        char *res;
        char *distr;
        char *path;
        char *uname;
} prompt_t;

int prompt_init(prompt_t *prompt);
void prompt_print(prompt_t prompt);
void prompt_path_update(prompt_t *prompt);
void prompt_free(prompt_t *prompt);