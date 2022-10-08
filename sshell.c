#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

void parsecmd(char cmd[], char* argv[]) {
        char *arg;
        int i = 0;

        arg = strtok(cmd, " "); 

        while (arg != NULL){
                argv[i] = arg;
                i++;
                arg = strtok(NULL, " ");
        } 
        argv[i] = NULL;
}

// fork - execute - wait
int run(char cmd[]) {
       pid_t pid;
       int status;
       char* argv[CMDLINE_MAX];
       
       parsecmd(cmd, argv); 
       
       pid = fork();
       if(pid == 0) { // Child Process
                execvp(argv[0], argv);
                perror("execv");
                exit(1);
       } else if (pid > 0) { // Parent Process
                waitpid(pid, &status, 0);
                // printf("Child returned %d\n", WEXITSTATUS(status));
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
                printf("sshell$ ");
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
                fprintf(stderr, "Return status value for '%s': %d\n",
                        cmd, retval);
        }
        return EXIT_SUCCESS;
}


