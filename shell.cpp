/**
 * Mini UNIX Shell with Job Control
 * * This program implements a custom command-line interpreter for Ubuntu Linux.
 * It demonstrates core operating system concepts including:
 * - Process Creation and Replacement (fork, execvp)
 * - Process Synchronization (waitpid)
 * - Inter-Process Communication (pipe)
 * - File Descriptor Manipulation and Redirection (dup2, open, close)
 * - Asynchronous Signal Handling (signal, SIGCHLD)
 * - Advanced input handling using the GNU Readline library
 */

#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>      // Provides POSIX API (fork, execvp, chdir, getcwd, pipe, dup2)
#include <sys/wait.h>    // Provides wait() and waitpid() macros
#include <signal.h>      // Provides signal handling (SIGCHLD)
#include <fcntl.h>       // Provides file control options (O_RDONLY, O_WRONLY, O_CREAT, etc.)
#include <readline/readline.h> // Provides advanced line reading and arrow-key support
#include <readline/history.h>  // Provides internal history management for readline

using namespace std;

// Global history list to store previously entered commands for the custom 'history' built-in
vector<string> command_history;

/**
 * Signal Handler for SIGCHLD
 * * This function is executed asynchronously by the OS whenever a child process terminates.
 * It prevents background processes from becoming "zombie processes" (defunct processes
 * that still consume an entry in the system process table).
 * * waitpid(-1, ...) waits for ANY child process.
 * WNOHANG ensures the shell doesn't block (freeze) if no child has exited yet.
 */
void handle_sigchld(int sig) {
    while (waitpid(-1, nullptr, WNOHANG) > 0);
}

/**
 * Quote-Aware Input Parser
 * * Converts the raw input string into a vector of distinct arguments.
 * It uses a simple state machine (the `in_quotes` boolean) to ensure that spaces 
 * enclosed within quotation marks (e.g., "hello world") are treated as part of a 
 * single argument rather than a delimiter.
 * * @param input The raw string entered by the user.
 * @return A vector of parsed string arguments.
 */
vector<string> parse_input(const string& input) {
    vector<string> args;
    string current_arg = "";
    bool in_quotes = false;

    for (size_t i = 0; i < input.length(); ++i) {
        char c = input[i];

        if (c == '"') {
            in_quotes = !in_quotes; // Toggle quote state; do not include the quote character
        } 
        else if (c == ' ' && !in_quotes) {
            // Space found outside of quotes marks the end of an argument
            if (!current_arg.empty()) {
                args.push_back(current_arg);
                current_arg = ""; 
            }
        } 
        else {
            current_arg += c; // Append character to the current argument
        }
    }

    // Push the final argument if the string did not end with a space
    if (!current_arg.empty()) {
        args.push_back(current_arg);
    }

    return args;
}

