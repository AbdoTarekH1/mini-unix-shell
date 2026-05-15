// ==========================================
// MAIN.CPP - MINI UNIX SHELL
// Developer: Abdelrahman Tarek Gamal Kilany Haggag
// ==========================================
#include <iostream>
#include <string>
#include <vector>
#include <unistd.h>
#include <signal.h>
#include <readline/readline.h>
#include <readline/history.h>
#include "utils.hpp"
#include "executor.hpp"

using namespace std;

int main() {
    // I set up my asynchronous signal handler here to catch SIGCHLD. 
    // This is my core OS defense against Zombie processes. When a background job ('&') 
    // finishes, the OS sends this signal, and I route it to my handler to reap the process.
    signal(SIGCHLD, handle_sigchld);
    
    // I am intentionally ignoring the SIGINT signal here. 
    // This ensures that if a user presses Ctrl+C, it only kills the running child process 
    // (like a stuck loop), but my main shell process stays alive and doesn't crash.
    signal(SIGINT, SIG_IGN); 

    char* raw_input;

    // This is the core Read-Eval-Print-Loop (REPL) of my operating system shell.
    while (true) {
        // I allocate a 1024-byte buffer here to safely hold the path of my current directory.
        char cwd[1024];
        
        // I use ANSI escape codes here to construct my dynamic, color-coded prompt.
        // The \x01 and \x02 markers protect Readline's character-width buffer calculations 
        // from the invisible ANSI escape sequences, preventing weird line-wrapping bugs.
        // I also split the string literal after \x02 to prevent a hex-parsing collision with 'A'.
        string prompt = "\x01\033[1;35m\x02" "ATH-mini-shell:\x01\033[1;36m\x02";
        
        // I use the getcwd() system call to ask the OS kernel for my exact location. 
        // This is how my prompt updates dynamically whenever I use the 'cd' command.
        if (getcwd(cwd, sizeof(cwd)) != NULL) {
            prompt += cwd;
        }
        
        prompt += "$ \x01\033[0m\x02";

        // I use the GNU Readline library here instead of a standard cin. 
        // This allows me to intercept the up/down arrow keys and scroll through my history natively.
        raw_input = readline(prompt.c_str());

        // I handle EOF (Ctrl+D) gracefully here so the shell breaks out of the loop cleanly.
        if (!raw_input) {
            cout << "\nLogging out..." << endl;
            break;
        }

        string input(raw_input);

        // I only process the input if the user didn't just hit Enter on an empty line.
        if (!input.empty()) {
            // I push the raw input into Readline's internal memory so the arrow keys work.
            add_history(raw_input);
            // I also save it to my custom tracking vector so my built-in 'history' command can access it.
            cmd_history.push_back(input);
            
            // My Routing Logic: 
            // First, I check if the user chained commands together. If I see a pipe '|', 
            // I route the string straight to my IPC pipeline execution engine.
            if (input.find('|') != string::npos) {
                // I use my parser to quickly check the end of the raw string for an ampersand
                auto args = parse_input(input); 
                bool background = false;
                if (!args.empty() && args.back() == "&") {
                    background = true;
                    // We must remove the '&' from the raw string before passing it to the pipeline parser
                    input = input.substr(0, input.find_last_of('&')); 
                }
                
                auto commands = parse_pipeline(input);
                // I pass the background flag so the pipeline knows whether to freeze or run asynchronously
                execute_pipeline(commands, background); 
            } else {
                // If there's no pipe, I use my custom parser to break the string into an array.
                auto args = parse_input(input);
                if (!args.empty()) {
                    
                    // I check the very last argument. If it's an ampersand, I flag this 
                    // to run asynchronously in the background so my REPL doesn't freeze.
                    bool background = false;
                    if (args.back() == "&") {
                        background = true;
                        args.pop_back(); // I pop the '&' off so it doesn't confuse the execvp() system call later
                    }

                    // I try to run it as a custom built-in command (like 'cd' or 'help') first.
                    // If my builtin function returns false, I pass it down to my standard fork/exec OS logic.
                    if (!execute_builtin(args)) {
                        execute_standard(args, background);
                    }
                }
            }
        }
        // I explicitly free the memory allocated by Readline here. 
        // If I didn't do this, I would create a memory leak on every single loop iteration.
        free(raw_input); 
    }

    return 0;
}
