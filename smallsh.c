#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <stdbool.h>
#include <fcntl.h>
#include <sys/stat.h>

/*-------------------------------------------------------
Defined Constants
-------------------------------------------------------*/
#define MAX_CHAR		2048
#define MAX_COMMAND		512

/*-------------------------------------------------------
Gloval Variables
-------------------------------------------------------*/
bool quit = false;
bool background = false;
bool allow_back = true;
int status;
pid_t spawnpid = -5;
int num_arg = 0;
int num_proc = 0;
char input[MAX_CHAR];
char* arg_arr[MAX_COMMAND];
int processes[MAX_CHAR];	
struct sigaction SIGINT_action = {0}, SIGTSTP_action = {0};	

/*-------------------------------------------------------
Initialized Functions
-------------------------------------------------------*/
void getcommands();
void builtin();
void call_exit();
void call_status();
void call_cd();
void run_commands();
void child_fork();

/*-------------------------------------------------------
handle_SIGTSP function
- Changes the global allow_back variable and lets the user know that it has been toggled.
-------------------------------------------------------*/
void handle_SIGTSTP(int signo){
    char *message;

    // If we allow background processes, change it to false and display message to user.
    if (allow_back){
        allow_back = false;
        message = "\nEntering foreground-only mode (& is now ignored)\n";
        write(STDOUT_FILENO, message, 50);
        message = ": ";
        write(STDOUT_FILENO, message, 2);
        fflush(stdout);
    }

    // If we do not allow background processes, change it to true and display message to user.
    else {
        allow_back = true;
        message = "\nExiting foreground-only mode\n";
        write(STDOUT_FILENO, message, 30);
        message = ": ";
        write(STDOUT_FILENO, message, 2);
        fflush(stdout);
    }
}

/*-------------------------------------------------------
main Function
-------------------------------------------------------*/
int main(){
    // Handles ^c which initially ignores the call.
    SIGINT_action.sa_handler = SIG_IGN;
    sigfillset(&SIGINT_action.sa_mask);
    SIGINT_action.sa_flags = 0;
    sigaction(SIGINT, &SIGINT_action, NULL);

    // Handle ^z and make sure it doesn't interupt other processses.
    SIGTSTP_action.sa_handler = handle_SIGTSTP;
    sigfillset(&SIGTSTP_action.sa_mask);
    SIGTSTP_action.sa_flags = SA_RESTART;
    sigaction(SIGTSTP, &SIGTSTP_action, NULL);

    // As long as quit it true, continue to recieve user inputs and handle commands.
    while(!quit){
        getcommands();
        builtin();
    }

    return 0;
}

/*-------------------------------------------------------
getcommands Function
- Will prompt the user to input a command.
- Parse the input and place it into the arg_arr array.
- num_arg will have the number of commands input.
-------------------------------------------------------*/
void getcommands(){
    // Diplay the colon and retrieve the input.
    memset(arg_arr, 0, MAX_CHAR);
    printf(": ");
    fflush(stdout);
    fgets(input, MAX_CHAR, stdin);

    // check the error state of stdin, clear if needed
    if (ferror(stdin))
    {
        clearerr(stdin);
        memset(input, 0, MAX_CHAR);
    }
    
    // Remove the /n and place a /0 instead so that a blank space isn't processed as an argument.
    for (int i=0; i<MAX_CHAR; i++) {
		if (input[i] == '\n') {
			input[i] = '\0';
			break;
		}
	}
    
    // Parse the input and count the amount of inputs.
    const char space[2] = " ";
	char* token = strtok(input, space);
    num_arg = 0;

	// Fill in the array arg_arr with the parsed tokens
	while(token != NULL) {    
        arg_arr[num_arg] = token;
		token = strtok(NULL, " ");
        num_arg++;
	}

    // Expand $$.
    char look[] = "$$";
    char pid_s[100] = {0};
    bool abool = true;

    // Get the smallsh pid as a string.
    sprintf(pid_s, "%d", getpid());

    // iterate through all arguments and set temp_s as the argument.
    for (int i = 0; i < num_arg; i++){
        char temp_s[MAX_CHAR] = {0};
        strcpy(temp_s, arg_arr[i]);

        // use strstr to search for the first instance of $$.
        // If strstr() returns null, then move onto the next argument.
        // If strstr returns the pointer, replace it with pid and continue to search for $$.
        while(abool){
            char *p = strstr(temp_s, look);
            if (p == NULL) {
                abool = false;
            }
            else{
                size_t len1 = strlen(look);
                size_t len2 = strlen(pid_s);
                if (len1 != len2) {
                    memmove(p + len2, p + len1, strlen(p + len1) + 1);
                }
                memcpy(p, pid_s, len2);
                arg_arr[i] = malloc(strlen(temp_s) + 1);  // Make space for the larger string.
                strcpy(arg_arr[i], temp_s);
            }
        }
        abool = true;
        
    }
    
}

