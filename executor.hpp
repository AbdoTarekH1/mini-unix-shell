// ==========================================
// EXECUTOR.HPP
// Developer: Abdelrahman Tarek Gamal Kilany Haggag
// ==========================================
// Standard include guards. I use these so the compiler doesn't throw 
// "multiple definition" errors if I include this header in multiple files.
#ifndef EXECUTOR_HPP
#define EXECUTOR_HPP

#include <vector>
#include <string>

// My built-in command router. I process these FIRST because commands like 'cd' 
// or 'exit' must execute in the parent shell process itself, not in a forked child.
// If it returns true, my main loop knows it was handled and skips standard execution.
bool execute_builtin(const std::vector<std::string>& args);

// My core OS execution engine for standard binaries (like ls, cat, grep).
// This is where I implemented the fork() and execvp() system calls. 
// It also sets up dup2() for I/O redirection and waitpid() for background jobs.
void execute_standard(std::vector<std::string>& args, bool background);

// My Inter-Process Communication (IPC) handler. 
// It takes a 2D vector of commands and uses the pipe() system call to chain 
// the STDOUT of one process directly into the STDIN of the next.
// I added a 'bool background' parameter so infinite pipelines can run asynchronously!
void execute_pipeline(const std::vector<std::vector<std::string>>& commands, bool background);

#endif
