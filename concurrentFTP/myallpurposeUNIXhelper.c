/* These functions are a collection of useful functions from the book
 * Advanced Programming in the Unix Environment as well as functions from the book
 * UNIX Network Programming. */


#include <time.h>
#include "myallpurposeUNIXhelper.h"

int sockfd = -1;
char openhost[MAXHOSTNAME] = {0};
char    command[MAXTOKEN]       = { 0 };
int     connected               = 0;
char    hostname[MAXHOSTNAME]   = { 0 };
int     inetdflag               = 1;
int     interactive             = 1;
jmp_buf jmp_mainloop            = { 0 };
int     lastsend                = 0;
FILE    *localfp                = NULL;
int     nextblknum              = 0;
int     op_sent                 = 0;
int     op_recv                 = 0;
int     port                    = 0;
char    *prompt                 = "MB's FTP: ";
char    recvbuff[MAXBUFF]       = { 0 };
char    senduff[MAXBUFF]       = { 0 };
int     sebndlen                 = 0;
char    temptoken[MAXTOKEN]     = { 0 };
long    totnbytes               = 0;
int     traceflag               = 0;
char    *pname                  = NULL;

/* Turn on one or more of the file status flags for a descriptor.
 * flags are file status flags to turn on */
void set_fl(int fd, int flags)
{
    int     val;

    if ( (val = fcntl(fd, F_GETFL, 0)) < 0)
        err_sys("fcntl F_GETFL error");

    val |= flags;       /* turn on flags */

    if (fcntl(fd, F_SETFL, val) < 0)
        err_sys("fcntl F_SETFL error");
}

/* flags are file status flags to turn on */
void clr_fl(int fd, int flags)
{
    int     val;

    if ( (val = fcntl(fd, F_GETFL, 0)) < 0)
        err_sys("fcntl F_GETFL error");

    val &= ~flags;      /* turn flags off */

    if (fcntl(fd, F_SETFL, val) < 0)
        err_sys("fcntl F_SETFL error");
}

/* Function to lock/unlock a region of a file. */
int lock_reg(int fd, int cmd, int type, off_t offset, int whence, off_t len)
{
    struct flock lock;

    lock.l_type = type;     /* F_RDLCK, F_WRLCK, F_UNLCK */
    lock.l_start = offset;  /* byte offset, relative to l_whence */
    lock.l_whence = whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
    lock.l_len = len;       /* #bytes (0 means EOF) */

    return (fcntl(fd, cmd, &lock));
}

/* Function to test for a locking condition. */
pid_t lock_test(int fd, int type, off_t offset, int whence, off_t len)
{
    struct flock lock;

    lock.l_type = type;     /* F_RDLCK, F_WRLCK, F_UNLCK */
    lock.l_start = offset;  /* byte offset, relative to l_whence */
    lock.l_whence = whence; /* SEEK_SET, SEEK_CUR, SEEK_END */
    lock.l_len = len;       /* #bytes (0 means EOF) */

    if (fcntl(fd, F_GETLK, &lock) < 0) {
        err_sys("fcntl error");
    }

    if (lock.l_type == F_UNLCK) {
        return (0);     /* false, region is not locked by another process.*/
    }
    return(lock.l_pid); /* true, return pid of lock owner.*/
}

/* Print a description of the exit status. */
void pr_exit(int status){
    if (WIFEXITED(status)){
        printf("normal termination, exit status = %d\n", WEXITSTATUS(status));
    }
    else if(WIFSIGNALED(status)){
        printf("abnormal termination, signal number = %d%s\n", WTERMSIG(status),

#ifdef WCOREDUMP
                WCOREDUMP(status) ? " (core file generated)" : "");
#else
               "");}
#endif

    }    else if(WIFSTOPPED(status)){
        printf("child stopped, signal number = %d\n", WSTOPSIG(status));
    }
}

/** Parent/Child synchronization routines. **/
static int pfd1[2], pfd2[2];
void TELL_WAIT(void)
{
    if (pipe(pfd1) < 0 || pipe(pfd2) < 0){
        err_sys("pipe error");
    }
}
void TELL_PARENT(pid_t pid){
    if(write(pfd2[1], "c", 1) != 1){
        err_sys("write error");
    }
}
void TELL_CHILD(pid_t pid){
    if (write(pfd1[1], "p", 1) != 1){
        err_sys("write error");
    }
}
void WAIT_PARENT(void){
    char c;
    if (read(pfd1[0], &c, 1) != 1){
        err_sys("read error");
    }
    if (c != 'p'){
        err_quit("WAIT_PARENT: incorrect data");
    }
}

