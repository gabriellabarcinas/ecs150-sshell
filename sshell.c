#include <fcntl.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>

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

void parseCmd(struct cmd* cmd, char cmdString[])
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
        parseCmd(currCmd, cmds[i]);
        struct cmd *next = (struct cmd*) malloc(sizeof(struct cmd));
        mallocFail(next);
        next->next = NULL;
        currCmd->next = next;
        currCmd = currCmd->next;
    }
}

// Stack created to store directories
// Used by pushd, popd, dirs
struct stack{
    char directories;
    struct stack* next;
};
struct stack* top = NULL;

// Push directory into stack
void push(char directory) {
    struct stack *curr;
    curr = malloc(sizeof(struct stack));
    curr->directories = directory;
    if (top == NULL) {
        curr->next = NULL;
    }
    else {
        curr->next = top;
    }
    top = curr;
}
// Pop directory out of stack
int pop() {
    struct stack *curr = top;
    char curr_directories = top->directories;
    top = top->next;
    free(curr);

    return curr_directories;
}

// fork - execute - wait cmd
int execute(struct cmd *cmd) {

    pid_t pid;
    int status;

    pid = fork();
    if(pid == 0) { // Child Process
        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, "Error: command not found\n");
        exit(1);
    }
    else if (pid > 0) { // Parent Process
        waitpid(pid, &status, 0);
    }
    else { // If fork fails
        perror("fork failed");
        exit(1);
    }
    return WEXITSTATUS(status);
}

/* Builtin Commands */

// Builtin Command - (pwd)
// Print Working Directory
int pwdCmd(){

    char cwd [PATH_MAX];

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error:");
    }
    else {
        printf("%s\n", cwd);
    }
    return 0;
}

// Builtin Command - (cd)
// Change Directory
int cdCmd(struct cmd *cmd) {

    int retval = chdir(cmd->argv[1]);
    if ( retval != 0) {
        fprintf(stderr, "Error: cannot cd into directory\n");
        return EXIT_FAILURE;
    }

    return retval;
}

/* Directory Stack */

// Builtin Command - (pushd)
// Push current directory to stack before changing directory
int pushdCmd(struct cmd *cmd) {

    char cwd[PATH_MAX];
    int retval;
    retval = chdir(cmd->argv[1]);

    if (getcwd(cwd, sizeof(cwd)) == NULL) {
        perror("Error:");
    }
    else {
        if ( retval != 0) {
            fprintf(stderr, "Error: cannot cd into directory\n");
            return EXIT_FAILURE;
        }
        else{
            push(*cwd);
        }
    }
    return retval;
}

// Builtin Command - (popd)
// Pops latest directory that was pushed and changes back to it
int popdCmd(){

    if (top == NULL){
        fprintf(stderr, "Error: directory stack empty\n");
    }
    else {
        pop();
        chdir("..");
    }
    return 0;
}

// Builtin Command - (dirs)
// Lists the stack of remembered directories
int dirsCmd(){

    char cwd [PATH_MAX];
    getcwd(cwd, sizeof(cwd));

    // Current directory starting first
    if (top == NULL) {
        push(*cwd);
        printf("%s\n", cwd);
    }
    else {
        struct stack *curr = top;
        while (curr->next != NULL) {
            printf("%d\n", curr->directories);
            curr = curr->next;
        }
        printf("%d\n", curr->directories);
    }
    return 0;
}

// Error Management
int iserror(char *cmd, char *filename) {

    if (cmd == NULL) {
        fprintf(stderr, "Error: missing command\n");
        return 1;
    } if (filename == NULL) {
        fprintf(stderr, "Error: no output file\n");
        return 1;
    }

    return 0;
}

/* I/O Redirection */

