/* These functions are a collection of useful functions from the book
 * Advanced Programming in the Unix Environment as well as functions from the book
 * UNIX Network Programming.
 *
 * Also, herein is my work/learning following the book UNIX Networking Programming
 * written by W. Richard Stevens. In particular, Chapter 12 devoted
 * to a simple file transfer server.
 *
 /* Cataloged by: Mirza Besic 7/15/2017*/

#ifndef CONCURRENTFTP_MYALLPURPOSEUNIXHELPER_H
#define CONCURRENTFTP_MYALLPURPOSEUNIXHELPER_H

#include	<sys/types.h>	/* required for some of our prototypes */
#include	<stdio.h>		/* for convenience */
#include	<stdlib.h>		/* for convenience */
#include	<string.h>		/* for convenience */
#include    <strings.h>
#include	<unistd.h>		/* for convenience */
#include 	<fcntl.h>		/* for set_fd / clr_fd */
#include 	<syslog.h>
#include 	<sys/time.h>
#include 	<sys/resource.h>
#include 	<setjmp.h>
#include 	<signal.h>
#include 	<sys/param.h>
#include 	<errno.h>
#include 	<sys/file.h>
#include 	<sys/ioctl.h>
#include 	<sys/stat.h>
#include 	<sys/socket.h>
#include    <sys/errno.h>
#include    <stdarg.h>
#include    <netdb.h>
#include    <netinet/in.h>
#include    <arpa/inet.h>
#include    <stdbool.h>
#include    <sys/wait.h>
#include    <pthread.h>
#include    <time.h>

#define INTERVAL 5 /* interval in seconds */
#define CCOUNT 64*1024 /*Default Character Count */

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif	/* INADDR_NONE */

#define	MAXLINE			4096			/* max line length */
#define	MAXBUFF			2048		 	/* Transmit and receive buf len*/
#define MAXDATA			512				/* Max size of data per packet to send or receive, 512 is specified by RFC. */
#define MAXFILENAME		128				/* Max filename len */
#define MAXHOSTNAME		128				/* Max host name len */
#define	MAXCMDLINE		512				/* Max command line len */
#define	MAXTOKEN		128				/* Max token len */

#define	TFTP_SERVICE	"tftp"			/* Name of the service*/
#define	DAEMONLOG		"/tmp/tftpd.log" /* Log file for daemon tracing */

#define	FILE_MODE	(S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
                    /* default file access permissions for new files */
#define	DIR_MODE	(FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH)
                    /* default permissions for new directories */

/** Define the simple ftp opcodes. **/
#define OP_RRQ  1   /* Read Request. */
#define OP_WRQ  2   /* Write Request. */
#define OP_DATA 3   /* Data. */
#define OP_ACK  4   /* Acknowledgement. */
#define OP_ERROR  5 /* Error, see error codes below also. */

#define OP_MIN  1   /* Minimum opcode sent. */
#define OP_MAX  5   /* Maximum opcode received. */

/** Define the simple ftp error codes.
 * These are transmitted in an error packet (OP_ERROR) with an optional netascii Error Message
 * describing the error.
 * **/

#define ERR_UNDEF   0   /* Not defined, see error message. */
#define ERR_NOFILE  1   /* File not found. */
#define ERR_ACCESS  2   /* Access Violation. */
#define ERR_NOSPACE 3   /* Disk full or allocation exceeded. */
#define ERR_BADOP   4   /* Illegal tftp operation. */
#define ERR_BADID   5   /* Unknown TID (port#) */
#define ERR_FILE    6   /* File already exists. */
#define ERR_NOUSER  7   /* No such user. */

/*
 * Debug macro, based on the traceflag.
 * Note that a daemon typically freopen()s stderr to another file
 * for debugging purposes.
 */

#define MODE_BINARY 1
#define MODE_ASCII  0

#define	DEBUG(fmt)		if (traceflag) { \
					fprintf(stderr, fmt); \
					fputc('\n', stderr); \
					fflush(stderr); \
				} else ;

#define	DEBUG1(fmt, arg1)	if (traceflag) { \
					fprintf(stderr, fmt, arg1); \
					fputc('\n', stderr); \
					fflush(stderr); \
				} else ;

