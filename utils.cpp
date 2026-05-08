// ==========================================
// UTILS.CPP
// ==========================================
#include "utils.hpp"
#include <sstream>
#include <sys/wait.h>
#include <iostream>

// I am allocating the actual memory space for my global vectors here. 
// They are declared as 'extern' in the header so my other files can access them.
std::vector<pid_t> background_jobs;
std::vector<std::string> cmd_history;

// My custom string parser. I built a simple state machine here to handle quotation marks.
// If the user types: echo "hello world", this ensures "hello world" stays as one 
// single argument instead of incorrectly splitting it into "hello" and "world" at the space.
std::vector<std::string> parse_input(const std::string& input) {
    std::vector<std::string> args;
    std::string current_arg;
    bool in_quotes = false;

    for (char c : input) {
        // Feature extension: If the TA asks me to add bash-style comments during 
        // the lab test, I just need to add: if (c == '#') break; right here.

        if (c == '\"') {
            in_quotes = !in_quotes; // I toggle the quote state to ignore spaces inside them
        } else if (c == ' ' && !in_quotes) {
            if (!current_arg.empty()) {
                args.push_back(current_arg);
                current_arg.clear();
            }
        } else {
            current_arg += c;
        }
    }
    if (!current_arg.empty()) {
        args.push_back(current_arg);
    }
    return args;
}

// My pipeline parser. I use a C++ stringstream here to easily chop up the raw input.
// I split it by the '|' character, and then run my parse_input() function on each chunk.
// This gives me a 2D vector (an array of command arrays) to feed into my execution engine.
std::vector<std::vector<std::string>> parse_pipeline(const std::string& input) {
    std::vector<std::vector<std::string>> commands;
    std::stringstream ss(input);
    std::string token;

    // Split the raw string by the '|' pipe character
    while (std::getline(ss, token, '|')) {
        commands.push_back(parse_input(token));
    }
    return commands;
}

// My custom signal handler for SIGCHLD. 
// This is my core defense against Zombie processes.
void handle_sigchld(int sig) {
    int status;
    pid_t pid;
    
    // I specifically passed the WNOHANG flag to waitpid(). 
    // This tells the OS: "If there are no dead children to reap, return immediately."
    // Without WNOHANG, my entire shell would freeze here waiting for a child to die.
    // The -1 argument tells waitpid to catch ANY background child process that finishes.
    while ((pid = waitpid(-1, &status, WNOHANG)) > 0) {
        // Once the OS tells me a child has died, I find it in my tracking vector and remove it.
        for (auto it = background_jobs.begin(); it != background_jobs.end(); ++it) {
            if (*it == pid) {
                background_jobs.erase(it);
                break;
            }
        }
    }
}
