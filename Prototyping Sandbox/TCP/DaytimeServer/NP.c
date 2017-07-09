/* Most of this code came from the Unix Network Programming book.
 * The code was tweaked as needed.*/
#include "NP.h"

/****************** Wrappers ********************************************/

/* A wrapper around the socket function
 * Takes as input the socket family, its type, and the protocol version
 * of the family (0 is for default). */
int Socket(int family, int type, int protocol)
{
	int n = socket(family, type, protocol);
	if (n < 0){
		err_sys("socket error");
		exit(1); //Quit as being unable to create a socket is fatal
	}
	return(n);
}

/* A wrapper around the connect function.
 * Returns false upon failing to connect.
 * Takes as input the socket ID, a socket address structure, and
 * the length of the socket address structure. 
 * Returns true if it can connect, false otherwise.*/
bool Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
	bool success = true; //Assume the connection succeeded
	int n = connect(fd, sa, salen);
	if (n < 0){
		err_sys("connect error");
		success = false;
	}
	return success;
}

/* A wrapper around the listen function
 * fd is an already created socket
 * backlog is the maximum # pending connections to queue.*/
bool Listen(int fd, int backlog)
{
	char *ptr;
	/*4can override 2nd argument with environment variable */
	if ( (ptr = getenv("LISTENQ")) != NULL)
		backlog = atoi(ptr);
	if (listen(fd, backlog) < 0){
		err_sys("listen error");
		return false;
	}
	else{
		return true; //Listening succeeded
	}
}

/* A wrapper around the bind function
 * fd is a file descriptor representing a socket
 * sa is a pointer to a sockaddr structure containing informaiton about
 * the address to bind
 * salen is the length of the sa structure passed in
 * */
bool Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (bind(fd, sa, salen) < 0){
		err_sys("bind error");
		return false; //Binding failed
	}
	return true; //Binding succeeded
}

/* A wrapper around write
 * */
bool Write(int fd, void *ptr, size_t nbytes)
{
	if (write(fd, ptr, nbytes) != nbytes){
		err_sys("write error");
		return false;
	}
	else{
		return true;
	}
}

/* A wrapper around close
 */
bool Close(int fd)
{
	if (close(fd) == -1){
		err_sys("close error");
		return false;
	}
	else{
		return true;
	}
}

/* A wrapper around accept */
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
	int	n;
again:
	n = accept(fd, sa, salenptr);
	if ( n < 0) {
#ifdef	EPROTO
		if (errno == EPROTO || errno == ECONNABORTED)
#else
		if (errno == ECONNABORTED)
#endif
			goto again;
		else
			err_sys("accept error");
	}
	return(n);
}


/* A wrapper aroud the inet_pton function
 * As opposed to inet_aton, it supports both IPv4 an IPv6 addresses
 * family here refers to the address family, i.e., AF_INET or AF_INET6
 * strptr is a pointer to a string containing the address to be converted
 * addrptr is a pointer to store the result in network byte order
 * Returns true if successful, false otherwise.*/
bool Inet_pton(int family, const char *strptr, void *addrptr)
{
	bool success = false; //Assume the conversion failed
	int n = inet_pton(family, strptr, addrptr);
	if (n < 0) 
		err_sys("inet_pton error for %s", strptr);	/* errno set */
	else if (n == 0)
		err_ret("inet_pton error for %s", strptr);	/* errno not set */
	else //Succeeded to convert
		success = true;
	return success;
}

/* A wrapper around the inet_ntop function. Upon success, it returns the
 * address as a string. Otherwise, it returns NULL.*/
bool Inet_ntop(int family, const void *addrptr, char *strptr, size_t len)
{
	if (strptr == NULL){ /* check for old code */
		err_ret("NULL 3rd argument to inet_ntop");
		return false;
	}
	if ( inet_ntop(family, addrptr, strptr, len) == NULL) {
		err_sys("inet_ntop error");		/* sets errno */
		return false;
	}
	return true;
}

/****************** Error Handling **************************************/

/* Print message and return to caller
 * Caller specifies "errnoflag" */
void err_doit(int errnoflag, const char *fmt, va_list ap)
{
	int errno_save, n;
	char buf[MAXLINE + 1];
	errno_save = errno; /* value caller might want printed */
	vsnprintf(buf, MAXLINE, fmt, ap); /* safe */
	n = strlen(buf);
	if (errnoflag)
		snprintf(buf + n, MAXLINE - n, ": %s", strerror(errno_save));
	strcat(buf, "\n");
	fflush(stdout);	 /* in case stdout and stderr are the same */
	fputs(buf, stderr);
	fflush(stderr);
}

/* Fatal error related to system call
 * Print message and terminate */
void err_sys(const char *fmt, ...)
{
	va_list ap; //Type for iterating arguments
	va_start(ap, fmt); //Start iterating arguments
	err_doit(1, fmt, ap);
	va_end(ap); //Frees a va_list
	//exit(1); //Keep the exit functionality in another function
}

/* Fatal error unrelated to system call
 * Print message and terminate */
void err_ret(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	err_doit(0, fmt, ap);
	va_end(ap);
	//exit(1); //Keep the exit functionality in another function
}