void WAIT_CHILD(void){
    char c;

    if (read(pfd2[0], &c, 1) != 1){
        err_sys("read error");
    }
    if (c != 'c'){
        err_quit("WAIT_CHILD: incorrect data");
    }
}


/** Read "n" bytes from a descriptor. **/
ssize_t readn(int fd, void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nread;
    char *ptr;

    ptr = vptr;
    nleft = n;
    while (nleft > 0){
        if ((nread = read(fd, ptr, nleft)) < 0){
            return(nread); /* error, return < 0 */
        }
        else if (nread == 0){
            break;          /*EOF*/
        }

        nleft -= nread;
        ptr += nread;
    }
    return(n - nleft);      /* return >= 0 */
}

/** Write "n" bytes to a descriptor. **/
ssize_t writen_(int fd, void *vptr, size_t n)
{
    size_t nleft;
    ssize_t nwritten;
    char *ptr;

    ptr = vptr; /* Unable to do ptr arithmetic on void. */
    nleft = n;
    while (nleft > 0){
        if ((nwritten = write(fd, ptr, nleft)) <= 0){
            return(nwritten); /* error */
        }

        nleft -= nwritten;
        ptr += nwritten;
    }
    return(n);      /* return >= 0 */
}

static void err_doit(int, const char *, va_list);

/* Nonfatal error related to a system call.
 * Print a message and return. */
void err_ret(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    return;
}

/* Fatal error related to a system call.
 * Print a message and terminate. */
void err_sys(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    exit(1);
}


/* Fatal error related to a system call.
 * Print a message, dump core & terminate. */
void err_dump(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);
    abort();
    exit(1);
}

/* Nonfatal error unrelated to a system call.
 * Print a message and return. */