int main() {
    // 1. Initialize Signal Handling
    // Instruct the OS to call handle_sigchld whenever a child process state changes
    signal(SIGCHLD, handle_sigchld);

    cout << "Welcome to the Mini Shell. Type 'exit' to quit." << endl;

    // 2. The REPL (Read-Evaluate-Print Loop)
    while (true) {
        
        // --- 2a. Dynamic Prompt Generation ---
        char cwd[1024];
        string prompt_str = "mini-shell> ";
        
        // Retrieve the Current Working Directory
        if (getcwd(cwd, sizeof(cwd)) != nullptr) {
            // Construct colored prompt using ANSI escape sequences.
            // \001 and \002 are Readline markers indicating invisible characters,
            // which prevents visual wrapping bugs when typing long commands.
            prompt_str = "\001\033[1;35m\002mini-shell\001\033[0m\002:\001\033[1;36m\002" + string(cwd) + "\001\033[0m\002> ";
        } else {
            perror("getcwd() error"); // Fallback if getting the directory fails
        }

        // --- 2b. Read Input ---
        // Readline handles Raw terminal mode, enabling arrow key history navigation
        char* buf = readline(prompt_str.c_str());
        
        // Handle EOF (e.g., when the user presses Ctrl+D)
        if (buf == nullptr) { 
            cout << "\nExiting shell..." << endl;
            break;
        }

        string input(buf);
        free(buf); // Prevent memory leaks; Readline dynamically allocates 'buf'

        if (input.empty()) continue; // Ignore empty presses of Enter

        // --- 2c. Update History ---
        add_history(input.c_str());        // Add to Readline's internal state for Up/Down arrows
        command_history.push_back(input);  // Add to our manual vector for the built-in 'history' command

        // --- 2d. Parse Input ---
        vector<string> parsed_args = parse_input(input);
        if (parsed_args.empty()) continue;

        // --- 3. Execute Built-in Commands ---
        // Built-ins must be executed by the parent shell process, not a forked child.
        
        // exit: Terminates the shell loop
        if (parsed_args[0] == "exit") {
            cout << "Exiting shell..." << endl;
            break;
        }

        // cd: Changes the shell's current working directory using chdir()
        if (parsed_args[0] == "cd") {
            if (parsed_args.size() < 2) {
                cerr << "cd: missing argument" << endl;
            } else {
                if (chdir(parsed_args[1].c_str()) != 0) {
                    perror("cd failed");
                }
            }
            continue; 
        }

        // history: Iterates through the global vector to print past commands
        if (parsed_args[0] == "history") {
            for (size_t i = 0; i < command_history.size(); ++i) {
                cout << i + 1 << ": " << command_history[i] << endl;
            }
            continue;
        }

        // --- 4. Background Execution Detection (&) ---
        bool run_in_background = false;
        if (parsed_args.back() == "&") {
            run_in_background = true;
            parsed_args.pop_back(); // Remove '&' so it is not passed to the target program
        }

        // --- 5. Pipeline Detection and Execution (|) ---
        int pipe_idx = -1;
        for (size_t i = 0; i < parsed_args.size(); ++i) {
            if (parsed_args[i] == "|") {
                pipe_idx = i;
                break;
            }
        }

        if (pipe_idx != -1) {
            // Split arguments into left (writer) and right (reader) commands
            vector<string> left_cmd(parsed_args.begin(), parsed_args.begin() + pipe_idx);
            vector<string> right_cmd(parsed_args.begin() + pipe_idx + 1, parsed_args.end());

            // Convert C++ std::string vectors to C-style char* arrays required by execvp()
            vector<char*> c_left, c_right;
            for (size_t i = 0; i < left_cmd.size(); ++i) c_left.push_back(const_cast<char*>(left_cmd[i].c_str()));
            c_left.push_back(nullptr); // MUST be null-terminated
            
            for (size_t i = 0; i < right_cmd.size(); ++i) c_right.push_back(const_cast<char*>(right_cmd[i].c_str()));
            c_right.push_back(nullptr); // MUST be null-terminated

            // Initialize the pipe. pipefd[0] is for reading, pipefd[1] is for writing.
            int pipefd[2];
            if (pipe(pipefd) < 0) {
                perror("Pipe creation failed");
                continue;
            }

            // Fork the first child (Left Command)
            pid_t pid1 = fork();
            if (pid1 == 0) {
                dup2(pipefd[1], STDOUT_FILENO); // Redirect stdout to the write-end of the pipe
                close(pipefd[0]); // Close unused read-end
                close(pipefd[1]); // Close original write-end fd (now duplicated)
                
                execvp(c_left[0], c_left.data()); // Replace process image
                perror("Command 1 execution error"); // Only reached if execvp fails
                exit(EXIT_FAILURE);
            }

            // Fork the second child (Right Command)
            pid_t pid2 = fork();
            if (pid2 == 0) {
                dup2(pipefd[0], STDIN_FILENO); // Redirect stdin to the read-end of the pipe
                close(pipefd[1]); // Close unused write-end
                close(pipefd[0]); // Close original read-end fd (now duplicated)
                
                execvp(c_right[0], c_right.data()); // Replace process image
                perror("Command 2 execution error"); // Only reached if execvp fails
                exit(EXIT_FAILURE);
            }

            // Parent Process: Close both ends of the pipe to send EOF to the reading child
            close(pipefd[0]);
            close(pipefd[1]);

            // Synchronize with children based on execution mode
            if (run_in_background) {
                cout << "[Pipeline running in background]" << endl;
            } else {
                waitpid(pid1, nullptr, 0);
                waitpid(pid2, nullptr, 0);
            }
            continue; // Skip the standard execution block
        }

        // --- 6. I/O Redirection Parsing (<, >, >>) ---
        // This block only executes if no pipe was found
        string input_file = "", output_file = "";
        bool append_mode = false;

        for (size_t i = 0; i < parsed_args.size(); ) {
            if (parsed_args[i] == "<" && i + 1 < parsed_args.size()) {
                input_file = parsed_args[i+1];
                parsed_args.erase(parsed_args.begin() + i, parsed_args.begin() + i + 2);
            } else if (parsed_args[i] == ">" && i + 1 < parsed_args.size()) {
                output_file = parsed_args[i+1];
                append_mode = false; // Truncate mode
                parsed_args.erase(parsed_args.begin() + i, parsed_args.begin() + i + 2);
            } else if (parsed_args[i] == ">>" && i + 1 < parsed_args.size()) {
                output_file = parsed_args[i+1];
                append_mode = true; // Append mode
                parsed_args.erase(parsed_args.begin() + i, parsed_args.begin() + i + 2);
            } else {
                i++; 
            }
        }

        // Convert arguments to C-style array for execvp
        vector<char*> c_args;
        for (size_t i = 0; i < parsed_args.size(); ++i) {
            c_args.push_back(const_cast<char*>(parsed_args[i].c_str()));
        }
        c_args.push_back(nullptr); 

        // --- 7. Standard Command Execution ---
        pid_t pid = fork(); // Duplicate the shell process

        if (pid < 0) {
            cerr << "Fork failed" << endl;
        } else if (pid == 0) {
            // --- CHILD PROCESS ---
            
            // Apply Input Redirection if a file was specified
            if (!input_file.empty()) {
                int fd_in = open(input_file.c_str(), O_RDONLY);
                if (fd_in < 0) { perror("Input redirection failed"); exit(EXIT_FAILURE); }
                dup2(fd_in, STDIN_FILENO); // Wire file descriptor 0 to the opened file
                close(fd_in);
            }

            // Apply Output Redirection if a file was specified
            if (!output_file.empty()) {
                int flags = O_WRONLY | O_CREAT; // Write-only, create if it doesn't exist
                flags |= append_mode ? O_APPEND : O_TRUNC; // Bitwise OR to set specific write mode
                
                // 0644 defines file permissions (rw-r--r--)
                int fd_out = open(output_file.c_str(), flags, 0644); 
                if (fd_out < 0) { perror("Output redirection failed"); exit(EXIT_FAILURE); }
                dup2(fd_out, STDOUT_FILENO); // Wire file descriptor 1 to the opened file
                close(fd_out);
            }

            // Replace the child process image with the target executable
            if (execvp(c_args[0], c_args.data()) == -1) {
                perror("Command execution error");
            }
            exit(EXIT_FAILURE); // Safety exit if execvp fails
            
        } else {
            // --- PARENT PROCESS ---
            // Synchronize with the child process
            if (run_in_background) {
                // Background: Do not wait, just print confirmation
                cout << "[Process running in background with PID " << pid << "]" << endl;
            } else {
                // Foreground: Pause the shell loop until this specific child finishes
                waitpid(pid, nullptr, 0); 
            }
        }
    }

    return 0;
}
