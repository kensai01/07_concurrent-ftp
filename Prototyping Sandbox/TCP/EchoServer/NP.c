/* Most of this code came from the Unix Network Programming book.
 * The code was tweaked as needed.*/
#include "NP.h"

/****************** Socket Wrappers ********************************/

/* A wrapper around the socket function
 * Takes as input the socket family, its type, and the protocol version
 * of the family (0 is for default). */
int Socket(int family, int type, int protocol)
{
	int n = socket(family, type, protocol);
	if (n < 0) 
		err_sys("socket error"); //This will also quit the program
	return(n);
}

/* A wrapper around the connect function.
 * Takes as input the socket ID, a socket address structure, and
 * the length of the socket address structure. 
 * Exits if it fails to connect. */ 
void Connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
	int n = connect(fd, sa, salen);
	if (n < 0)
		err_sys("connect error"); //This will also quit the program
}

/* A wrapper around the listen function
 * fd is an already created socket
 * backlog is the maximum # pending connections to queue.*/
void Listen(int fd, int backlog)
{
	char *ptr;
	/*4can override 2nd argument with environment variable */
	if ( (ptr = getenv("LISTENQ")) != NULL)
		backlog = atoi(ptr);
	if (listen(fd, backlog) < 0)
		err_sys("listen error");
}

/* A wrapper around the bind function
 * fd is a file descriptor representing a socket
 * sa is a pointer to a sockaddr structure containing informaiton about
 * the address to bind
 * salen is the length of the sa structure passed in
 * */
void Bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
	if (bind(fd, sa, salen) < 0)
		err_sys("bind error"); //This will also quit the program
}

/* A wrapper around close */
void Close(int fd)
{
	if (close(fd) == -1)
		err_sys("close error"); //This will also quit the program
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
			err_sys("accept error"); //This will also quit the program
	}
	return(n);
}

/****************** Address Conversion *********************************/

/* A wrapper aroud the inet_pton function
 * As opposed to inet_aton, it supports both IPv4 an IPv6 addresses
 * family here refers to the address family, i.e., AF_INET or AF_INET6
 * strptr is a pointer to a string containing the address to be converted
 * addrptr is a pointer to store the result in network byte order */
void Inet_pton(int family, const char *strptr, void *addrptr)
{
	int n = inet_pton(family, strptr, addrptr);
	if (n < 0) //Quitting the program in either case 
		err_sys("inet_pton error for %s", strptr);	/* errno set */
	else if (n == 0)
		err_quit("inet_pton error for %s", strptr);	/* errno not set */
}

/* A wrapper around the inet_ntop function. */
void Inet_ntop(int family, const void *addrptr, char *strptr, size_t len)
{
	if (strptr == NULL) /* check for old code */
		err_quit("NULL 3rd argument to inet_ntop");
	if ( inet_ntop(family, addrptr, strptr, len) == NULL)
		err_sys("inet_ntop error");		/* sets errno */
}

/****************** I/O Wrappers/Code **********************************/

//Some "global" variables
static int	read_cnt;
static char	*read_ptr;
static char	read_buf[MAXLINE];

/* A helper function to read from the file descriptor specified and
 * stores it in read_buf, which is "global" */
static ssize_t my_read(int fd, char *ptr)
{

	if (read_cnt <= 0) {
again:
		if ( (read_cnt = read(fd, read_buf, sizeof(read_buf))) < 0) {
			if (errno == EINTR)
				goto again;
			return(-1);
		} else if (read_cnt == 0)
			return(0);
		read_ptr = read_buf;
	}

	read_cnt--;
	*ptr = *read_ptr++;
	return(1);
}

/* Tries to read a line from a socket or file descriptor. It can also
 * handle the cases when EOF is reached. */
ssize_t readline(int fd, void *vptr, size_t maxlen)
{
	ssize_t	n, rc;
	char	c, *ptr;

	ptr = vptr;
	for (n = 1; n < maxlen; n++) {
		if ( (rc = my_read(fd, &c)) == 1) {
			*ptr++ = c;
			if (c == '\n')
				break;	/* newline is stored, like fgets() */
		} else if (rc == 0) {
			*ptr = 0;
			return(n - 1);	/* EOF, n - 1 bytes were read */
		} else
			return(-1);		/* error, errno set by read() */
	}

	*ptr = 0;	/* null terminate like fgets() */
	return(n);
}

