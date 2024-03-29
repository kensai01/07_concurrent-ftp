/* An example of a concurrent Echo server
 * To quit the client, hit Control D as that is the
 * NULL character */

#include "NP.h" //Includes a variety of functions

#define QLEN 32 /* maximum connection que length */


/*Ls function for server.*/
int ls();

int str_echo(int sfd);


/* Catches ctrl-C and Quits */
void sig_quit(int signo){
	printf("Ctrl-C was entered.\n");
	exit(0);
}

/* Catches the SIGCHILD signal upon termination of a child */
void sig_chld(int signo)
{
    return;
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
    char *service = NULL;
    int mSockFd, sSockFd;

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

int str_echo(int sfd) {

    time_t start;
    char **buf;
    char fileBuf[1000000];
    int cc;
    char **CmdArray;
    char *CmdCpy;
    char *TmpCpy;
    FILE *fd;

    start = time(0); /* Connection start time. */


    /*Unlock, increase connection count shared variable then lock. */
    (void) pthread_mutex_lock(&stats.Mutex);
    stats.ConnectionCount++;
    (void) pthread_mutex_unlock(&stats.Mutex);

    while ((cc = Readn(sfd, fileBuf, sizeof(fileBuf))) != 0) {

        CmdArray = malloc(1000000 * sizeof(char *));
        buf = malloc(1000000 * sizeof(char *));
        CmdCpy = malloc(1000000 * sizeof(char));
        TmpCpy = malloc(1000000 * sizeof(char));

        strcpy(TmpCpy, fileBuf);
        strcpy(CmdCpy, fileBuf);
        Tokenize(CmdCpy, CmdArray, " ");

        /*Remove header fully. TmpCpy will have the contents of file to put on server w/o header.*/
        Tokenize(TmpCpy, buf, "~");

        if (TokenCount >= 1) CmdArray[0] = StripWhite(CmdArray[0]);
        if (TokenCount >= 2) CmdArray[1] = StripWhite(CmdArray[1]);
        if (TokenCount >= 3) CmdArray[2] = StripWhite(CmdArray[2]);

        int sentChcksum = 0;
        /*Received cheksum value as int.*/
        if (CmdArray[3] != NULL) {
            sentChcksum = atoi(CmdArray[3]);
            fprintf(stdout, "Sent checksum %u\n", sentChcksum);
        }

        if ((strcmp(CmdArray[0], "!ls")) == 0) {
            fprintf(stdout, "About to run ls /w this: %s\n", CmdArray[0]);
            ls();
        }
        else if ((strcmp(CmdArray[0], "get")) == 0){

            int fileSize = 0;
            char *tempBuf = NULL;

            if (TokenCount != 3) { printf("Not enough tokens for get.\n"); }

            /* Open file */
            if ((fd = fopen(CmdArray[1], "rb")) == NULL) {
                printf("ERROR: %s\n", strerror(errno));
            }

            /* Determine the size of the file to set buffer. */
            if (fd) {
                fseek(fd, 0L, SEEK_END);
                fileSize = ftell(fd);
                rewind(fd);
            }

            /* Initialize the buffer with file size. */
            int bufferSize = fileSize+1;

            if ((tempBuf = calloc(1, fileSize)) == NULL) {
                fclose(fd);
                fputs("Failed to allocate memory.", stderr);
            }

            /*Copy file into the temp buffer*/
            if((fread(tempBuf, fileSize, 1, fd)) != 1){
                fclose(fd);
                free(tempBuf);
                fputs("Read failed.", stderr);
                //return 1;
            }

            /* Add null terminated character. */
            strcat(tempBuf, "\0");
            fclose(fd);

            /*Checksum setup and calculation.*/
            int chcksum = 0;
            chcksum = checksum(tempBuf);
            printf("Checksum: %u\n", chcksum);

            /*Get the number of digits in the checksum integer.*/
            int numDigits = numDigi(chcksum);
            numDigits++;
            printf("Num Digits: %u\n", numDigits);

            /*Convert checksum integer into a character array to add into the buffer.*/
            char* chcksumChar = malloc(numDigits * sizeof(int));
            sprintf(chcksumChar, "%u", chcksum);
            printf("Checksum: %s\n", chcksumChar);

            /*Add the checksum character to the buffer.*/
            strcat(tempBuf, "~");
            strcat(tempBuf, chcksumChar);

            bufferSize = bufferSize + numDigits + 2;

            /* A wrapper around writen. Returns true if write is successful. */
            Writen(sfd, tempBuf, bufferSize);

            bufferSize = 0;
            (void) pthread_mutex_lock(&stats.Mutex);
            stats.ByteCount += fileSize;
            (void) pthread_mutex_unlock(&stats.Mutex);
        }
        else if ((strcmp(CmdArray[0], "put")) == 0) {
            /*Lock writing.*/
            (void) pthread_mutex_lock(&stats.Mutex);

            /*Checksum setup and calculation.*/
            int chcksum = 0;
            chcksum = checksum(buf[1]);
            fprintf(stdout, "Buffers checksum %u\n", chcksum);

            if(chcksum != sentChcksum){
                fprintf(stdout, "Checksums don't match! Not writing file, connect & try again.\n");
            }

            /* Open file for writing. */
            if ((fd = fopen(CmdArray[2], "w")) == NULL) {
                fputs("File open failed.", stderr);
            }

            /*Write the buffers contents to opened file.*/
            int bufSize = strlen(buf[1]);
            fprintf(stdout, "buffer size: %u\n", bufSize);
            if(fd){
                fwrite(buf[1], bufSize, 1, fd);
            }

            /*Free memory*/
            fclose(fd);
            /*Unlock writing.*/
            (void) pthread_mutex_unlock(&stats.Mutex);

            //break;
        }

        if (cc < 0) {
            errexit("ERROR: read %s\n", strerror(errno));
        }

        (void) pthread_mutex_lock(&stats.Mutex);
        stats.ByteCount += cc;
        (void) pthread_mutex_unlock(&stats.Mutex);

        free(CmdArray);
        free(CmdCpy);
        free(buf);
        bzero(fileBuf, 1000000);
    }

    (void) close(sfd);
    /*House keeping some statistics after socket is closed.*/
    (void) pthread_mutex_lock(&stats.Mutex);
    stats.ConnectionTime = time(0) - start; /* Update the connection time.*/
    stats.ConnectionCount--; /*Decrement the current connection count. */
    stats.ConnectionTotal++; /*Increment the completed connections count. */
    (void) pthread_mutex_unlock(&stats.Mutex);
    return 0;
}

int ls()
{
    DIR * d;
    char * dir_name = ".";
    /* Open the current directory. */
    d = opendir (dir_name);
    if (! d) {
        fprintf (stderr, "Cannot open directory '%s': %s\n",
                 dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }
    while (1) {
        struct dirent * entry;
        entry = readdir (d);
        if (! entry) {
            break;
        }
        printf ("%s\n", entry->d_name);
    }
    /* Close the directory. */
    if (closedir (d)) {
        fprintf (stderr, "Could not close '%s': %s\n",
                 dir_name, strerror (errno));
        exit (EXIT_FAILURE);
    }
    return 0;
}
