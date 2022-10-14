sshell : sshell.o
	gcc -Wall -Werror -Wextra -o sshell sshell.o

sshell.o : sshell.c
	gcc -Wall -Werror -Wextra -c sshell.c

clean:
	rm sshell sshell.o
