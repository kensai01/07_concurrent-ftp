//An example of an iterative daytime serve

#include "NP.h" //Includes functions for error handling 
#include <time.h>

#define LISTENQ 100

//argc- stands for the number of arguments
//argv- stands for the argument vector
int main(int argc, char **argv)
{
	int listenfd, connfd;
	char buff[MAXLINE];
	//A C structure to hold the server address
	struct sockaddr_in servaddr;
	time_t ticks; //Stores the time	

	//Fancy name for TCP socket
	//AF_INET is the address family that is used for the socket.
	//In this case, it is the Internet Protocol Address
	//SOCK_STREAM basically create a full-duplex connection
	//0 is the protocol number. 
	listenfd = Socket(AF_INET, SOCK_STREAM, 0);
	//Quit if it failed to create a socket for listening
	if (listenfd < 0)
		exit(1);

	//Initialize the memory of the structure to zero 
	bzero(&servaddr, sizeof(servaddr));
	//Use IPv4
	servaddr.sin_family = AF_INET;
	//Use any IP address
	servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
	//Listen on port 9999
	servaddr.sin_port	 = htons(10000);	/* daytime server */

	//Check if the bind failed
	if (! Bind(listenfd, (struct sockaddr *) &servaddr, sizeof(servaddr)) )
		exit(1);

	/* Check if listening failed
	 * LISTENQ specifies the default size of the queue for 
	 * incoming connections*/
	if (! Listen(listenfd, LISTENQ))
		exit(1);
 
	for ( ; ; ) {
		connfd = Accept(listenfd, (struct sockaddr *) NULL, NULL);
		if (connfd < 0)
			exit(1);
        ticks = time(NULL);
        snprintf(buff, sizeof(buff), "%.24s\r\n", ctime(&ticks));

        if (! Write(connfd, buff, strlen(buff)))
			exit(1);

		if (! Close(connfd))
			exit(1);
	}
	//In UNIX, this closes all open descriptors. Not necessarily
	//clean if ported to other systems.
	exit(0);
}