#define	DEBUG2(fmt, arg1, arg2)	if (traceflag) { \
					fprintf(stderr, fmt, arg1, arg2); \
					fputc('\n', stderr); \
					fflush(stderr); \
				} else ;

#define	DEBUG3(fmt, arg1, arg2, arg3)	if (traceflag) { \
					fprintf(stderr, fmt, arg1, arg2, arg3); \
					fputc('\n', stderr); \
					fflush(stderr); \
				} else ;

/**
 * Define macros to load and stoer 2-byte integers since these are used in the tftp headers for opcodes,
 * block numbers and error numbers. These macros handle the conversion between host format and network
 * byte ordering.
 * **/
#define ldshort(addr)       ( ntohs (* ( (u_short *) (addr) ) ) )
#define stshort(sval, addr) ( *( (u_short *)(addr)) = htons(sval) )

#ifdef lint /* hush up lint */
#undef ldshort
#undef stshort
short ldshort();
#endif /* lint */

/* Most of this code came from the Unix Network Programming book.
 * The code was tweaked as needed.*/
//enum {MAXLINE = 4096, LISTENQ = 100};

struct {
    pthread_mutex_t Mutex;
    unsigned int ConnectionCount;
    unsigned int ConnectionTotal;
    unsigned long ConnectionTime;
    unsigned long ByteCount;
} stats;

/** Externals **/
extern char command[];                  /* The command being processed. */
extern int  connected;                  /* True if we're connected to host. */
extern char hostname[];                 /* Name of host system. */
extern jmp_buf  jmp_mainloop;           /* To return to main command loop.*/
extern int  lastsend;                   /* #bytes of data in last data packet. */
extern FILE *localfp;                   /* File descriptor of local file to read or write. */
extern int  nextblknum;                 /* next block# to send/receive. */
extern char  *pname;                  /* the name by which we are invoked */
extern int  port;                       /* port number - host byte order, 0 use default. */
extern char *prompt;                    /* prompt string, for interactive use. */
extern long totnbytes;                  /* for get/put statistics printing. */
extern int traceflag;                   /* -t command line option, to "trace cmd/ */
extern char temptoken[];
/** One receive buffer and one transmit buffer. **/
extern char recvbuff[];
extern char sendbuff[];
extern int  sendlen;    /* #bytes in sendbuff[] */
extern int  op_sent;       /* Last opcode sent. */
extern int  op_recv;       /* Last opcode received. */



struct sockaddr_in	tcp_srv_addr;	/* server's Internet socket addr */
struct servent		tcp_serv_info;	/* from getservbyname() */
struct hostent		tcp_host_info;	/* from gethostbyname() */




/** PROCESS - PARENT / CHILD SYNC. ROUTINES**/
void	TELL_WAIT(void);
void	TELL_PARENT(pid_t);
void	TELL_CHILD(pid_t);
void	WAIT_PARENT(void);
void	WAIT_CHILD(void);

//Fork wrapper
pid_t Fork(void);


/**FILE / I/O and FILE DECORATOR ROUTINES**/
/************* I/O Wrappers ******************************************/
//Another write function
ssize_t file_write(FILE *fp, register char *ptr, register size_t nbytes);
//Another read function
ssize_t file_read(FILE *fp, register char *ptr, register size_t maxbytes);
//Another close function
void file_close(FILE *fp);

//write
void Writen(int fd, void *ptr, size_t nbytes);

//readline wrapper
ssize_t Readline(int fd, void *ptr, size_t maxlen);

//write
void Write(int fd, void *ptr, size_t nbytes);

//fclose
void Fclosee(FILE *fp);

//fdopen
FILE * Fdopen(int fd, const char *type);

//fgets
char * Fgets(char *ptr, int n, FILE *stream);

//fopen
FILE * Fopen(const char *filename, const char *mode);

//fputs
void Fputs(const char *ptr, FILE *stream);

char *gettoken(char token[]);
FILE * file_open(char *fname, char *mode, int initblknum);

/** Clear / set file descriptors. **/
void	 clr_fl(int, int);
void	 set_fl(int, int);
void     pr_exit(int);