/*-------------------------------------------------------
builtin Function
- Will check if the user input exit, status, or cd
- Call the correct command.
-------------------------------------------------------*/
void builtin(){
    // If no arguments are given, then reset the input char.
    if (num_arg == 0){
        memset(input, 0, MAX_CHAR);
    }

    // If the first character of the argument list is #, reset the input char.
    else if (arg_arr[0][0] == '#'){
        memset(input, 0, MAX_CHAR);
    }

    // If first argument is exit, then call_exit()
    else if (strcmp(arg_arr[0], "exit") == 0){
        call_exit();
    }

    // If first argument is status, then call_status()
    else if (strcmp(arg_arr[0], "status") == 0){
        call_status();
    }

    // If first argument is cd, then call_cd()
    else if (strcmp(arg_arr[0], "cd") == 0){
        call_cd();
    }

    // If no other built-ins are found then continue to run the non-built in commands.
    else {
        run_commands();
    }
}

/*-------------------------------------------------------
call_exit Function.
- Terminate the program and end all processes.
-------------------------------------------------------*/
void call_exit(){
    // Set quit to true to prevent the while loop in main to continue.
    quit = true;

    // Kill all the processes in the background.
    for (int i = 0; i < num_proc; i++){
        kill(processes[i], SIGTERM);
    }
}

/*-------------------------------------------------------
call_status Function.
- Display the status of all background processes.
-------------------------------------------------------*/
void call_status(){
    int exit_status;
    int signal_status;
    // If the process exited normally return the correct exit value.
    if (WIFEXITED(status)){
        exit_status = WEXITSTATUS(status);
        printf("exit value %d\n", exit_status);
    }

    // If the process was exited by a signal, then return the correct signal termination.
    else{
        signal_status = WTERMSIG(status);
        printf("terminated by signal %d\n", signal_status);
    }
        
    fflush(stdout);
}
/*-------------------------------------------------------
call_cd Function.
- CD into another directory.
- will cd into the home directory if no path provided
-------------------------------------------------------*/
void call_cd(){
    int error_status = 0;

    // If there is only 1 argument, then cd into the home directory.
    if (num_arg == 1) {
        error_status = chdir(getenv("HOME"));
    }

    // If there are more than 1 arguments, then cd into only the first argument.
    else {
        error_status = chdir(arg_arr[1]);
    }

    // If successful, display the current directory.
    if (error_status == 0){
        // Prints the directory cd'ed to.
        char path[1000];
        getcwd(path, 1000);
        printf("%s\n", path);
    }
    else {
        printf("cd failed\n");
    }
    fflush(stdout);
}

