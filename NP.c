/* Most of this code came from the Unix Network Programming book.
 * The code was tweaked as needed.*/

#include "NP.h"



/*Strip white space
/*Strip white space
 * Returns a pointer to a string.*/
char * StripWhite(char *str){
    char *end;
    // Trim leading space
    while(isspace((unsigned char)*str)) str++;
    if(*str == 0)  // All spaces?
        return str;
    // Trim trailing space
    end = str + strlen(str) - 1;
    while(end > str && isspace((unsigned char)*end)) end--;
    // Write new null terminator
    *(end+1) = 0;
    return str;
}

/****************** Socket Wrappers ********************************/
unsigned short	portbase = 0;	/* port base, for non-root servers	*/
int PassiveSocket(const char *service, const char *transport, int qlen)
/*
 * Arguments:
 *      service   - service associated with the desired port
 *      transport - transport protocol to use ("tcp" or "udp")
 *      qlen      - maximum server request queue length
 */
{
    struct servent	*pse;	/* pointer to service information entry	*/
    struct protoent *ppe;	/* pointer to protocol information entry*/
    struct sockaddr_in sin;	/* an Internet endpoint address		*/
    int	s, type;	/* socket descriptor and socket type	*/

    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;

    /* Map service name to port number */
    if ( pse = getservbyname(service, transport) )
        sin.sin_port = htons(ntohs((unsigned short)pse->s_port)
                             + portbase);
    else if ((sin.sin_port=htons((unsigned short)atoi(service))) == 0)
        printf("can't get \"%s\" service entry\n", service);

    /* Map protocol name to protocol number */
    if ( (ppe = getprotobyname(transport)) == 0)
        printf("can't get \"%s\" protocol entry\n", transport);

    /* Use protocol to choose a socket type */
    if (strcmp(transport, "udp") == 0)
        type = SOCK_DGRAM;
    else
        type = SOCK_STREAM;

    /* Allocate a socket */
    s = socket(PF_INET, type, ppe->p_proto);
    if (s < 0)
        printf("can't create socket: %s\n", strerror(errno));

    /* Bind the socket */
    if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        printf("can't bind to %s port: %s\n", service,
                strerror(errno));
    if (type == SOCK_STREAM && listen(s, qlen) < 0)
        printf("can't listen on %s port: %s\n", service,
                strerror(errno));
    return s;
}

int ConnectSocket(const char *host, const char *service, const char *transport )
/*
 * Arguments:
 *      host      - name of host to which connection is desired
 *      service   - service associated with the desired port
 *      transport - name of transport protocol to use ("tcp" or "udp")
 */
{
    struct hostent	*phe;	/* pointer to host information entry	*/
    struct servent	*pse;	/* pointer to service information entry	*/
    struct protoent *ppe;	/* pointer to protocol information entry*/
    struct sockaddr_in sin;	/* an Internet endpoint address		*/
    int	s, type;	/* socket descriptor and socket type	*/


    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;

    /* Map service name to port number */
    if ( pse = getservbyname(service, transport) )
        sin.sin_port = pse->s_port;
    else if ((sin.sin_port=htons((unsigned short)atoi(service))) == 0)
        printf("can't get \"%s\" service entry\n", service);

    /* Map host name to IP address, allowing for dotted decimal */
    if ( phe = gethostbyname(host) )
        memcpy(&sin.sin_addr, phe->h_addr, phe->h_length);
    else if ( (sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE )
        printf("can't get \"%s\" host entry\n", host);

    /* Map transport protocol name to protocol number */
    if ( (ppe = getprotobyname(transport)) == 0)
        printf("can't get \"%s\" protocol entry\n", transport);

    /* Use protocol to choose a socket type */
    if (strcmp(transport, "udp") == 0)
        type = SOCK_DGRAM;
    else
        type = SOCK_STREAM;

    /* Allocate a socket */
    s = socket(PF_INET, type, ppe->p_proto);
    if (s < 0)
        printf("can't create socket: %s\n", strerror(errno));

    /* Connect the socket */
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0)
        printf("can't connect to %s.%s: %s\n", host, service,
                strerror(errno));
    return s;
}

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
/* Read "n" bytes from a descriptor. */
ssize_t readn(int fd, void *vptr, size_t n)
{
    size_t	nleft;
    ssize_t	nread;
    char	*ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0) {
        if ( (nread = read(fd, ptr, nleft)) < 0) {
            if (errno == EINTR)
                nread = 0;		/* and call read() again */
            else
                return(-1);
        } else if (nread == 0)
            break;				/* EOF */

        nleft -= nread;
        ptr   += nread;
    }
    return(n - nleft);		/* return >= 0 */
}

