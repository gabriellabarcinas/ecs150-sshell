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

void parsecmd(char cmdcopy[]) {
        const char delimiters[] = " \t\r\n\v\f";
        char *token;
        int i = 0;

        token = strtok(cmdcopy, delimiters); 
        strcpy(cmd1.type, token);

        while (token != NULL) {
                cmd1.argv[i] = token;
                i++;
                token = strtok(NULL, delimiters);
        } 
        cmd1.argv[i] = NULL;
}

// fork - execute - wait cmd
int run() {
       pid_t pid;
       int status;

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
                char cmdcopy[CMDLINE_MAX];

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
                parsecmd(strcpy(cmdcopy,cmd));

                /* Builtin command */
                if (!strcmp(cmd1.type, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                } else if (!strcmp(cmd1.type, "pwd")) {
                        char cwd[PATH_MAX];
                        getcwd(cwd, sizeof(cwd));
                        fprintf(stderr, "%s\n", cwd);
                        retval = 0;
                } else if (!strcmp(cmd1.type, "cd")) {
                        retval = chdir(cmd1.argv[1]);
                } else { /* Regular command */
                     retval = run();   
                }

                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, retval);
        }
        return EXIT_SUCCESS;
}
