/* An example of a concurrent Echo server
 * To quit the client, hit Control D as that is the
 * NULL character */

#include "NP.h" //Includes a variety of functions

#define QLEN 32 /* maximum connection que length */

/* Catches ctrl-C and Quits */
void sig_quit(int signo){
	printf("Ctrl-C was entered.\n");
	exit(0);
}

/* Catches the SIGCHILD signal upon termination of a child
 * *UNUSED CURRENTLY*/
void sig_chld(int signo)
{
/*    pid_t   pid;
    int     stat;

    *//* WNOHANG means the call is not blocking. This is useful in case
     * SIGCHILD is raised for a different reason other than child
     * terminating. *//*
    while ( (pid = waitpid(-1, &stat, WNOHANG)) > 0) {
        printf("child %d terminated\n", (int) pid);
    }*/
    return;
}

int str_echo(int sockfd) {
    time_t start; /* Used for time tracking. */
    //ssize_t n;
    char buf[MAXLINE];
    int bytes;


    start = time(0); /* Connection start time. */
    /*Unlock, increase connection count shared variable then lock. */
    (void) pthread_mutex_lock(&stats.Mutex);
    stats.ConnectionCount++;
    (void) pthread_mutex_unlock(&stats.Mutex);

    /*Reading incoming bytes.*/
    while (bytes = read(sockfd, buf, sizeof buf)) {
        if (bytes < 0) {
            printf("ERROR: %s\n", strerror(errno));

            /*Write read bytes.*/
            if (write(sockfd, buf, bytes) < 0)
                printf("ERROR: %s\n", strerror(errno));

            /*Keep track of processed byte count.*/
            (void) pthread_mutex_lock(&stats.Mutex);
            stats.ByteCount += bytes;
            (void) pthread_mutex_unlock(&stats.Mutex);
        }

        Close(sockfd); /*Close open socket.*/

        /*House keeping some statistics after socket is closed.*/
        (void) pthread_mutex_lock(&stats.Mutex);
        stats.ConnectionTime = time(0) - start; /* Update the connection time.*/
        stats.ConnectionCount--; /*Decrement the current connection count. */
        stats.ConnectionTotal++; /*Increment the completed connections count. */
        (void) pthread_mutex_unlock(&stats.Mutex);
        return 0;
    }
}

//argc- stands for the number of arguments
//argv- stands for the argument vector
int main(int argc, char **argv) {
    /* Trap signal for Ctrl-C */
    Signal(SIGINT, sig_quit);

    /** Variable Creation **/
    pthread_t Thread;
    pthread_attr_t ThreadAttributes;
    struct sockaddr_in fsin;
    unsigned int alen;
    int mSockFd, sSockFd;
    char *service;

    /* Checks to see that two arguments were passed.
    * The first argument would be the command name.
    * The second argument would be the port number.*/

    if (argc == 1)
        err_quit("usage: command <PortNumber>");
    if (argc == 2)
        service = argv[1];

    /* Basially, create a TCP socket
     * If it fails, it will automatically quit the program due
     * to the wrapper being used. */
    mSockFd = PassiveSocket(service, "tcp", QLEN);

    (void) pthread_attr_init(&ThreadAttributes);
    (void) pthread_attr_setdetachstate(&ThreadAttributes, PTHREAD_CREATE_DETACHED);
    (void) pthread_mutex_init(&stats.Mutex, 0);

    /*Create a thread that displays the server statistics. */
    if (pthread_create(&Thread, &ThreadAttributes, (void *(*)(void *)) ServerStatistics, 0) < 0) {
        printf("ERROR: %s\n", strerror(errno));
    }

    /* Trap the signal for a child terminating. Call sig_child for it*/
    Signal(SIGCHLD, sig_chld);

    while (1) {
        /* 1) We first get the size of the client address data structure
         * 2) We call a regular accept and not the wrapper. The reason we
         *	  do so is to be able to check for EINTR. Basically, EINTR
         *	  means that a system call interrupted. In many cases, we
         *	  simply restart whatever we were doing. In this case,
         *	  going back to the top of the for loop.
         * 3) Otherwise, we got an error that is not EINTR and quit. */
        alen = sizeof(fsin);
        sSockFd = accept(mSockFd, (struct sockaddr *) &fsin, &alen);
        if (sSockFd < 0) {
            if (errno == EINTR)
                continue;
            printf("ERROR: %s\n", strerror(errno));
        }

        /* Here we create a thread to handle the connection. */
        if (pthread_create(&Thread, &ThreadAttributes, (void *(*)(void *)) str_echo, (void *) (long) sSockFd) < 0) {
            printf("ERROR: %s\n", strerror(errno));
        }
    }
}