// Output Redirection
int outRedirection(char cmdString[], struct cmd *cmd)
{
    const char delimiter[] = ">";
    char *filename;
    char *token;
    int status;
    pid_t pid;

    token = strtok(cmdString, delimiter);
    filename = strtok(NULL, " \t\r\n\v\f");
    if(iserror(token, filename)) {
        return 1;
    }
    parseCmd(cmd, token);

    //Child process
    if ((pid = fork()) == 0) {
        int fdOut = open(filename, O_WRONLY | O_TRUNC | O_CREAT, 0644);
        dup2(fdOut, STDOUT_FILENO);
        if (fdOut == -1) {
            fprintf(stderr, "Error: cannot open output file\n");
            return 1;
        }
        close(fdOut);
        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, "Error: command not found");
        exit(1);
    } else if (pid > 0) { // Parent Process
        waitpid(pid, &status, 0);
    } else {
        perror("fork failed");
        exit(1);
    }

    return WEXITSTATUS(status);
}


// Input Redirection
int inRedirection(char cmdString[], struct cmd *cmd)
{
    const char delimiter[] = "<";
    char *filename;
    char *token;
    int status;
    pid_t pid;

    token = strtok(cmdString, delimiter);
    filename = strtok(NULL, " \t\r\n\v\f");
    if(iserror(token, filename)) {
        return 1;
    }
    parseCmd(cmd, token);

    //Child process
    if ((pid = fork()) == 0) {
        int fdIn = open(filename, O_RDONLY);
        dup2(fdIn, STDIN_FILENO);
        if (fdIn == -1) {
            fprintf(stderr, "Error: cannot open input file\n");
            return 1;
        }
        close(fdIn);
        execvp(cmd->argv[0], cmd->argv);
        fprintf(stderr, "Error: command not found");
        exit(1);
    } else if (pid > 0) { // Parent Process
        waitpid(pid, &status, 0);
    } else {
        perror("fork failed");
        exit(1);
    }

    return WEXITSTATUS(status);
}

// Piping
void pipeline(struct cmd *cmd, int numcmds, int retvals[]) {
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
            fprintf(stderr, "Error: command not found");
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

// Count number of commands
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

// Print completion status after execution
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
        if (strchr(cmdcopy, '|')) {
            parsePipeline(cmd1, cmdcopy, count);
        } else if (strchr(cmdcopy, '>')) {
            retval = outRedirection(cmdcopy, cmd1);
            if (retval != 1) {
                printCompletionStatus(cmd, retval);
            }
            continue;
        } else if (strchr(cmdcopy, '<')) {
            retval = inRedirection(cmdcopy, cmd1);
            if (retval != 1) {
                printCompletionStatus(cmd, retval);
            }
            continue;
        }
        else {
            parseCmd(cmd1, cmdcopy);
        }

        if (cmd1->argv[0] == NULL) {
            continue;
        }

        /* Builtin command */
        // pwd
        else if (!strcmp(cmd1->argv[0], "pwd")) {
            retval = pwdCmd();
            printCompletionStatus(cmd, retval);
            }
        // cd
        else if (!strcmp(cmd1->argv[0], "cd")) {
            retval = cdCmd(cmd1);
            printCompletionStatus(cmd, retval);
        }
        // pushd
        else if (!strcmp(cmd1->argv[0], "pushd")) {
            retval = pushdCmd(cmd1);
            printCompletionStatus(cmd, retval);
        }
        // popd
        else if (!strcmp(cmd1->argv[0], "popd")) {
            retval = popdCmd();
            printCompletionStatus(cmd, retval);
        }
        // dirs
        else if (!strcmp(cmd1->argv[0], "dirs")) {
            retval = dirsCmd();
            printCompletionStatus(cmd, retval);
        }
        // exit
        else if (!strcmp(cmd1->argv[0], "exit")) {
            fprintf(stderr, "Bye...\n");
            fprintf(stderr, "+ completed '%s' [%d]\n", cmd, 0);
            exit(0);
        }
        // Piping
        else if (cmd1->next != NULL) {
            int retvals[count];
            pipeline(cmd1, count, retvals);
            fprintf(stderr, "+ completed '%s' ", cmd);
            for (int i = 0; i < count; i++) {
                fprintf(stderr, "[%d]", retvals[i]);
            }
            fprintf(stderr, "\n");
        }
        /* Regular command */
        else {
            retval = execute(cmd1);
            printCompletionStatus(cmd, retval);
        }
    }
    freeMemory(cmd1);
    return EXIT_SUCCESS;
}
