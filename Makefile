# ==========================================
# MINISHELL MAKEFILE
# Developer: Abdelrahman Tarek Gamal Kilany Haggag
# ==========================================

# I am defining my compiler as g++ since I wrote this project in C++.
CXX = g++

# Compiler Flags:
# -Wall: I enabled all warnings to ensure my code is clean and to catch bugs early.
# -g: I included debugging symbols so I can trace errors if the shell crashes.
# -std=c++17: I explicitly required the C++17 standard because my custom 'search', 
#             'rmfolder', and 'move' filesystem commands rely on the modern <filesystem> library.
CXXFLAGS = -Wall -g -std=c++17

# Linker Flags:
# -lreadline: This is crucial. I am linking the GNU Readline library here 
#             so my shell can smoothly handle up/down arrow keys and history.
LDFLAGS = -lreadline

# I am tracking my compiled object files here. By keeping the project modular, 
# 'make' only recompiles the files I actually change, saving build time.
OBJS = main.o utils.o executor.o

# The final name of my compiled shell program
TARGET = minishell

# This is the default rule that runs when I just type 'make' in the terminal
all: $(TARGET)

# The Linking Step: 
# This rule takes all my compiled .o files and links them together with 
# the Readline library to create the final 'minishell' executable.
$(TARGET): $(OBJS)
	$(CXX) $(CXXFLAGS) -o $(TARGET) $(OBJS) $(LDFLAGS)

# The Compilation Steps:
# These rules compile my .cpp files into .o (object) files. 
# Notice that I included the .hpp header files as dependencies! 
# This means if I change a header file, 'make' is smart enough to recompile this file.
main.o: main.cpp utils.hpp executor.hpp
	$(CXX) $(CXXFLAGS) -c main.cpp

utils.o: utils.cpp utils.hpp
	$(CXX) $(CXXFLAGS) -c utils.cpp

executor.o: executor.cpp executor.hpp utils.hpp
	$(CXX) $(CXXFLAGS) -c executor.cpp

# A utility command I wrote so I can type 'make clean' to wipe out 
# the compiled files and start with a totally fresh build environment.
clean:
	rm -f *.o $(TARGET)
