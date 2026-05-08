// ==========================================
// EXECUTOR.CPP
// ==========================================
#include "executor.hpp"
#include "utils.hpp"
#include <iostream>
#include <unistd.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <filesystem>
#include <cstdio>
#include <sys/stat.h>
#include <cstring>

using namespace std;

// I process built-in commands first because they must execute in the parent 
// shell process itself. If I forked them, things like 'cd' would only change 
// the directory for the child, not the main shell!
bool execute_builtin(const vector<string>& args) {
    if (args.empty()) return true;

    if (args[0] == "exit") {
        cout << "Terminating Mini UNIX Shell. Goodbye!" << endl;
        exit(0);
    }

    // I use the chdir() system call here to change the kernel's working directory 
    // pointer for this specific process.
    if (args[0] == "cd") {
        if (args.size() < 2) {
            cerr << "cd: missing argument" << endl;
        } else {
            if (chdir(args[1].c_str()) != 0) {
                perror("cd failed");
            }
        }
        return true;
    }

    if (args[0] == "history") {
        for (size_t i = 0; i < cmd_history.size(); ++i) {
            cout << "  " << i + 1 << "  " << cmd_history[i] << endl;
        }
        return true;
    }

    // My logic for bringing a background job back to the foreground
    if (args[0] == "fg") {
        if (background_jobs.empty()) {
            cout << "fg: no background jobs currently running." << endl;
        } else {
            pid_t last_job = background_jobs.back();
            cout << "Bringing job " << last_job << " to foreground..." << endl;
            int status;
            // I intentionally do NOT use WNOHANG here because I actively want my 
            // shell to freeze and synchronize with this specific job until it finishes.
            waitpid(last_job, &status, 0); // Wait for it to finish
            background_jobs.pop_back();
        }
        return true;
    }

    // ==========================================
    // CUSTOM FILESYSTEM OPERATIONS
    // ==========================================

    if (args[0] == "search") {
        if (args.size() < 2) {
            cerr << "search: missing file or folder name" << endl;
        } else {
            string target = args[1];
            cout << "Searching for '" << target << "'..." << endl;
            bool found = false;
            // I use the C++17 filesystem library here to recursively traverse directories
            auto options = std::filesystem::directory_options::skip_permission_denied;
            try {
                for (const auto& entry : std::filesystem::recursive_directory_iterator(std::filesystem::current_path(), options)) {
                    if (entry.path().filename() == target) {
                        cout << "\033[1;32mFound:\033[0m " << entry.path().string() << endl;
                        found = true;
                    }
                }
            } catch (const std::filesystem::filesystem_error& e) {
                cerr << "Search error: " << e.what() << endl;
            }
            if (!found) cout << "Could not find '" << target << "'." << endl;
        }
        return true; 
    }

    // Using the rename() system call
    if (args[0] == "move") { 
        if (args.size() < 3) {
            cerr << "move: usage: move <source> <destination>" << endl;
        } else {
            if (rename(args[1].c_str(), args[2].c_str()) != 0) {
                perror("move failed");
            } else {
                cout << "Successfully moved '" << args[1] << "' to '" << args[2] << "'" << endl;
            }
        }
        return true; 
    }

    // Using the unlink() system call to delete the file
    if (args[0] == "rmfile") {
        if (args.size() < 2) {
            cerr << "rmfile: missing filename" << endl;
        } else {
            if (unlink(args[1].c_str()) != 0) {
                perror("rmfile failed"); 
            } else {
                cout << "Deleted file: " << args[1] << endl;
            }
        }
        return true; 
    }

    // Using the chmod() system call to change octal permissions
    if (args[0] == "giveall") {
        if (args.size() < 2) {
            cerr << "giveall: missing target" << endl;
        } else {
            if (chmod(args[1].c_str(), 0777) != 0) {
                perror("giveall failed");
            } else {
                cout << "Granted 0777 permissions to: " << args[1] << endl;
            }
        }
        return true; 
    }

    // ==========================================
    // THE CORE MANUAL
    // ==========================================
    if (args[0] == "help") {
        cout << "\n========================================================" << endl;
        cout << "             MINI UNIX SHELL - COMPLETE MANUAL          " << endl;
        cout << "========================================================" << endl;
        cout << "\n\033[1;33m[1] BUILT-IN COMMANDS\033[0m" << endl;
        cout << "  cd <dir>       : Change working directory" << endl;
        cout << "  history        : Display command history" << endl;
        cout << "  fg             : Bring background job forward" << endl;
        cout << "  exit           : Terminate shell" << endl;
        cout << "\n\033[1;33m[2] PROCESS CONTROL & PIPELINES\033[0m" << endl;
        cout << "  <cmd> &        : Execute in background" << endl;
        cout << "  cmd1 | cmd2    : Chain processes" << endl;
        cout << "\n\033[1;33m[3] I/O REDIRECTION\033[0m" << endl;
        cout << "  < file         : Redirect input" << endl;
        cout << "  > file         : Redirect output (Overwrite)" << endl;
        cout << "  >> file        : Redirect output (Append)" << endl;
        cout << "\n\033[1;33m[4] STANDARD NATIVE COMMANDS\033[0m" << endl;
        cout << "  ls, grep, cat, echo, pwd, clear, etc." << endl;
        cout << "\n\033[1;33m[5] CUSTOM FILE OPERATIONS\033[0m" << endl;
        cout << "  search <name>  : Find file recursively" << endl;
        cout << "  move <old> <new>: Rename/move file" << endl;
        cout << "  rmfile <file>  : Delete file" << endl;
        cout << "  giveall <file> : Grant 0777 permissions" << endl;
        cout << "========================================================\n" << endl;
        return true;
    }

    return false; // Not a built-in command, tell main.cpp to use standard execution
}

