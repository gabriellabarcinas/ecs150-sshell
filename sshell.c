#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/wait.h>

#define CMDLINE_MAX 512

struct cmd
{
        char type[50]; // pwd, cd, echo, date, etc.
        char* argv[CMDLINE_MAX];
        struct cmd *next;      
};

struct cmd* parsecmd(struct cmd* cmd, char cmdString[])
{
        const char delimiters[] = " \t\r\n\v\f";
        char *token;
        int i = 0;

        token = strtok(cmdString, delimiters); 
        strcpy(cmd->type, token);

        while (token != NULL) {
                if (!strcmp(token, "|")){
                        i++;
                        cmd->argv[i] = NULL;
                        token = strtok(NULL, "|");
                        printf("Next command: %s\n", token);
                        struct cmd *next = (struct cmd*) malloc(sizeof(struct cmd));
                        cmd->next = next;
                        parsecmd(next, token);
                }
                cmd->argv[i] = token;
                i++;
                token = strtok(NULL, delimiters);
        } 
        cmd->argv[i] = NULL;
        cmd->next = NULL;

        return cmd;
}

// fork - execute - wait cmd
int run(struct cmd *cmd) 
{
       pid_t pid;
       int status;

       pid = fork();
       if(pid == 0) { // Child Process
                execvp(cmd->type, cmd->argv);
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

// Builtin Command - Print Working Directory (pwd)
int runpwd()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error:");
    }
    else {
        fprintf(stderr, "%s\n", cwd);
    }
    return 0;
}

// Builtin Command - Change Directory (cd)
int runcd(struct cmd *cmd) 
{
    int retval = chdir(cmd->argv[1]);    
    if (retval != 0) {
        fprintf(stderr, "Error: cannot cd into directory\n");
        return EXIT_FAILURE;
    }

    return retval;
}

// Pipe
// void runpipe() {
//         pid_t pid;
//         int pipe[cmd1.numargs];

// }

// Free allocated memory
void freeMemory(struct cmd *ptr)
{
        while (ptr != NULL) {
                free(ptr);
                ptr = ptr->next; 
        }
}
int main(void)
{
        char cmd[CMDLINE_MAX];
        struct cmd *cmd1 = (struct cmd*) malloc(sizeof(struct cmd));

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
                parsecmd(cmd1, strcpy(cmdcopy,cmd));

                /* Builtin command */
                // exit
                if (!strcmp(cmd1->type, "exit")) {
                        fprintf(stderr, "Bye...\n");
                        break;
                } 
                // pwd
                else if (!strcmp(cmd1->type, "pwd")) {
                        retval = runpwd();
                } 
                // cd
                else if (!strcmp(cmd1->type, "cd")) {
                        retval = runcd(cmd1);
                }
                /* Regular command */
                else { 
                     retval = run(cmd1);   
                }

                fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, retval);
        }
        
        freeMemory(cmd1);
        return EXIT_SUCCESS;
}
