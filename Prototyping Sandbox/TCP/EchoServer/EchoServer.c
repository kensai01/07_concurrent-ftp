/* An example of a concurrent Echo server
 * To quit the client, hit Control D as that is the
 * NULL character */

#include "NP.h" //Includes a variety of functions

/* Catches ctrl-C and Quits */
void sig_quit(int signo){
	printf("Ctrl-C was entered.\n");
	exit(0);
}


/* Catches the SIGCHILD signal upon termination of a child */
void sig_chld(int signo)
{
    pid_t   pid;
    int     stat;

    /* WNOHANG means the call is not blocking. This is useful in case
     * SIGCHILD is raised for a different reason other than child
     * terminating. */
    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("child %d terminated\n", (int) pid);
    }
    return;
}

/* str_echo deals with reading and communicating back
 * and forth from the server. Essentially, this is the
 * function that needs to be modified for HWK 2. */
void str_echo(int sockfd)
{
    ssize_t n;
    char buf[MAXLINE];

again:
	/* Writen as it is written is good enough to write your data
	 * "all at once." read, on the other hand, can return a single
	 * byte, which needs to be taken into account for. */
    while ( (n = read(sockfd, buf, MAXLINE)) > 0)
        Writen(sockfd, buf, n);
	/* Basically, the read was interrupted. We try to read again. */
    if (n < 0 && errno == EINTR)
        goto again;
    else if (n < 0)
        err_sys("str_echo: read error");
}

//argc- stands for the number of arguments
//argv- stands for the argument vector
int main(int argc, char **argv)
{
	/* Trap signal for Ctrl-C */
	Signal(SIGINT, sig_quit);

	/* Checks to see that two arguments were passed.
	 * The first argument would be the command name.
	 * The second argument would be the port number.*/
	if (argc == 1)
    	err_quit("usage: command <PortNumber>");
			
	/* pid_t is a type that can hold process IDS. In GNU C,
	 * this is an int. */
	pid_t pid;
	/* listenfd points to the file descriptor of the listening socket
	 * connfd points to the file descriptor of the socket returned
	 * by accept*/
	int listenfd, connfd;
	/* Stores the socket length */
	socklen_t clilen;
	//A C structure to hold the client and server address
	struct sockaddr_in cliaddr, servaddr;

	/* Basially, create a TCP socket
	 * If it fails, it will automatically quit the program due
	 * to the wrapper being used. */
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);

	/* 1) Initialize the memory. memset could have been instead.
	 * 2) Use IPv4
	 * 3) Listen on any interface that the server is running on
	 * 4) Bind to the port specified by the user. */
	bzero(&servaddr, sizeof(servaddr));
	servaddr.sin_family = AF_INET;
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	servaddr.sin_port = htons(atoi(argv[1]));

	//Bind the port. It will automatically exit(1) upon failure.
	Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	/* LISTENQ specifies the default size of the queue for 
	 * incoming connections*/
	Listen(listenfd, LISTENQ);

	/* Trap the signal for a child terminating. Call sig_child for it*/
	Signal (SIGCHLD, sig_chld); 
 
	for ( ; ; ) {
		/* 1) We first get the size of the client address data structure
		 * 2) We call a regular accept and not the wrapper. The reason we
		 *	  do so is to be able to check for EINTR. Basically, EINTR
		 *	  means that a system call interrupted. In many cases, we 
		 *	  simply restart whatever we were doing. In this case, 
		 *	  going back to the top of the for loop.
		 * 3) Otherwise, we got an error that is not EINTR and quit. */
		clilen = sizeof(cliaddr);
		/* If we do not care about the client's address, we can simply
		 * replace cliaddr and clilen with NULL */
		if ( (connfd = accept(listenfd, (struct sockaddr *) &cliaddr, 
				&clilen)) < 0) {
			if (errno == EINTR)
				continue;		/* back to for( ; ; ) */
			else
				err_sys("accept error");
		}

		/* Here we fork a child process to handle the connection
		 * All code inside the for loop is executed by the child
		 * only due to the if condition.*/ 
		if ( (pid = Fork()) == 0){
			Close(listenfd); /* Child closes listening socket */
			/* Write to the connected socket */
			str_echo(connfd);
			/* Close the connected socket */
			Close(connfd);
			/* The child terminates */
			exit(0);
		}
		/* The parent closes the connected socket */
		Close(connfd);
	}
}
