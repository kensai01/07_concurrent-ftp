/* Most of this code came from the Unix Network Programming book.
 * The code was tweaked as needed.*/
enum {MAXLINE = 4096};
#include <stdarg.h> 	//Needed for va_list
#include <errno.h> 	    //Needed to use errno
#include <strings.h> 	//needed for the bzero function
#include <stdio.h>      //standard C i/o facilities
#include <stdlib.h>     //needed for atoi()
#include <unistd.h>     //Unix System Calls
#include <sys/types.h>  //system data type definitions
#include <sys/socket.h> //socket specific definitions
#include <netinet/in.h> //INET constants and stuff
#include <arpa/inet.h>  //IP address conversion stuff
#include <stdbool.h>	//Needed for booleans 

/************* Wrappers ******************************************/

//socket creation wrapper
int Socket(int family, int type, int protocol);

//connect wrapper
bool Connect(int fd, const struct sockaddr *sa, socklen_t salen);

//close wrapper
bool Close(int fd);

//accept wrapper
int Accept(int fd, struct sockaddr *sa, socklen_t *salenptr);

//write wrapper
bool Write(int fd, void *ptr, size_t nbytes);

//bind wrapper
bool Bind(int fd, const struct sockaddr *sa, socklen_t salen);

//listen wrapper
bool Listen(int fd, int backlog);



//inet_pton wrapper
bool Inet_pton(int family, const char *strptr, void *addrptr);

//inet_ntop wrapper
bool Inet_ntop(int family, const void *addrptr, char *strptr, size_t len);

/************* Error Handling ************************************/

/* Print message and return to caller
 * Caller specifies "errnoflag"*/
void err_doit(int errnoflag, const char *fmt, va_list ap);

//Error related to system call
void err_sys(const char *fmt, ...);

//Error unrelated to system call
void err_ret(const char *fmt, ...);
