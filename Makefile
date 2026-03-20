NAME = mshell

CC = gcc
CFLAGS = -Wall -Wextra -Werror -g -lreadline

SRC = \
main.c \
src/prompt.c \
src/lexer.c \
src/joblist.c \
src/executor.c \
src/cmd_parser.c \
src/mshell_cmd.c

OBJDIR = build

OBJ = $(SRC:%.c=$(OBJDIR)/%.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CC) $(CFLAGS) $(OBJ) -o $(NAME)

$(OBJDIR)/%.o: %.c
	@mkdir -p $(dir $@)
	$(CC) $(CFLAGS) -c $< -o $@

clean:
	rm -rf $(OBJDIR)

fclean: clean
	rm -f $(NAME)

re: fclean all

.PHONY: all clean fclean re