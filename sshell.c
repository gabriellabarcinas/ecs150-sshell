#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

struct cmd {
    char type[50]; // pwd, cd, echo, date, etc.
    char* argv[CMDLINE_MAX];
} cmd1;

void parseCmd(char cmd[]) {
    const char delimiters[] = " \t\r\n\v\f";
    char *token;
    int i = 0;

    token = strtok(cmd, delimiters);
    strcpy(cmd1.type, token);

    while (token != NULL) {
        cmd1.argv[i] = token;
        i++;
        token = strtok(NULL, delimiters);
    }
    cmd1.argv[i] = NULL;
}

// Phase 1: Modify the program to use fork+exec+wait instead of using the function system()
/* The shell should fork and create a child process; the child process should run the specified command
with exec while the parent process waits until the child process has completed and the parent is able to
collect its exit status
 */
int execute() {
    
    pid_t pid;
    int status;

    pid = fork();
    if(pid == 0) { // Child Process
        execvp(cmd1.type, cmd1.argv);
        perror("execvp");
        exit(1);
    }
    else if (pid > 0) { // Parent Process
        waitpid(pid, &status, 0);
    }
    else { // If fork fails
        perror("fork");
        exit(1);
    }
    return WEXITSTATUS(status);
}

// Builtin Command - Print Working Directory (pwd)
int pwdCmd(){
    char cwd [PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL)
        perror("Error:");
    else
        printf("%s\n", cwd);
    return 0;
}

// Builtin Command - Change Directory (cd)
int cdCmd() {
    if (chdir(cmd1.argv[1]) != 0) {
        fprintf(stderr, "Error: cannot cd into directory\n");
        return EXIT_FAILURE;
    }
    else{
        chdir(cmd1.argv[1]);
    }
    return 0;
}

int main(void)
{
    char cmd[CMDLINE_MAX];

    while (1) {
        char *nl;
        int retval;
        char cmdCopy[CMDLINE_MAX];

        /* Print prompt */
        printf("sshell@ucd$ ");
        fflush(stdout);

        /* Get command line */
        fgets(cmd, CMDLINE_MAX, stdin);

        /* Print command line if stdin is not provided by terminal */
        if (!isatty(STDIN_FILENO)) {
            printf("%s", cmd);
            fflush(stdout);
        }

        /* Remove trailing newline from command line */
        nl = strchr(cmd, '\n');
        if (nl)
            *nl = '\0';

        /* Parse command line */
        parseCmd(strcpy(cmdCopy,cmd));

        /* Builtin command */
        // PWD
        if (!strcmp(cmd1.type, "pwd")){
            retval = pwdCmd(cmd1.type);
        }
        // CD
        else if (!strcmp(cmd1.type, "cd")){
            retval = cdCmd(cmd1.type);
        }
        // Exit
        else if (!strcmp(cmd1.type, "exit")) {
            fprintf(stderr, "Bye...\n");
            break;
        }
        /* Regular command */
        else{
            retval = execute(cmd1.type);
        }
        fprintf(stderr, "+ completed '%s' [%d]\n",
                cmd, retval);
    }
    return EXIT_SUCCESS;
}