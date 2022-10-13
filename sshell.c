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
        char* argv[CMDLINE_MAX];
        struct cmd *next;      
};

void mallocFail(struct cmd *cmd) 
{
        if (cmd == NULL) {
                perror("malloc failed");
                exit(1);
        }
}

void parsecmd(struct cmd* cmd, char cmdString[])
{       
        const char delimiters[] = " \t\r\n\v\f";
        char *token;
        int i = 0;

        token = strtok(cmdString, delimiters);           

        while (token != NULL) {
                cmd->argv[i] = token;
                token = strtok(NULL, delimiters);
                i++;
        }
        cmd->argv[i] = NULL;

        if (i+1 > 16) {
                fprintf(stderr, "Error: too many process arguments");
        }
}

void parsePipeline(struct cmd* cmd, char cmdString[], int numCmds)
{
        struct cmd *currCmd = cmd;
        char* cmds[numCmds];
        const char delimeters[] = "|"; 
        int i = 0;
        
        char *token = strtok(cmdString, delimeters);

        while (token != NULL) {
                cmds[i] = token;
                token = strtok(NULL, delimeters);
                i++;
        }

        for (i = 0; i < numCmds; i++) {
                parsecmd(currCmd, cmds[i]);
                struct cmd *next = (struct cmd*) malloc(sizeof(struct cmd));
                mallocFail(next);
                next->next = NULL;
                currCmd->next = next;
                currCmd = currCmd->next;
        }
}

// fork - execute - wait cmd
int run(struct cmd *cmd) 
{
       pid_t pid;
       int status;

       pid = fork();
       if (pid == 0) { // Child Process
                execvp(cmd->argv[0], cmd->argv);
                // perror("execv");
                perror("Error: command not found");
                exit(1);
       } else if (pid > 0) { // Parent Process
                waitpid(pid, &status, 0);
       } else { // If fork failed
                perror("fork failed");
                exit(1);
       }
       return WEXITSTATUS(status);
}

// Builtin Command - Print Working Directory (pwd)
int runpwd()
{
    char cwd[PATH_MAX];
    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error: could not retrieve current working directory");
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

void runpipeline(struct cmd *cmd, int numcmds, int retvals[])
{
        int numpipes = numcmds - 1;
        int fd[numpipes][2]; 
        pid_t childpid[numcmds];
        int status[numcmds];
        struct cmd *currCmd = cmd;
        int i;

 
        for (i = 0; i < numpipes; i++) {
                if (pipe(fd[i]) < 0) {
                        perror("pipe failed");
                        exit(1);
                }        
        }

        for (i = 0; i < numcmds; i++) {
                if ((childpid[i] = fork()) == 0) {
                        if (i == 0) {
                                close(fd[i][0]);
                                dup2(fd[i][1], STDOUT_FILENO);  
                                close(fd[i][1]);
                        } else if (i == numcmds - 1) {
                                close(fd[i-1][1]);
                                dup2(fd[i-1][0], STDIN_FILENO);
                                close(fd[i-1][0]);
                        } else {
                                close(fd[i][0]);
                                dup2(fd[i][1], STDOUT_FILENO);
                                close(fd[i][1]);
                                close(fd[i-1][1]);
                                dup2(fd[i-1][0], STDIN_FILENO);
                                close(fd[i-1][0]);
                        }
                        execvp(currCmd->argv[0], currCmd->argv);
                        perror("Error: command not found");
                        exit(1);
                } if (childpid[i] < 0) {
                        perror("fork failed");
                        exit(1);
                }
                if (i != 0) {  
                        close(fd[i-1][0]);
                        close(fd[i-1][1]);
                }
                if(currCmd->next != NULL) {
                        currCmd = currCmd->next;
                }
        }

        for (i = 0; i < numcmds; i++) {
                waitpid(childpid[i], &status[i], 0);
                retvals[i] = WEXITSTATUS(status[i]);
        }
}

// Free allocated memory
void freeMemory(struct cmd *ptr)
{
        while (ptr != NULL) {
                free(ptr);
                ptr = ptr->next; 
        }
}

// Count number of commands in pipeline
int numCmds(char argv[])
{
        unsigned i, count = 0;

        for (i = 0; i < strlen(argv); i++) {
                if (argv[i] == '|') {
                        count++;
                }
        }
        return count + 1;
}

void printCompletionStatus(char cmd[], int retval) 
{
        fprintf(stderr, "+ completed '%s' [%d]\n",
                        cmd, retval);
}

int main(void)
{
        char cmd[CMDLINE_MAX];
        struct cmd *cmd1 = (struct cmd*) malloc(sizeof(struct cmd));
        mallocFail(cmd1);

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
                if (nl) {
                        *nl = '\0';
                }

                /* Parse command line */
                strcpy(cmdcopy,cmd);
                int count = numCmds(cmdcopy);
                if (count > 1) {
                        parsePipeline(cmd1, cmdcopy, count);
                } else {
                        parsecmd(cmd1, cmdcopy);
                }
                
                if (cmd1->argv[0] == NULL) {
                        continue;
                }
                /* Builtin command */
                // exit
                else if (!strcmp(cmd1->argv[0], "exit")) {
                        fprintf(stderr, "Bye...\n");
                        fprintf(stderr, "+ completed '%s' [%d]\n", cmd, 0);
                        exit(0);
                } 
                // pwd
                else if (!strcmp(cmd1->argv[0], "pwd")) {
                        retval = runpwd();
                        printCompletionStatus(cmd, retval);
                } 
                // cd
                else if (!strcmp(cmd1->argv[0], "cd")) {
                        retval = runcd(cmd1);
                        printCompletionStatus(cmd, retval);
                }
                // piping
                else if (cmd1->next != NULL) {
                        int retvals[count];
                        runpipeline(cmd1, count, retvals);
                        // runpipeline(cmd1, retvals);
                        fprintf(stderr, "+ completed '%s' ", cmd);
                        for (int i = 0; i < count; i++) {
                                fprintf(stderr, "[%d]", retvals[i]);
                        }
                        fprintf(stderr, "\n");
                }
                /* Regular command */
                else {
                        retval = run(cmd1); 
                        printCompletionStatus(cmd, retval);  
                }
        }
        freeMemory(cmd1);
        return EXIT_SUCCESS;
}

