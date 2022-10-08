#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define CMDLINE_MAX 512

void parsecmd(char cmd[], char* args[]) {
        char *arg;
        int i = 0;

        arg = strtok(cmd, " "); 

        while (arg != NULL){
                printf(" %s\n", arg);
                args[i] = arg;
                i++;
                arg = strtok(NULL, " ");
        } 
}

// fork - execute - wait
int run(char cmd[]) {

//        pid_t pid;
       char* args[CMDLINE_MAX];
       
       parsecmd(cmd, args); 
       
       if(fork() == 0) { // Child Process
                execvp(command, );
                perror("execv");
                exit(1);
       } else { // Parent Process
                wait
       }
        return 0;
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
                fprintf(stdout, "Return status value for '%s': %d\n",
                        cmd, retval);
        }
        return EXIT_SUCCESS;
}


