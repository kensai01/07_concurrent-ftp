/* Most of this code came from the Unix Network Programming book.
 * The code was tweaked as needed.*/
enum {MAXLINE = 4096, LISTENQ = 100};
#include <stdarg.h> 	//Needed for va_list
#include <errno.h> 	    //Needed to use errno
#include <strings.h> 	//needed for the bzero function
#include <string.h>
#include <stdio.h>      //standard C i/o facilities
#include <stdlib.h>     //needed for atoi()
#include <unistd.h>     //Unix System Calls
#include <sys/types.h>  //system data type definitions
#include <sys/socket.h> //socket specific definitions
#include <sys/errno.h>
#include <netinet/in.h> //INET constants and stuff
#include <arpa/inet.h>  //IP address conversion stuff
#include <stdbool.h>	//Needed for booleans 
#include <sys/wait.h>   //Needed for waitpid
#include <signal.h>		//Needed for signal handling
#include <pthread.h>    //Needed for thread usage
#include <sys/param.h>
#include <sys/ioctl.h>
#include <sys/time.h>
#include <netdb.h>

#define INTERVAL 5 /* interval in seconds */
#define CCOUNT 64*1024 /*Default Character Count */

#ifndef	INADDR_NONE
#define	INADDR_NONE	0xffffffff
#endif	/* INADDR_NONE */


struct {
    pthread_mutex_t Mutex;
    unsigned int ConnectionCount;
    unsigned int ConnectionTotal;
    unsigned long ConnectionTime;
    unsigned long ByteCount;
} stats;
/************* Socket Wrappers *************************************/

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

/************* I/O Wrappers ******************************************/

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

/*********** Address Conversion **************************************/

//inet_pton wrapper
void Inet_pton(int family, const char *strptr, void *addrptr);

//inet_ntop wrapper
void Inet_ntop(int family, const void *addrptr, char *strptr, size_t len);

/************* Error Handling ************************************/

/* Print message and return to caller
 * Caller specifies "errnoflag"*/
void err_doit(int errnoflag, const char *fmt, va_list ap);

//Error related to system call
void err_sys(const char *fmt, ...);

//Error unrelated to system call
void err_quit(const char *fmt, ...);


/************* Signal Handling *************************************/
/* Define a prototype for signal handlers */
typedef	void Sigfunc(int);

//Wrapper around the signal function
Sigfunc *Signal(int signo, Sigfunc *func); 

/*********************** Other ***************************************/

//Fork wrapper
pid_t Fork(void);

//Server Statistics Function
void ServerStatistics(void);

//Reports the number of miliseconds elapsed
long MsTime(unsigned long *);