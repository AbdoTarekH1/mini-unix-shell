# Mini UNIX Shell with Job Control

A custom command-line interpreter built in C++ for the Ubuntu Linux environment. This project demonstrates core operating system mechanisms including process management, inter-process communication (IPC), file descriptor manipulation, and asynchronous signal handling.

Developed as a systems-level programming project for the Computer Systems Engineering (CSE) Operating Systems module at MSA University.

## 🚀 Features

* **Process Management:** Executes standard Linux commands using the `fork()` and `execvp()` system calls.
* **Job Control & Background Execution:** Supports running jobs in the background using the `&` operator, with asynchronous zombie process prevention via `SIGCHLD` signal handling.
* **I/O Redirection:** Directs standard input and output to files using `<`, `>`, and `>>` via `dup2()` and `open()`.
* **Pipelines:** Connects the output of one command to the input of another using `|` and the `pipe()` system call.
* **Dynamic Prompt:** Displays the current working directory utilizing `getcwd()`, styled with ANSI escape codes.
* **Advanced Command History:** Integrates the GNU Readline library for persistent command history and intuitive Up/Down arrow key navigation.
* **Built-in Commands:** * `cd`: Changes the current working directory.
  * `history`: Displays a numbered list of all commands entered during the session.
  * `exit`: Safely terminates the shell.
* **Quote-Aware Parsing:** Accurately parses arguments containing spaces when enclosed in quotation marks.

## 🛠️ System Requirements

* **Operating System:** Ubuntu Linux (Developed on 24.04.4 LTS)
* **Compiler:** GNU C++ Compiler (`g++`)
* **Dependencies:** GNU Readline Library (`libreadline-dev`)

## ⚙️ Installation & Compilation

1. **Clone the repository:**
   ```bash
   git clone [https://github.com/YourUsername/mini-unix-shell.git](https://github.com/YourUsername/mini-unix-shell.git)
   cd mini-unix-shell
