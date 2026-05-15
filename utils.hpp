// ==========================================
// UTILS.HPP
// Developer: Abdelrahman Tarek Gamal Kilany Haggag
// ==========================================
// I am using standard C++ include guards here to ensure this header 
// is only compiled once, preventing "multiple definition" linker errors.
#ifndef UTILS_HPP
#define UTILS_HPP

#include <string>
#include <vector>
#include <sys/types.h>

// I declared this as 'extern' because I need both my main.cpp (to add jobs) 
// and my utils.cpp (to reap jobs) to access this exact same vector in memory.
extern std::vector<pid_t> background_jobs;

// Similar to above, I made my history vector 'extern' so main.cpp can append 
// user input to it, and my executor.cpp can print it out when the user types "history".
extern std::vector<std::string> cmd_history;

// My primary parser declaration. I designed this to handle quotes properly 
// so that commands like `echo "hello world"` are treated as two arguments, not three.
std::vector<std::string> parse_input(const std::string& input);

// My pipeline parser declaration. I wrote this to specifically scan for the '|' character 
// and split the string into a 2D vector (an array of command arrays) for my IPC routing.
std::vector<std::vector<std::string>> parse_pipeline(const std::string& input);

// The declaration for my Zombie process prevention logic. 
// I intercept the SIGCHLD signal sent by the OS here.
void handle_sigchld(int sig);

#endif
