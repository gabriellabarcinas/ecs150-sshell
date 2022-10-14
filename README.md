PHASE 0: Preliminary work
-------------------------

During this phase, we worked on merely understanding the skeleton code in the C file provided and formed a plan to tackle each of the phases one by one. Additionally, we set up our makefile during this phase to generate the executable sshell.

PHASE 1: Running simple commands
--------------------------------

For this phase, we implemented simple commands utilizing the fork + executre + wait method from lecture. In this process, we simply fork() the parent process to create a child process that executes the command with execvp and waits for the return status of the child in the parent process. We chose to use execvp amongst the other exec functions in the family in order to automatically search programs in the $PATH. Additionally, we chose

PHASE 2: Arguments
------------------

Foremost, we created a parsing function, void parsecmd(struct cmd* cmd, char cmdString[]), that parses the command line and stores the arguments into a struct. We utilized the strtok() library function to parse through each 'token' or argument of the command. We decided to use a this data structure because it allows for proper storage of command arguments which will be applicable and easy for keeping track of pipeline commands as well. 

PHASE 3: Builtin commands
-------------------------

To implement the builtin commands, we created seperate calling functions to excute cd and pwd in the shell. Namely:

    1. int runpwd()
    2. int runcd(struct cmd *cmd) 

For printing the working directory, we utilized the library function getcwd() as it retrieves the absolute pathname of the current working directory. Moreover, we utilizedd the chdir() library function to change the current working directory. These builtin commands, as well as all other commands, are closely paired with appropriate error management messeges. 

PHASE 4: Ouput Redirection
--------------------------

To implement output redirection, we first started with parsing the command line on the '>' delimeter. From here we were able to utilize fork(), close(), open(), and exevp(), to redirect the output of the given command to the specified file instead of to stdout. 

PHASE 5: Pipeline commands
--------------------------

For piping, we began off small and first implemented two commands and one pipe. Here, we reffered to the GNU Libc resources pertaining to pipes and the lecture pseudo-code that allowed us to better understand this process. First, we added a supplemental part to parsing this type of command by first using strtok() to parse the entire command line by the delimeter '|' to distinguish one command from the next in the pipeline. Then we parsed the individual command in our already implemented parsing function. 

To implement two commands and one pipe, we first created a single pipe using pipe() and forked twice for each command. For each child process we connect the appropriate reading and writing ends of the pipe to its stdin or stdout. 

Moving from a two command pipeline to a multi command pipeline, we generalized this same behavior. The procedure we created is as follows:
    1. For each pair of commands, create a pipe
    2. For each command, fork() to create a child process
    3. Connect stdin and/or stdout of child process depending on whether it is the first, last, or a middle command in the pipeline and close appropriate fd's
    4. execute command
    5. close shell fd's attached to shell
    6. wait for all child processes to return status via waitpid()

PHASE 6: Extra feature(s)
-------------------------

For Phase 6, we began by implementing a stack of struct by utilizing a linked list to help us with implementation of the builtin commands pushd, popd, and dirs that is used in the directory stack. We created two functions: push() and pop() to attain the results of pushing to a stack and popping from a stack. Along with creating the stack, we also utlized the library functions of getcwd(), which retrieves the current working directory, and chdir(), which changes the current working directory. For the three builtin commands, we created three seperate functions to implement each one; namely, pushCmd(), popdCmd(), and dirsCmd(). In each function we applied the appropriate library function of getcwd(), chdir(), or both to handle every test case as well as including relevant error management statements to handle errors.

To implement input redirection, we mirrored the process of output redirection that took place Phase 4. For input redirection, 

TESTING
-------

We continuously tested our sshell while and after finishing each a phase. Utilizing the gdb debugger allowed us to solve segmentation faults and print values to ensure their correctness. We followed the given testing script as well.

