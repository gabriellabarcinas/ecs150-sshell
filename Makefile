# Generate executable
sshell: sshell.o
    gcc -Wall -Wextra -Werror -o sshell sshell.o

# Generate objects files from C files
sshell.o: sshell.c
    gcc -Wall -Wextra -Werror -c sshell.c

# Clean generated files
clean:
    rm -f sshell sshell.o