/*-------------------------------------------------------
run_commands Function.
- If the commands are none of the build ins, fork the process.
-------------------------------------------------------*/
void run_commands(){
    // Modeled after the module's forking switch.
    background = false;
    spawnpid = fork();

    // Determine if the child needs to be run in the foreground or background.
    // If it is a background process, add it to a list of processes to terminate on exit.
    if (strcmp(arg_arr[num_arg - 1], "&") == 0) {
        if (allow_back){
            background = true;
            processes[num_proc] = spawnpid;
            num_proc += 1;
        }
        // Remove &
        arg_arr[num_arg - 1] = NULL;
    }

    switch (spawnpid){
        // Fork failed.
		case -1:
			perror("fork() failed!");
			exit(1);
            break;

        // This is the child fork.
		case 0:
            child_fork(background);
            break;

        // This is the parent fork
		default:
            // If the child process was a background process, then continue and don't wait for the child to terminate.
            if (background) {
                waitpid(spawnpid, &status, WNOHANG);
                printf("Background PID: %d\n", spawnpid);
                fflush(stdout);
            }

            // If the child process is a foreground process, wait for it to terminate and display a signal termination is ^C.
            else {
                waitpid(spawnpid, &status, 0);	
                if (WIFSIGNALED(status)){
                    call_status();
                }
                fflush(stdout);
            }
	}

    // Will print out the pid and exit status once a background child is finished.
    while ((spawnpid = waitpid(-1, &status, WNOHANG)) > 0) {
        printf("background pid %d is done: ", spawnpid);
        fflush(stdout);
        call_status();
	}
}

/*-------------------------------------------------------
child_fork Function.
- Determines if there is an input / output file necessary.
-------------------------------------------------------*/
void child_fork(){
    int in_fd = 0;
    int out_fd = 0;
    int copy = 0;
    char infile[MAX_CHAR + 1] = {0};
    char outfile[MAX_CHAR + 1] = {0};
    bool inbool = false;
    bool outbool = false;

    // Iterate through your arguments until you have reached a NULL as a background process could happen.
    for (int i = 0; arg_arr[i] != NULL; i++){
        // If there is a <. then set the infile name to the word to the right of < and replace < with NULL.
        if (strcmp(arg_arr[i], "<") == 0){
            inbool = true;
            strcpy(infile, arg_arr[i + 1]);
            arg_arr[i] = NULL;
        }

        // If there is a >. then set the outfile name to the word to the right of > and replace > with NULL.
        else if (strcmp(arg_arr[i], ">") == 0){
            outbool = true;
            strcpy(outfile, arg_arr[i + 1]);
            arg_arr[i] = NULL;
        }
    }

    // Taken from the modules.
    // If there was a <, then set dup2 to infile.
    if(inbool) {
        in_fd = open(infile, O_RDONLY);
        if (in_fd == -1){
            printf("open() failed on \"%s\"\n", infile);
            exit(1);
        }
        copy = dup2(in_fd, 0);
        if (copy == -1){
            printf("dup2() failed on \"%s\"\n", infile);
            exit(2);
        }
        fcntl(in_fd, F_SETFD, FD_CLOEXEC);
    }
    // If there was a >, then dup2 into the outfile.
    if(outbool) {
        out_fd = open(outfile, O_WRONLY | O_CREAT | O_TRUNC, 0666);
        if (out_fd == -1){
            printf("open() failed on \"%s\"\n", outfile);
            exit(1);
        }
        copy = dup2(out_fd, 1);
        if (copy == -1){
            printf("dup2() failed on \"%s\"\n", outfile);
            exit(2);
        }
        fcntl(out_fd, F_SETFD, FD_CLOEXEC);
    }

    // Re-assign ^c to its default signal to kill the foreground process only if the process is foreground.
    if (!background){
        SIGINT_action.sa_handler = SIG_DFL;
        sigaction(SIGINT, &SIGINT_action, NULL);
    }
    else{
        SIGINT_action.sa_handler = SIG_IGN;
        sigaction(SIGINT, &SIGINT_action, NULL);
    }

    // Run the non-built-in command
	if(execvp(arg_arr[0], arg_arr) == -1 ) {
        perror(arg_arr[0]);
        exit(1); 
    } 
}

