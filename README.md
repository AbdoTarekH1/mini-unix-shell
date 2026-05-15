# 🐚 ATH1 Mini UNIX Shell

> A fully functional, custom UNIX shell built from scratch in **C++17** for Ubuntu Linux — implementing core operating system concepts through direct POSIX system calls.

**Course:** CSE264/CSE364 — Operating Systems | MSA University, Spring 2026  
**Developer:** Abdelrahman Tarek Gamal Kilany Haggag (ID: 248761)

---

## 📋 Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Architecture](#architecture)
- [System Calls Used](#system-calls-used)
- [Installation & Build](#installation--build)
- [Usage](#usage)
- [Command Reference](#command-reference)
- [Project Structure](#project-structure)

---

## Overview

This project is a custom command-line interpreter that bridges the user and the Linux kernel — without relying on any high-level shell wrappers. Every feature is implemented through direct POSIX system calls, giving a hands-on demonstration of how real shells like `bash` work under the hood.

The shell features a dynamic color-coded prompt that updates in real time as you navigate directories, full GNU Readline integration for arrow-key history navigation, and a clean modular C++17 architecture.

---

## Features

| Feature | Description |
|---|---|
| **Standard Execution** | Run any Linux binary (`ls`, `cat`, `grep`, `whoami`, etc.) |
| **Background Jobs (`&`)** | Execute commands asynchronously; shell returns immediately |
| **Foreground (`fg`)** | Bring the most recent background job back to the foreground |
| **Zombie Prevention** | `SIGCHLD` handler with `WNOHANG` automatically reaps dead children |
| **I/O Redirection** | `<` input, `>` output (truncate), `>>` output (append) |
| **Infinite Pipelines** | Chain any number of commands with `\|` |
| **Command History** | Built-in `history` command + GNU Readline arrow-key navigation |
| **Quote-Aware Parsing** | `echo "hello world"` correctly passes one argument, not two |
| **Custom Filesystem Ops** | `search`, `move`, `rmfile`, `giveall` — using direct system calls |
| **Dynamic Prompt** | Color-coded prompt showing live current working directory |
| **Built-in Help** | `help` displays a full color-coded command manual |

---

## Architecture

The shell is organized into three distinct, modular layers:

```
┌─────────────────────────────────────────────┐
│              main.cpp  (REPL Core)          │
│  • Read-Eval-Print-Loop                     │
│  • GNU Readline integration                 │
│  • Signal setup (SIGCHLD, SIGINT)           │
│  • Routing: pipeline vs. standard vs. builtins │
└──────────────┬──────────────────────────────┘
               │
       ┌───────┴────────┐
       │                │
┌──────▼──────┐  ┌──────▼──────────────┐
│  utils.cpp  │  │    executor.cpp      │
│  • Parser   │  │  • Built-in router   │
│  • Pipeline │  │  • fork() + execvp() │
│    parser   │  │  • I/O redirection   │
│  • SIGCHLD  │  │  • Pipeline engine   │
│    handler  │  │  • Custom FS cmds    │
└─────────────┘  └─────────────────────┘
```

---

## System Calls Used

| System Call | Purpose |
|---|---|
| `fork()` | Clone the parent shell to create a child process |
| `execvp()` | Replace the child's memory image with the requested binary; auto-searches `PATH` |
| `waitpid()` | Synchronize with foreground children; reap background children with `WNOHANG` |
| `pipe()` | Open a unidirectional in-memory data channel for pipeline IPC |
| `dup2()` | Redirect `STDIN`/`STDOUT` to files or pipe ends |
| `open()` / `close()` | Open files with `O_RDONLY`, `O_WRONLY`, `O_CREAT`, `O_TRUNC`, `O_APPEND` |
| `signal()` | Register `SIGCHLD` handler for zombie prevention; ignore `SIGINT` in parent |
| `chdir()` | Change the kernel's working directory for the `cd` built-in |
| `getcwd()` | Fetch the current directory path for the dynamic prompt |
| `rename()` | Rename/move files (`move` command) |
| `unlink()` | Delete files from disk (`rmfile` command) |
| `chmod()` | Alter file permissions (`giveall` command) |

---

## Installation & Build

### Prerequisites

```bash
sudo apt update
sudo apt install g++ libreadline-dev
```

### Clone & Build

```bash
git clone https://github.com/AbdoTarekH1/mini-unix-shell.git
cd mini-unix-shell
make
```

### Run

```bash
./minishell
```

### Clean

```bash
make clean
```

---

## Usage

Once launched, you'll see the color-coded prompt showing your current directory:

```
ATH1-mini-shell:/home/user$
```

**Examples:**

```bash
# Standard commands
ls -la
whoami

# Navigate (prompt updates live)
cd Desktop
pwd

# Background execution
sleep 10 &
fg

# I/O Redirection
echo "Hello World" > output.txt
cat output.txt
echo "Second line" >> output.txt
wc -w < output.txt

# Pipelines (infinite chaining)
ls -l | wc -l
ls -la | grep "cpp" | wc -l
cat output.txt | grep "Hello" | wc -c

# Custom filesystem operations
search output.txt
giveall output.txt
move output.txt archive.txt
rmfile archive.txt

# Built-ins
history
help
exit
```

**Press Ctrl+C** — kills the running child, but the shell stays alive.  
**Press Ctrl+D** — gracefully logs out of the shell.

---

## Command Reference

### Built-in Commands

| Command | Description |
|---|---|
| `cd <dir>` | Change working directory |
| `history` | Display numbered command history for the session |
| `fg` | Bring the last background job to the foreground |
| `help` | Display the full color-coded command manual |
| `exit` | Terminate the shell |

### Custom Filesystem Commands

| Command | System Call | Description |
|---|---|---|
| `search <name>` | C++17 `filesystem` | Recursively find a file or folder from the current directory |
| `move <src> <dst>` | `rename()` | Rename or move a file |
| `rmfile <file>` | `unlink()` | Permanently delete a file from disk |
| `giveall <file>` | `chmod()` | Grant full `0777` permissions to a file |

### I/O Redirection

| Operator | Description |
|---|---|
| `< file` | Redirect file into command's STDIN |
| `> file` | Redirect command's STDOUT to file (overwrites) |
| `>> file` | Redirect command's STDOUT to file (appends) |

---

## Project Structure

```
mini-unix-shell/
├── main.cpp        # REPL core — signal setup, routing logic, readline loop
├── executor.cpp    # Built-in commands, fork/exec engine, pipelines, I/O redirection
├── executor.hpp    # Executor function declarations
├── utils.cpp       # Input parser (quote-aware), pipeline parser, SIGCHLD handler
├── utils.hpp       # Shared extern globals (background_jobs, cmd_history), utility declarations
└── Makefile        # Build system — compiles with C++17, links libreadline
```

---

*CSE264/CSE364 Operating Systems — MSA University / University of Greenwich, Spring 2026*