/* A wrapper around readn */
ssize_t Readn(int fd, void *ptr, size_t nbytes)
{
    ssize_t		n;

    if ( (n = readn(fd, ptr, nbytes)) < 0)
        err_sys("readn error");
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
/* Print an error message and exit */
int errexit(const char *format, ...)
{
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    exit(1);
}

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


void ServerStatistics(void){
    time_t now;
    while (1){
        (void) sleep(INTERVAL);
        (void) pthread_mutex_lock(&stats.Mutex);

        now = time(0);
        (void) printf("--- %s", ctime(&now));
        (void) printf("%-32s: %u\n", "Current Connections", stats.ConnectionCount);
        (void) printf("%-32s: %u\n", "Completed Connections", stats.ConnectionTotal);

        if(stats.ConnectionTotal){
            float AvgConTime = (float)stats.ConnectionTime / (float)stats.ConnectionTotal;
            (void) printf("%-32s: %.2f (secs)\n", "Average Connection Time", AvgConTime);
            float AvgByteCount = (float)stats.ByteCount / ((float)stats.ConnectionTotal + (float)stats.ConnectionCount);
            (void) printf("%-32s: %.2f (secs)\n", "Average Byte Count", AvgByteCount);
        }
        (void) printf("%-32s: %lu\n\n", "Total Byte Count", stats.ByteCount);
        (void) pthread_mutex_unlock(&stats.Mutex);
    }
}

//Reports the number of miliseconds elapsed
long MsTime(unsigned long * pms)
{
    static struct timeval epoch;
    struct timeval now;

    if (gettimeofday(&now, (struct timezone*)0)){
        printf("ERROR: %s\n", strerror(errno));
    }

    if(!pms){
        epoch = now;
        return 0;
    }
    *pms = (now.tv_sec - epoch.tv_sec) * 1000;
    *pms += (now.tv_usec - epoch.tv_usec + 500) / 1000;
    return *pms;
}


void Tokenize(char *sentence, char** StorageArray, char * delimiter)
{
    TokenCount = 0;
    char *token;
    char *next_token;
    int counter = 1, position = 0;
    token = strtok(sentence, delimiter);
    next_token = token;
    while (next_token != NULL){
        TokenCount++;
        StorageArray[position] = token;
        //printf("Token: %s\n", StorageArray[position]);
        position++;
        if((next_token = strtok(NULL, delimiter))){
            token = next_token;
        }
        counter++;
    }
    counter--;
}

/** Returns true on success, or false if there was an error */
bool SetSocketBlockingEnabled(int fd, bool blocking)
{
    if (fd < 0) return false;
    int flags = fcntl(fd, F_GETFL, 0);
    if (flags < 0) return false;
    flags = blocking ? (flags&~O_NONBLOCK) : (flags|O_NONBLOCK);
    return (fcntl(fd, F_SETFL, flags) == 0) ? true : false;
}


/*Checksum function*/
int checksum(char* str){
    int i;
    int chk = 0x12345678;

    for (i = 0; str[i] != '\0'; i++){
        chk += ((int)(str[i]) * (i + 1));
    }
    return chk;
}

/*Number of places in an int. RAW speed version*/
int numDigi (int n) {

    if (n < 0) n = (n == INT_MIN) ? INT_MAX : -n;
    if (n < 10) return 1;
    if (n < 100) return 2;
    if (n < 1000) return 3;
    if (n < 10000) return 4;
    if (n < 100000) return 5;
    if (n < 1000000) return 6;
    if (n < 10000000) return 7;
    if (n < 100000000) return 8;
    if (n < 1000000000) return 9;
    if (n < 10000000000) return 10;
    if (n < 100000000000) return 11;
    if (n < 1000000000000) return 12;
    if (n < 10000000000000) return 13;
    if (n < 100000000000000) return 14;
    if (n < 1000000000000000) return 15;
    if (n < 10000000000000000) return 16;
    if (n < 100000000000000000) return 17;

    /*      2147483647 is 2^31-1 - add more ifs as needed
    and adjust this final return as well. */
    return 18;
}
