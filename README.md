# mshell — minimal Unix shell

mshell is a custom minimal Unix shell written in C for educational purposes. It implements core Unix shell functionality including pipelines, redirections, environment variables, logical operators, and job execution.

This project is experimental and may contain bugs, undefined behavior, parser limitations, and incomplete POSIX compatibility.

---

## Features

### Command execution

* execution of external programs
* execution of shell builtins
* PATH-based command lookup
* background execution with `&`

### Pipelines & operators

* `|` pipelines
* `&&` logical AND execution
* `||` logical OR execution
* command separator `;`

### Redirections

* `>` stdout redirection
* `>>` append stdout redirection
* `<` stdin redirection
* `<<` here-document support

### Environment variables

* shell variable assignments
* temporary environment variables for commands
* variable expansion with `$VAR`
* `export` builtin
* `unset` builtin

### Shell behavior

* execution of `~/.mshellrc` on startup
* custom dynamic prompt (`user@host path>` style)
* tilde expansion (`~`)
* basic signal handling (`SIGINT`, `SIGTERM`)
* command history via GNU Readline

---

## Dependencies

External libraries used:

* GNU Readline — line editing and command history

---

## Install dependencies

### Arch Linux

```bash
sudo pacman -S readline
````

### Debian / Ubuntu

```bash
sudo apt install libreadline-dev
```

---

## Build

```bash
git clone https://github.com/daynizm2000/mshell.git
cd mshell
make
```

---

## Run

```bash
./mshell
```

---

## Example usage

```sh
echo hello | cat

grep main < main.c

echo test > file.txt

echo append >> file.txt

cat << EOF
hello
EOF

USER_NAME=guest

echo $USER_NAME

EDITOR=nvim env

export PATH=/usr/bin

unset PATH

sleep 10 &
```

---

## Notes

mshell is an educational project and does not aim to fully replicate POSIX shell behavior.

Some parsing rules, expansion behavior, quoting semantics, and edge cases may differ from shells like:

* bash
* zsh

```
```