void execute_standard(vector<string>& args, bool background) {
    int in_fd = -1, out_fd = -1;
    vector<string> clean_args;

    // I scan the arguments for <, >, >> and open files with the proper flags.
    // O_WRONLY = Write Only | O_CREAT = Create if missing | O_TRUNC = Overwrite | O_APPEND = Add to end
    for (size_t i = 0; i < args.size(); ++i) {
        if (args[i] == "<" && i + 1 < args.size()) {
            in_fd = open(args[i + 1].c_str(), O_RDONLY);
            if (in_fd < 0) { perror("Input redirection error"); return; }
            i++; 
        } else if (args[i] == ">" && i + 1 < args.size()) {
            out_fd = open(args[i + 1].c_str(), O_WRONLY | O_CREAT | O_TRUNC, 0644);
            if (out_fd < 0) { perror("Output redirection error"); return; }
            i++; 
        } else if (args[i] == ">>" && i + 1 < args.size()) {
            out_fd = open(args[i + 1].c_str(), O_WRONLY | O_CREAT | O_APPEND, 0644);
            if (out_fd < 0) { perror("Append redirection error"); return; }
            i++; 
        } else {
            clean_args.push_back(args[i]);
        }
    }

    if (clean_args.empty()) return;

    // I convert my C++ strings to a C-style array because the execvp() system call requires it
    vector<char*> c_args;
    for (auto& arg : clean_args) {
        c_args.push_back(&arg[0]);
    }
    c_args.push_back(nullptr);

    // Creating the child process. fork() clones the parent entirely.
    // It returns 0 to the child, and returns the child's PID to the parent.
    pid_t pid = fork();

    if (pid < 0) {
        perror("Fork failed");
    } else if (pid == 0) {
        // --- CHILD PROCESS ---
        
        // I use dup2() here to forcefully route STDIN or STDOUT to the files I opened above.
        // Once duplicated, I close the original file descriptors to prevent memory leaks.
        if (in_fd != -1) {
            dup2(in_fd, STDIN_FILENO);
            close(in_fd);
        }
        if (out_fd != -1) {
            dup2(out_fd, STDOUT_FILENO);
            close(out_fd);
        }

        // I use execvp() to completely replace the child's memory space with the new program.
        // The 'p' means it automatically searches the Ubuntu PATH variable for commands like 'ls'.
        if (execvp(c_args[0], c_args.data()) == -1) {
            perror("Execution failed");
            exit(EXIT_FAILURE);
        }
    } else {
        // --- PARENT PROCESS (The Shell) ---
        if (background) {
            cout << "[Background Job Started] PID: " << pid << endl;
            // I push it to my tracking vector so my handle_sigchld function can reap it later
            background_jobs.push_back(pid);
        } else {
            int status;
            // The shell halts here to synchronize with the child process before printing the next prompt
            waitpid(pid, &status, 0); // Wait for foreground process
        }
    }

    // Cleanup file descriptors in the parent so they don't stay open in the background
    if (in_fd != -1) close(in_fd);
    if (out_fd != -1) close(out_fd);
}

void execute_pipeline(const vector<vector<string>>& commands) {
    int num_cmds = commands.size();
    int pipefds[2];
    int in_fd = 0; // Tracks the read end of the previous pipe

    // I loop through every command chained in the pipeline
    for (int i = 0; i < num_cmds; ++i) {
        
        // I use pipe() to open a unidirectional data channel in memory.
        // pipefds[0] becomes the READ end. pipefds[1] becomes the WRITE end.
        if (i < num_cmds - 1) {
            if (pipe(pipefds) < 0) {
                perror("Pipe failed");
                return;
            }
        }

        pid_t pid = fork();
        if (pid < 0) {
            perror("Fork failed");
            return;
        } else if (pid == 0) {
            // --- CHILD PROCESS ---
            
            // If I am NOT the first command, I change my STDIN to read from the PREVIOUS pipe
            if (in_fd != 0) {
                dup2(in_fd, STDIN_FILENO);
                close(in_fd);
            }
            
            // If I am NOT the last command, I plumb my STDOUT to the CURRENT pipe's write end
            if (i < num_cmds - 1) {
                dup2(pipefds[1], STDOUT_FILENO);
                close(pipefds[1]);
                close(pipefds[0]); // A child writing to a pipe has no business holding the read end open
            }

            // Convert to C-array and execute
            vector<char*> c_args;
            for (const auto& arg : commands[i]) {
                c_args.push_back(const_cast<char*>(arg.c_str()));
            }
            c_args.push_back(nullptr);

            execvp(c_args[0], c_args.data());
            perror("Execution failed in pipeline");
            exit(EXIT_FAILURE);
        } else {
            // --- PARENT PROCESS ---
            
            // I close the old input pipe so the system knows data stopped flowing
            if (in_fd != 0) close(in_fd);
            
            // If I created a pipe for the next command, I close the write end in the parent.
            // I save the read end (pipefds[0]) into in_fd so the NEXT loop iteration can plug it into its child.
            if (i < num_cmds - 1) {
                close(pipefds[1]);
                in_fd = pipefds[0]; // Save read end for the next command
            }
        }
    }

    // My parent shell waits for ALL children in the pipeline to finish before showing the prompt again
    for (int i = 0; i < num_cmds; ++i) {
        wait(NULL);
    }
}
