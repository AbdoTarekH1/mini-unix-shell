# 🐚 Mini UNIX Shell

> A feature-complete Unix shell built from scratch in C++17 — implementing process control, IPC pipelines, I/O redirection, signal handling, and a custom filesystem layer using raw POSIX system calls.

![C++17](https://img.shields.io/badge/C%2B%2B-17-blue?style=flat-square&logo=c%2B%2B)
![Platform](https://img.shields.io/badge/Platform-Linux%20%2F%20Unix-orange?style=flat-square&logo=linux)
![Build](https://img.shields.io/badge/Build-Makefile-lightgrey?style=flat-square)
![License](https://img.shields.io/badge/License-Academic-green?style=flat-square)

---

## Overview

Mini UNIX Shell is a handcrafted, POSIX-compliant shell interpreter written entirely in C++17. Rather than wrapping an existing shell or using high-level abstractions, every core mechanic — from process forking and pipe creation to signal interception and zombie reaping — is implemented directly against the Linux kernel via system calls.

The project was built as a deep-dive into operating systems concepts: the `fork`/`exec` model, file descriptor manipulation, inter-process communication, and asynchronous signal handling. The result is a fully interactive shell that can run real system commands, chain processes through pipes, redirect I/O, manage background jobs, and navigate the filesystem — all from a custom REPL with GNU Readline integration.

---

## Features

### Core Shell Mechanics
- **Interactive REPL** — A Read-Eval-Print Loop with a dynamic, color-coded prompt that reflects the current working directory in real time
- **GNU Readline Integration** — Full up/down arrow key history navigation and line-editing support via `libreadline`
- **Command History** — Both Readline's native history and a custom `history` built-in that persists throughout the session

### Process Management
- **Foreground Execution** — Standard `fork()` + `execvp()` model; the shell blocks with `waitpid()` until the child process exits
- **Background Execution (`&`)** — Commands suffixed with `&` are forked and tracked asynchronously; the shell immediately returns to the prompt
- **`fg` Built-in** — Brings the most recently launched background job back to the foreground using a blocking `waitpid()` call
- **Zombie Process Prevention** — `SIGCHLD` is intercepted via a custom signal handler; `waitpid(-1, &status, WNOHANG)` reaps all terminated background children non-blockingly and auto-redraws the prompt via Readline's internal API

### Inter-Process Communication (IPC)
- **Pipelines (`|`)** — Chains an arbitrary number of commands using the `pipe()` system call; each stage's `STDOUT` is plumbed into the next stage's `STDIN` via `dup2()`
- **Background Pipelines** — Entire pipelines can be sent to the background with `&`; all stage PIDs are tracked
- **Correct Descriptor Lifecycle** — Parent process closes both ends of each pipe segment after forking to prevent read-end stalls and descriptor leaks

### I/O Redirection
| Operator | Behavior | Flags Used |
|----------|----------|------------|
| `< file` | Redirect STDIN from file | `O_RDONLY` |
| `> file` | Redirect STDOUT to file (overwrite) | `O_WRONLY \| O_CREAT \| O_TRUNC` |
| `>> file` | Redirect STDOUT to file (append) | `O_WRONLY \| O_CREAT \| O_APPEND` |

### Signal Handling
- **`SIGCHLD`** — Custom handler reaps background children, removes them from the tracking vector, and redraws the shell prompt without interrupting the foreground session
- **`SIGINT` (`Ctrl+C`)** — Set to `SIG_IGN` in the parent shell process; only propagates to and terminates the currently-running foreground child, leaving the shell itself alive

### Built-in Commands

| Command | Description |
|---------|-------------|
| `cd <dir>` | Change working directory via `chdir()` system call |
| `history` | Print the session's full command history |
| `fg` | Bring the last background job to the foreground |
| `exit` | Gracefully terminate the shell |
| `search <name>` | Recursively find a file or folder from the CWD using `std::filesystem` |
| `move <src> <dst>` | Move or rename a file/directory using the `rename()` system call |
| `rmfile <file>` | Delete a file using the `unlink()` system call |
| `rmfolder <dir>` | Force-recursively delete a directory using `std::filesystem::remove_all` |
| `giveall <target>` | Grant `0777` permissions to a file or directory via `chmod()` |
| `help` | Print the complete shell manual with all commands and usage |

### Parsing
- **Quote-aware tokenizer** — A custom state-machine parser correctly handles quoted strings (e.g., `echo "hello world"` produces two arguments, not three)
- **Pipeline parser** — Splits raw input on `|` and runs the tokenizer on each segment, producing a 2D argument vector
- **`&` detection** — Checks the final token of any command (including pipelines) for background execution intent before stripping it from the argument list

---

## Architecture

```
┌─────────────────────────────────────────────────────┐
│                     main.cpp                        │
│                                                     │
│  Signal Setup → REPL Loop → Parse → Route           │
│                                    │                │
│                    ┌───────────────┼────────────┐   │
│                    ▼               ▼            ▼   │
│              Pipeline?        Builtin?     Standard  │
└─────────────────────────────────────────────────────┘
         │                  │               │
         ▼                  ▼               ▼
  execute_pipeline()  execute_builtin() execute_standard()
  [executor.cpp]      [executor.cpp]    [executor.cpp]
         │                               │
  pipe() + fork()                   fork() + execvp()
  + dup2() per stage                + dup2() for I/O redir
  + waitpid() (all)                 + waitpid() / background
```

### Execution Flow

1. **Signal Setup** — `main()` registers `SIGCHLD → handle_sigchld` and ignores `SIGINT` before entering the REPL
2. **Prompt Construction** — `getcwd()` is called every iteration to build a dynamic, ANSI-colored prompt
3. **Input** — GNU Readline reads the line with full line-editing and adds it to both Readline history and the custom `cmd_history` vector
4. **Routing** — The raw string is inspected for `|`; if present it goes to `execute_pipeline()`, otherwise to `execute_builtin()` and then `execute_standard()` as a fallback
5. **Execution** — The appropriate executor forks child processes, wires up file descriptors with `dup2()`, and calls `execvp()` to replace child memory with the target binary
6. **Reaping** — The `SIGCHLD` handler asynchronously reaps finished background children with `WNOHANG` and refreshes the prompt

---

## Technologies & System Calls

| Category | Detail |
|----------|--------|
| Language | C++17 |
| Build System | GNU Make |
| Standard | POSIX / Linux |
| Process API | `fork()`, `execvp()`, `waitpid()`, `exit()` |
| IPC | `pipe()`, `dup2()` |
| Filesystem | `chdir()`, `getcwd()`, `rename()`, `unlink()`, `chmod()`, `std::filesystem` (C++17) |
| Signal API | `signal()`, `SIGCHLD`, `SIGINT`, `WNOHANG` |
| I/O | `open()`, `close()`, `O_RDONLY`, `O_WRONLY`, `O_CREAT`, `O_TRUNC`, `O_APPEND` |
| UI | GNU Readline (`libreadline`), ANSI escape codes |

---

## Project Structure

```
mini-unix-shell/
├── main.cpp          # REPL entry point: signal setup, prompt, routing logic
├── executor.cpp      # Core execution engine: builtins, fork/exec, pipelines, I/O redirection
├── executor.hpp      # Declarations for all executor functions
├── utils.cpp         # Parser, pipeline parser, SIGCHLD handler, global state definitions
├── utils.hpp         # extern declarations for shared globals (background_jobs, cmd_history)
└── Makefile          # Modular build system with dependency tracking and clean target
```

---

## Compilation & Usage

### Prerequisites

```bash
# Debian / Ubuntu
sudo apt-get install g++ libreadline-dev
```

### Build

```bash
git clone https://github.com/AbdoTarekH1/mini-unix-shell.git
cd mini-unix-shell
make
```

### Run

```bash
./minishell
```

### Clean Build Artifacts

```bash
make clean
```

---

## Example Commands

```bash
# Standard command execution
ATH-mini-shell:~/mini-unix-shell$ ls -la

# Change directory (built-in via chdir())
ATH-mini-shell:~$ cd /tmp

# I/O redirection
ATH-mini-shell:~$ ls > output.txt
ATH-mini-shell:~$ cat < output.txt
ATH-mini-shell:~$ echo "log entry" >> log.txt

# Pipeline: list processes, filter by name, count matches
ATH-mini-shell:~$ ps aux | grep firefox | wc -l

# Background execution
ATH-mini-shell:~$ sleep 10 &
[Background Job Started] PID: 4821

# Bring job to foreground
ATH-mini-shell:~$ fg

# Background pipeline
ATH-mini-shell:~$ cat large_file.txt | sort | uniq > sorted.txt &

# Custom filesystem commands
ATH-mini-shell:~$ search config.json
ATH-mini-shell:~$ move old_name.txt new_name.txt
ATH-mini-shell:~$ rmfile temp.log
ATH-mini-shell:~$ rmfolder ./build
ATH-mini-shell:~$ giveall script.sh

# View session history
ATH-mini-shell:~$ history

# Full command reference
ATH-mini-shell:~$ help
```

---

## Challenges & Learning Outcomes

**Zombie Process Elimination** — The most subtle challenge was ensuring background child processes were properly reaped without blocking the shell. The solution was registering a `SIGCHLD` handler that calls `waitpid(-1, &status, WNOHANG)` in a loop, guaranteeing every finished child — not just the first — is cleaned up regardless of how many terminate simultaneously.

**Readline Prompt Corruption** — ANSI escape sequences for colors caused Readline to miscalculate cursor position, leading to broken line-wrapping and overwritten prompts. The fix was wrapping every invisible escape sequence in `\x01...\x02` markers so Readline's internal width counter ignores them.

**Pipeline Descriptor Lifecycle** — Incorrectly leaving pipe file descriptors open in the parent caused downstream readers to block indefinitely (the read end never sees EOF if any writer is still open). This was solved by carefully closing the write end in the parent immediately after forking each stage, and always tracking the read end for the next iteration.

**`SIGCHLD` and Readline Interaction** — When a background job finishes mid-prompt, the completion message was printing inside the current input line. This was resolved by calling `rl_on_new_line()` and `rl_redisplay()` from within the signal handler to atomically reprint the prompt below the notification.

**Built-in vs. Forked Commands** — Commands like `cd` and `exit` must run in the parent process's own address space, not in a child. Running `cd` in a forked child would change only the child's directory, which is immediately discarded when the child exits. This distinction drove the two-phase routing design: `execute_builtin()` is always tried first.

**Key Systems Concepts Applied** — `fork`/`exec` process model, file descriptor duplication, pipe-based IPC, POSIX signal handling, `waitpid` semantics, UNIX permission bits, C++17 `<filesystem>` traversal, and GNU Readline integration.

---

## Future Improvements

- **Tab Completion** — Hook into Readline's `rl_attempted_completion_function` to complete commands and file paths
- **Environment Variable Expansion** — Parse and expand `$VAR` tokens before command execution using `getenv()`
- **Job Control (`jobs`, `bg`)** — Full job table with numbered job IDs and the ability to resume any suspended job, not just the last one
- **Here-Documents (`<<`)** — Support inline multi-line input redirection
- **Command Substitution (`$(...)`)** — Allow embedding command output directly in arguments
- **Logical Operators (`&&`, `||`)** — Sequential execution based on exit status
- **Config File (`.minishellrc`)** — Read startup aliases and environment configuration on launch

---

## Author

**Abdelrahman Tarek Gamal Kilany Haggag**
Computer Systems Engineering Student

[![GitHub](https://img.shields.io/badge/GitHub-AbdoTarekH1-181717?style=flat-square&logo=github)](https://github.com/AbdoTarekH1)
[![Repository](https://img.shields.io/badge/Repo-mini--unix--shell-blue?style=flat-square&logo=github)](https://github.com/AbdoTarekH1/mini-unix-shell)