/* A wrapper around Readline. Returns the number of bytes read. 
 * If the return value is less than 0, then the read failed.*/
ssize_t Readline(int fd, void *ptr, size_t maxlen){
	ssize_t	n;
	if ( (n = readline(fd, ptr, maxlen)) < 0)
		err_sys("readline error");
	return(n);
}

/* Write "n" bytes to a descriptor. 
 * This function will continue to write to the file descriptor until
 * all of the bytes specified are written.*/
static size_t writen(int fd, const void *vptr, size_t n)
{
	size_t		nleft;
	ssize_t		nwritten;
	const char	*ptr;

	ptr = vptr;
	nleft = n;
	while (nleft > 0) {
		if ( (nwritten = write(fd, ptr, nleft)) <= 0) {
			if (nwritten < 0 && errno == EINTR)
				nwritten = 0;		/* and call write() again */
			else
				return(-1);			/* error */
		}

		nleft -= nwritten;
		ptr   += nwritten;
	}
	return(n);
}

/* A wrapper around writen. Returns true if write is successful. */
void Writen(int fd, void *ptr, size_t nbytes)
{
	if (writen(fd, ptr, nbytes) != nbytes)
		err_sys("writen error");
}

/* A wrapper around write. Returns true if write is successful,
 * false otherwise. */
void Write(int fd, void *ptr, size_t nbytes)
{
	if (write(fd, ptr, nbytes) != nbytes)
		err_sys("write error");
}

//fclose
void Fclose(FILE *fp)
{
	if (fclose(fp) != 0)
		err_sys("fclose error");
}

//fdopen
FILE * Fdopen(int fd, const char *type)
{
	FILE	*fp;

	if ( (fp = fdopen(fd, type)) == NULL)
		err_sys("fdopen error");

	return(fp);
}

//fgets
char * Fgets(char *ptr, int n, FILE *stream)
{
	char	*rptr;

	if ( (rptr = fgets(ptr, n, stream)) == NULL && ferror(stream))
		err_sys("fgets error");

	return (rptr);
}

//fopen
FILE * Fopen(const char *filename, const char *mode)
{
	FILE	*fp;

	if ( (fp = fopen(filename, mode)) == NULL)
		err_sys("fopen error");

	return(fp);
}

//fputs
void Fputs(const char *ptr, FILE *stream)
{
	if (fputs(ptr, stream) == EOF)
		err_sys("fputs error");
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
	exit(1);
}

/* Fatal error unrelated to system call
 * Print message and terminate */
void err_quit(const char *fmt, ...)
{
	va_list ap;
	va_start(ap, fmt);
	err_doit(0, fmt, ap);
	va_end(ap);
	exit(1);
}

/****************** Signal Handling ************************************/

Sigfunc * signal(int signo, Sigfunc *func)
{
	struct sigaction act, oact;

	act.sa_handler = func;
	sigemptyset(&act.sa_mask);
	act.sa_flags = 0;
	if (signo == SIGALRM) {
#ifdef	SA_INTERRUPT
		act.sa_flags |= SA_INTERRUPT;	/* SunOS 4.x */
#endif
	} else {
#ifdef	SA_RESTART
		act.sa_flags |= SA_RESTART;		/* SVR4, 44BSD */
#endif
	}
	if (sigaction(signo, &act, &oact) < 0)
		return(SIG_ERR);
	return(oact.sa_handler);
}
/* end signal */

//A wrapper around signal
Sigfunc *Signal(int signo, Sigfunc *func)	/* for our signal() function */
{
	Sigfunc	*sigfunc;

	if ( (sigfunc = signal(signo, func)) == SIG_ERR)
		err_sys("signal error");
	return(sigfunc);
}

/*********************** Other ***************************************/

//Fork wrapper
pid_t Fork(void)
{
	pid_t	pid;

	if ( (pid = fork()) == -1)
		err_sys("fork error");
	return(pid);
}