void err_msg(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(0, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Fatal error unrelated to a system call.
 * Print a message and terminate. */
void err_quit(const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    err_doit(0, fmt, ap);
    va_end(ap);
    exit(1);
}

/* Print a message and return to caller.
 * Caller specifies "errnoflag". */
static void err_doit(int errnoflag, const char *fmt, va_list ap)
{
    int errno_save;
    char buf[MAXLINE];

    errno_save = errno; /* Value that can be printed. */
    vsprintf(buf, fmt, ap);
    if(errnoflag){
        sprintf(buf+strlen(buf), " : %s", strerror(errno_save));
    }
    strcat(buf, "\n");
    fflush(stdout);     /* Flush in case stdout and sterr are the same. */
    fputs(buf, stderr);
    fflush(NULL);       /* Flush all stdio output streams. */
}


/**	The function in this file are ind. of any app.
 * variables, and may be used with any C program.
 * Either of the names CLIENT or SERVER may be defined when
 * compiling this function. If neither are,
 * it is assumed CLIENT. **/
#ifdef	CLIENT
#ifdef 	SERVER
cant define both CLIENT and SERVER
#endif
#endif

#ifndef	CLIENT
#ifndef SERVER
#define CLIENT 1	/* Defaulting to client since none defined. */
#endif
#endif

#ifdef CLIENT 		/* These output to stderr */

/* Fatal Error. Print a message and terminate.
 * Don't dump core and don't print the system's errno value.
 *
 * 	err_quit(str, arg1, arg2, ...)
 *
 * The string "str" must specify the conversion specification for any args. */

void net_err_quit(const char *fmt, ...)
{
    va_list args;

    va_start(args, fmt);
    if (pname != NULL) { fprintf(stderr, "%s: ", pname); }
    //fmt = va_arg(args, char*);
    vfprintf(stderr, fmt, args);
    fputc('\n', stderr);
    va_end(args);

    exit(1);
}

/* Fatal Error related to a system call. Print a message and terminate.
 * Don't dump core, but do print the system's errno value and its associated
 * message.
 *
 * 	err_sys(str, arg1, arg2, ...)
 *
 * 	*/

void net_err_sys(const char *fmt, ...){
    va_list args;
    //char *fmt;

    va_start(args, fmt);
    if (pname != NULL) { fprintf(stderr, "%s: ", pname); }
    //fmt = va_arg(args, char*);
    vfprintf(stderr, fmt, args);
    va_end(args);

    my_perror();

    exit(1);
}

/* Recoverable error. Print a message and return to caller. */
void net_err_ret(const char *fmt, ...){
    va_list args;
    //char *fmt;

    va_start(args, fmt);
    if (pname != NULL) { fprintf(stderr, "%s: ", pname); }
    //fmt = va_arg(args, char*);
    vfprintf(stderr, fmt, args);
    va_end(args);

    my_perror();

    fflush(stdout);
    fflush(stderr);

    return;
}

/* Fatal Error. Print a message, dump core (for debugging) and terminate. */
void net_err_dump(const char *fmt, ...)
{
    va_list args;
    //char *fmt;

    va_start(args, fmt);
    if (pname != NULL) { fprintf(stderr, "%s: ", pname); }
    //fmt = va_arg(args, char*);
    vfprintf(stderr, fmt, args);
    va_end(args);

    my_perror();

    fflush(stdout);
    fflush(stderr);

    abort();
    exit(1);
}

/* Print the UNIX errno value. */
void my_perror() {
    char *sys_err_str();
    fprintf(stderr, " %s\n", sys_err_str());
}

#endif // DONE /w CLIENT

#ifdef SERVER

char emesgstr[255] = {0}; /* Used by all serv. routines. */

/*	LOG_PID is an option that says prepend each message with our pid
	LOG_CONS is an option that says write to console if unable to send
	the message to syslog.
	LOG_DAEMON is our facility.
*/
void err_init(char *ident)
{
	openlog(ident, (LOG_PID | LOG_CONS), LOG_DAEMON);
}

/* Fatal Error. Print a message and terminate.
 * Don't print the system's errno value.
 *
 * 	err_quit(str, arg1, arg2, ...)
 *
 * The string "str" must specify the conversion specification for any args. */

void net_err_quit(const char *fmt, ...)
{
	va_list args;

	va_start(args, fmt);
	//fmt = va_arg(args, char*);
	vsprintf(emesgstr, fmt, args);
	va_end(args);

	syslog(LOG_ERR, emesgstr);

	exit(1);
}

/* Fatal Error related to a system call. Print a message and terminate.
 * Don't dump core, but do print the system's errno value and its associated
 * message.
 *
 * 	err_sys(str, arg1, arg2, ...)
 *
 * 	*/

void net_err_sys(const char *fmt, ...)
{
	va_list args;
//	char *fmt;

	va_start(args, fmt);
//	fmt = va_arg(args, char*);
	vsprintf(emesgstr, fmt, args);
	va_end(args);

	my_perror();
	syslog(LOG_ERR, emesgstr);

	exit(1);
}

/* Recoverable error. Print a message and return to caller. */
void net_err_ret(const char *fmt, ...)
{
	va_list args;
	//char *fmt;

	va_start(args, fmt);
	//fmt = va_arg(args, char*);
	vsprintf(emesgstr, fmt, args);
	va_end(args);

	my_perror();
	syslog(LOG_ERR, emesgstr);

	return;
}

/* Fatal Error. Print a message, dump core (for debugging) and terminate. */
void net_err_dump(const char *fmt, ...)
{
	va_list args;
	//char *fmt;

	va_start(args);
	//fmt = va_arg(args, char*);
	vsprintf(emesgstr, fmt, args);
	va_end(args);

	my_perror();
	syslog(LOG_ERR, emesgstr);

	abort();
	exit(1);
}

/* Print the UNIX errno value. */
void my_perror() {
	register int len;
	char *sys_err_str();

	len = strlen(emesgstr);
	sprintf(emesgstr + len, " %s", sys_err_str());
}

#endif //DONE /w SERVER

/* Return a string containing some additional operating-system info. */
char * sys_err_str()
{
    static char msgstr[200];
    if(errno != 0){
        if (errno > 0 && errno < sys_nerr){ sprintf(msgstr, "(%s)", sys_errlist[errno]);}
        else { sprintf(msgstr, "(errno = %d)", errno);}
    }
    else { msgstr[0] = '\0';}
    return(msgstr);
}
char *h_errlist[];
int h_nerr;
/* Return a string containing some additional operating-system info. */
char * host_err_str()
{
    static char msgstr[200];
    if(h_errno != 0){
        if (h_errno > 0 && h_errno < h_nerr){ sprintf(msgstr, "(%s)", h_errlist[h_errno]);}
        else { sprintf(msgstr, "(h_errno = %d)", h_errno);}
    }
    else { msgstr[0] = '\0';}
    return(msgstr);
}

/*Start the timer.
 * We don't return anything to the caller, we just store some
 * information for the stop timer routine to access. */
void t_start()
{
    if(gettimeofday(&time_start, (struct timezone *) 0) < 0){ err_sys("t_start: gettimeofday() error");}
    if(getrusage(RUSAGE_SELF, &ru_start) < 0) {err_sys("t_start: getrusage() error");}
}
/*Stop the timer and save the appropriate information. */
void t_stop()
{
    if(getrusage(RUSAGE_SELF, &ru_stop) < 0){ err_sys("t_stop: getrusage() error");}
    if(gettimeofday(&time_stop, (struct timezone *) 0) < 0) { err_sys("t_stop: gettimeofday() error");}

}
/*Return the user time in seconds. */
double t_getutime()
{
    start = ((double) ru_start.ru_utime.tv_sec) * 1000000.0 + ru_start.ru_utime.tv_usec;
    stop = ((double) ru_stop.ru_utime.tv_sec) * 1000000.0 + ru_stop.ru_utime.tv_usec;
    seconds = (stop - start) / 1000000.0;
}

/*Return the system time in seconds. */
double t_getstime()
{
    start = ((double) ru_start.ru_stime.tv_sec) * 1000000.0 + ru_start.ru_stime.tv_usec;
    stop = ((double) ru_stop.ru_stime.tv_sec) * 1000000.0 + ru_stop.ru_stime.tv_usec;
    seconds = (stop - start) / 1000000.0;
}
/*Return the real(elapsed) time in seconds. */
double t_getrtime()
{
    start = ((double) time_start.tv_sec) * 1000000.0 + time_start.tv_usec;
    stop = ((double) time_start.tv_sec) * 1000000.0 + time_start.tv_usec;
    seconds = (stop - start) / 1000000.0;
}

/****************** Socket Wrappers ********************************/
/* Send a record to the other end. Used by client & server.
 * With a stream socket we have to preface each record with it's length
 * since our simple TFTP doesn't have a record length as part of each record.
 * We encode the length as a 2-byte integer in network byte order.
 * */
void net_send(char *buff, uint16_t len)
{
    register ssize_t rc;
    short templen;
    DEBUG1("net_send: sent %d bytes", len);

    templen = htons(len);
    rc = writen(sockfd, (char *) &templen, sizeof(short));
    if(rc != sizeof(short)){ net_err_dump("written error of length prefix");}

    rc = writen(sockfd, buff, len);
    if (rc != len){ net_err_dump("writen error");}
}

/* Receive a record from the other end, used by both client & server. */
ssize_t net_rcv(char *buff, int maxlen)
{
    register ssize_t nbytes;
    uint16_t templen;          /* value-result parameter */

    again1:
    if ((nbytes = readn(sockfd, (char *) &templen, sizeof(short))) < 0){
        if (errno == EINTR){
            errno = 0; /* assume SIGCLD */
            goto again1;
        }
        net_err_dump("readn error for length prefix");
    }
    if(nbytes != sizeof(short)){ net_err_dump("error in readn of length prefix");}
    templen = ntohs(templen); /* # of bytes that follow. */
    if(templen > maxlen) { net_err_dump("record length too large. ");}

    again2:
    if ((nbytes = readn(sockfd, buff, templen)) < 0){
        if (errno == EINTR){
            errno = 0; /* assume SIGCLD */
            goto again2;
        }
        net_err_dump("readn error");
    }
    if(nbytes != templen){ net_err_dump("error in readn");}
    DEBUG1("net_rcv: got %zu bytes", nbytes);

    return nbytes; /* Return the actual length of the message. */
}


/* Close the network connection, used by both client & server. */
void net_close()
{
    DEBUG2("net_close: host = %s, fd = %d", openhost, sockfd);
    close(sockfd);
    sockfd = -1;
}

/** Open the network connection. Client Version. **/
/* host =Name of other system to com. /w */
/* service =Name of service being requested. */
/* port = if > 0, use as port #, else use value for service.
*/
int cli_net_open(char *host, char *service, int port)
{
    if ( (sockfd = tcp_open(host, service, port)) < 0) { return -1; }
    DEBUG2("net_open: host %s, port# %d", inet_ntoa(tcp_srv_addr.sin_addr), ntohs(tcp_srv_addr.sin_port));
    strcpy(openhost, host); /* Save the hosts name. */
    return(0);
}

int tcp_open(char *host, char *service, int port)
{
    int		fd, resvport;
    unsigned long	inaddr;
    char		*host_err_str();
    struct servent	*sp;
    struct hostent	*hp;

    /*
     * Initialize the server's Internet address structure.
     * We'll store the actual 4-byte Internet address and the
     * 2-byte port# below.
     */

    bzero((char *) &tcp_srv_addr, sizeof(tcp_srv_addr));
    tcp_srv_addr.sin_family = AF_INET;

    if (service != NULL) {
        if ( (sp = getservbyname(service, "tcp")) == NULL) {
            err_ret("tcp_open: unknown service: %s/tcp", service);
            return(-1);
        }
        tcp_serv_info = *sp;			/* structure copy */
        if (port > 0)
            tcp_srv_addr.sin_port = htons(port);
            /* caller's value */
        else
            tcp_srv_addr.sin_port = sp->s_port;
        /* service's value */
    } else {
        if (port <= 0) {
            err_ret("tcp_open: must specify either service or port");
            return(-1);
        }
        tcp_srv_addr.sin_port = htons(port);
    }

    /*
     * First try to convert the host name as a dotted-decimal number.
     * Only if that fails do we call gethostbyname().
     */

    if ( (inaddr = inet_addr(host)) != INADDR_NONE) {
        /* it's dotted-decimal */
        bcopy((char *) &inaddr, (char *) &tcp_srv_addr.sin_addr,
              sizeof(inaddr));
        tcp_host_info.h_name = NULL;

    } else {
        if ( (hp = gethostbyname(host)) == NULL) {
            err_ret("tcp_open: host name error: %s %s",
                    host, host_err_str());
            return(-1);
        }
        tcp_host_info = *hp;	/* found it by name, structure copy */
        bcopy(hp->h_addr, (char *) &tcp_srv_addr.sin_addr,
              hp->h_length);
    }

    if (port >= 0) {
        if ( (fd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            err_ret("tcp_open: can't create TCP socket");
            return(-1);
        }

    } else if (port < 0) {
        resvport = IPPORT_RESERVED - 1;
        if ( (fd = rresvport(&resvport)) < 0) {
            err_ret("tcp_open: can't get a reserved TCP port");
            return(-1);
        }
    }

    /*
     * Connect to the server.
     */

    if (connect(fd, (struct sockaddr *) &tcp_srv_addr,
                sizeof(tcp_srv_addr)) < 0) {
        err_ret("tcp_open: can't connect to server");
        close(fd);
        return(-1);
    }

    return(fd);	/* all OK */
}


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
void Err_doit(int errnoflag, const char *fmt, va_list ap)
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
void Err_sys(const char *fmt, ...)
{
    va_list ap; //Type for iterating arguments
    va_start(ap, fmt); //Start iterating arguments
    err_doit(1, fmt, ap);
    va_end(ap); //Frees a va_list
    exit(1);
}

/* Fatal error unrelated to system call
 * Print message and terminate */
void Err_quit(const char *fmt, ...)
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


/*Routines to open/close/read/write the local file.
 * For "binary" (octet) transmissions, we use the UNIX open/read/write
 * system calls (or their equivalent)
 * For "ascii" (netascii) transmissions, we use the UNIX standard i/o routines
 * fopen/getc/putc (or their equivalent). */

/* The definitions are used by the functions in this file only. */
static int lastcr = 0; /* 1 if last character was a carriage-return */
static int nextchar = 0;

/* Open the local file for reading or writing.
 * Return a FILE pointer, or NULL on error.
 * mode = for fopen() - "r" or "w" */

FILE * file_open(char *fname, char* mode, int initblknum)
{
    register FILE *fp;

    if(strcmp(fname, "-") == 0) { fp = stdout; }
    else if((fp = fopen(fname, mode)) == NULL) { return((FILE *)0);}

    nextblknum = initblknum; /* for first data packet or first ACK */
    lastcr = 0;              /* for file_write() */
    nextchar = -1;           /* for file_read() */

    DEBUG2("file_open: opend %s", fname, MODE_BINARY);

    return fp;
}

/* Close the local file. This causes the standard i/o system to flush its buffers
 * for this file. */
void file_close(FILE *fp){
    if(lastcr){net_err_dump("final character was a CR");}
    if(nextchar >= 0){net_err_dump("nextchar >= 0");}
    if(fp == stdout) return;
    else if(fclose(fp) == EOF){net_err_dump("fclose error");}
}

/* Read data from the local file. Here is where we handle any conversion between the file's mode
 * on the local system and the network mode.
 *
 * Return the number of bytes read (between 1 and maxnbytes, inclusive) or 0 on EOF. */
ssize_t file_read(FILE *fp, register char *ptr, register size_t maxbytes)
{
    register ssize_t count;
    count = read(fileno(fp), ptr, maxbytes);
    if (count < 0){net_err_dump("read error on local file");}
    return count; /* will be 0 on EOF */

}

/* Write data to the local file. Here is where we handle any conversion between the mode of the file
 * on the network and the local system's conventions.
 */
ssize_t file_write(FILE *fp, register char *ptr, register size_t nbytes)
{
    register ssize_t i;
    i = write(fileno(fp), ptr, nbytes);
    if (i != nbytes){net_err_dump("write error on local file, i = %d", i);}
    return i; /* will be 0 on EOF */
}

