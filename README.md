# 🐚 mshell — minimal Unix shell

mshell is a custom minimal Unix shell written in C for educational purposes. It implements basic Unix shell functionality including pipelines, redirections, logical operators, and simple job execution.

⚠️ This project is experimental and may contain bugs, undefined behavior, and incomplete edge-case handling.

---

## ⚙️ Features

* `|` pipelines
* `>` stdout redirection
* `>>` append stdout redirection
* `<` stdin redirection
* `&&` logical AND execution
* `||` logical OR execution
* `<<` here-document support
* background execution with `&`
* execution of external programs
* execution of `~/.mshellrc` on startup
* custom dynamic prompt (`user@host path>` style)
* basic signal handling (`SIGINT`, `SIGTERM`)
* command history (via GNU Readline)

---

## 📦 Dependencies

External libraries used:

* GNU Readline (line editing, history, interactive input)

### Install dependencies

**Arch Linux:**

```bash
sudo pacman -S readline
```

**Debian / Ubuntu:**

```bash
sudo apt install libreadline-dev
```

---

## 🧰 Build

```bash
git clone https://github.com/daynizm2000/mshell.git
cd mshell
make
```

---

## 🚀 Run

```bash
./mshell
```

---