/** Read / Write n bytes..**/
ssize_t	 readn(int, void *, size_t);
ssize_t	 writen_(int, void *, size_t);
static size_t writen(int fd, const void *vptr, size_t n);

/** Lock Record**/
int		lock_reg(int, int, int, off_t, int, off_t); /* {Prog lockreg} */


/** ERROR HANDLING ROUTINES **/
/** Standard Error Handling (from Adv. Prog. in the UNIX
 * Environment). **/
void	err_dump(const char *, ...);
void	err_msg(const char *, ...);
void	err_quit(const char *, ...);
void	err_ret(const char *, ...);
void	err_sys(const char *, ...);

/** Standard Error Handling for Client/Server (from UNIX Network
 * Programming) **/
void net_err_quit(const char *, ...);
void net_err_sys(const char *, ...);
void net_err_ret(const char *, ...);
void net_err_dump(const char *, ...);
void err_init(char *);
void my_perror();
char * sys_err_str();
char * host_err_str();

/************* Error Handling ************************************/
/* Print message and return to caller
 * Caller specifies "errnoflag"*/
void Err_doit(int errnoflag, const char *fmt, va_list ap);
//Error related to system call
void Err_sys(const char *fmt, ...);
//Error unrelated to system call
void Err_quit(const char *fmt, ...);

/** TIMER ROUTINES
 *
 * These routines are structured so that there are only the following
 * entry points:
 *
 * 	void	t_start()	start timer
 * 	void	t_stop()	stop timer
 * 	double	t_getrtime() return real (elapsed) time (seconds)
 * 	double 	t_getutime() return user time (seconds)
 * 	double 	t_getstime() return system time (seconds)
 *
 * 	Purposely, there are no structures passed back and forth between the
 * 	caller and these routines, and there are no include files
 * 	required by the caller.
 * 	**/

static struct timeval 	time_start, time_stop; /* for real time */
static struct rusage	ru_start, ru_stop;		/* for user & sys time */
static double 			start, stop, seconds;

/*Start the timer.
 * We don't return anything to the caller, we just store some
 * information for the stop timer routine to access. */
void t_start();
/*Stop the timer and save the appropriate information. */
void t_stop();
/*Return the user time in seconds. */
double t_getutime();
/*Return the system time in seconds. */
double t_getstime();
/*Return the real(elapsed) time in seconds. */
double t_getrtime();
//Reports the number of miliseconds elapsed
long MsTime(unsigned long *);

/************* Socket Wrappers *************************************/

/** Open the network connection. Client Version. **/
/* host =Name of other system to com. /w */
/* service =Name of service being requested. */
/* port = if > 0, use as port #, else use value for service.
*/
int cli_net_open(char *host, char *service, int port);

/*
* Open a TCP connection.
 * The following globals are available to the caller, if desired.
 * Return socket descriptor if OK, else -1 on error
 * name or dotted-decimal addr of other system
 * name of service being requested can be NULL, iff port > 0
 * if == 0, nothing special - use port# of service
 * if < 0, bind a local reserved port */
/* if > 0, it's the port# of server (host-byte-order) */
int tcp_open(char *host, char *service, int port);

//Passive socket creation (server)
int PassiveSocket(const char *service, const char *transport, int qlen);
//Active socket creation (client)
int ConnectSocket(const char *host, const char *service, const char *transport );

//socket creation
int Socket(int family, int type, int protocol);

//connect
void Connect(int fd, const struct sockaddr *sa, socklen_t salen);

//close
void Close(int fd);

//accept
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);

//bind
void Bind(int fd, const struct sockaddr *sa, socklen_t salen);

//listen
void Listen(int fd, int backlog);

/*********** Address Conversion **************************************/
//inet_pton wrapper
void Inet_pton(int family, const char *strptr, void *addrptr);

//inet_ntop wrapper
void Inet_ntop(int family, const void *addrptr, char *strptr, size_t len);


/************* Signal Handling *************************************/
/* Define a prototype for signal handlers */
typedef	void Sigfunc(int);

//Wrapper around the signal function
Sigfunc *Signal(int signo, Sigfunc *func);

/*********************** Other ***************************************/
//Server Statistics Function
void ServerStatistics(void);

#endif //CONCURRENTFTP_MYALLPURPOSEUNIXHELPER_H





