#include "NP.h"

//This is the main function you want to modify for HWK 2
void str_cli(FILE *fp, int sockfd)
{
	char	sendline[MAXLINE], recvline[MAXLINE];

	while (fgets(sendline, MAXLINE, fp) != NULL) {

		Writen(sockfd, sendline, strlen(sendline));

		if (Readline(sockfd, recvline, MAXLINE) == 0)
			err_quit("str_cli: server terminated prematurely");

		fputs(recvline, stdout);
	}
}

/* argc- stands for the number of arguments
 * argv- stands for the argument vector storing the arguments */
 int main(int argc, char **argv)
 {
	int sockfd;
	//A C structure to hold the server address for IPv4
	struct sockaddr_in servaddr;
						
	/* Checks to see that two arguments were passed.
	 * The first argument would be the command name.
 	 * The second argument would be the IP address.*/
	if (argc != 3){
		err_quit("usage: command <IPaddress> PortNumber");
		exit(1); //We just want to quit in this instance
	}
	/* More or less, create a TCP IPv4 socket
	 * While PF_INET should be used in this case, for all practical purposes
	 * AF_INET can be used instead for the protocol family. AF_INET represents
	 * IPv4.
	 * SOCK_STREAM provides sequenced,  reliable,  two-way,	connection-based 
	 * byte streams.
	 * 0 is the default protocol version. */
	sockfd = Socket(AF_INET, SOCK_STREAM, 0); //A wrapper is being used.
	//Initialize the memory of the structure to zero
	bzero(&servaddr, sizeof(servaddr));
	//Use IPv4 for the address family
	servaddr.sin_family = AF_INET; 
	/* Try to connect to the specified port.
	 * Here, we used "host to network byte order short. */
 	servaddr.sin_port = htons(atoi(argv[2]));
	/* Convert to a binary IPv4 address the argument given and
	 * store it in the server's address. */
	Inet_pton(AF_INET, argv[1], &servaddr.sin_addr);
	//Use the socket we created earlier to connect to the server.
	Connect(sockfd, (struct sockaddr *) &servaddr, sizeof(servaddr));

	//This is the main function you want to modify for HWK 2
	str_cli(stdin, sockfd);

	/* In UNIX, this closes all open descriptors. Not necessarily
	 * clean if ported to other systems. */
	exit(0);
}
