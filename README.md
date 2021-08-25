# CS344-Portfolio-Project

Description:
    Replicate the funcitonality shell in C with subset of features known as bash.

How to set up:
	1.) Download all p3testscript.sh, compile.sh and smallsh.c
	2.) Naviate through the terminal until you have reached the directory with all the files. 
	Use the ls command to see all directories and cd into the one you need to get to.

How to compile smallsh:
	3.) run chmod +x compile.sh
	4.) run ./compile.sh

How to compile p3testscript:
	3.) run chmod +x p3testscript
	4.) run ./p3testscript

What is compile.sh?
	compile.sh is a script that compiles myar.c into a usable program myar.

Content: 
	gcc -std=c11 -Wall -Werror -g3 -O0 -o smallsh smallsh.c -D_POSIX_C_SOURCE=200809L

Format Input:
    command [arg1 arg2 ...] [< input_file] [> output_file] [&]
    - command will be able to replicate most shell commands.
    - arguments may be placed into the front of the command
    - redirections of input/output are possible for single file only.
    - & toggles foreground use of programs only.
    
Program Command (exit):
    - exit will terminate all background processes and return to the main terminal of the directory.

Program Command (cd):
    - cd without an argument will return the user to the home directory
    - they may cd into a path through only the first argument after cd.

Program Command (status):
    - Will print the status of the last process if it was terminated normally or through a signal.

Program Command (<, >):
    - Files may be redirected following the format input above.

Program Command (^z or ctrl+c):
    - This will terminate a foreground process and display the signal it was terminated with.
    - This will not affect background processes.

Program Command (^z or ctrl+z):
    - This will toggle between background processes being allowed or foreground processes only.
