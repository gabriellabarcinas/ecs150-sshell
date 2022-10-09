#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

struct cmd {
        char type[50]; // pwd, cd, echo, date, etc.
        char* argv[CMDLINE_MAX];
} cmd1;

void parsecmd(char cmd[]) {
        const char delimiters[] = " \t\r\n\v\f";
        char *token;
        int i = 0;

        token = strtok(cmd, delimiters); 
        strcpy(cmd1.type, token);
        // printf("cmd type: %s\n", cmd1.type);

        while (token != NULL) {
                cmd1.argv[i] = token;
                // printf("argv[%d] = %s\n", i, cmd1.argv[i]);
                i++;
                token = strtok(NULL, delimiters);
        } 
        cmd1.argv[i] = NULL;
}

// fork - execute - wait cmd
int run(char cmd[]) {
       pid_t pid;
       int status;
       char cmdcopy[CMDLINE_MAX];
       
       parsecmd(strcpy(cmdcopy,cmd));

       pid = fork();
       if(pid == 0) { // Child Process
                execvp(cmd1.type, cmd1.argv);
                perror("execv");
                exit(1);
       } else if (pid > 0) { // Parent Process
                waitpid(pid, &status, 0);
       } else { // If fork failed
                perror("fork");
                exit(1);
       }
       return WEXITSTATUS(status);
}

int main(void)
{
        char cmd[CMDLINE_MAX];

        while (1) {
                char *nl;
                int retval;

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

                /* Builtin command */
                if (!strcmp(cmd, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                }

                /* Regular command */
                retval = run(cmd);
                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, retval);
        }
        return EXIT_SUCCESS;
}


