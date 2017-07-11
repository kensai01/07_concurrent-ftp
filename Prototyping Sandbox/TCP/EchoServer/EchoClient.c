#include <fcntl.h>
#define OUTPUT_MODE 0700 /* protection bits for output file */
#include "NP.h"

int StrCli(fd_set *pafds, int nfds, int ccount, int hcount);
int Reader(int fd, fd_set *pfdset);
int Writer(int fd, fd_set *pfdset);

char *HName[NOFILE]; /* File desc. to host name mapping. */
int ReadCount[NOFILE], WriteCount[NOFILE]; /*Read/WRite character counts.*/
char buf[MAXLINE];

/* argc- stands for the number of arguments
 * argv- stands for the argument vector storing the arguments */
 int main(int argc, char **argv) {
    int ccount = CCOUNT;
    int i, hcount, maxfd, sockfd;
    int one = 1;
    fd_set afds;

    hcount = 0;
    maxfd = -1;
    for (i = 1; i < argc; ++i) {
        if (strcmp(argv[i], "-c") == 0) {
            //TODO add some use to this or remove it
            if (++i < argc && (ccount == atoi(argv[i]))) { continue; }
            printf("usage: TODO - add me or remove me");
        }
    }
    /* More or less, create a TCP IPv4 socket
     * While PF_INET should be used in this case, for all practical purposes
     * AF_INET can be used instead for the protocol family. AF_INET represents
     * IPv4.
     * SOCK_STREAM provides sequenced,  reliable,  two-way,	connection-based
     * byte streams.
     * 0 is the default protocol version. */
    //TODO update wrapper functions to be tcp static, not offer udp option
    sockfd = ConnectSocket(argv[1], argv[2], "tcp");

    /* Checks to see that two arguments were passed.
    * The first argument would be the command name.
    * The second argument would be the IP address.*/
    if (argc != 3) {
        err_quit("usage: command <IPaddress> PortNumber");
        exit(1); //We just want to quit in this instance
    }

    //Use the socket we created earlier to connect to the server.
    if (ioctl(sockfd, FIONBIO, (char *) &one)) {
        printf("ERROR ioctl: %s\n", strerror(errno));
    }
    if (sockfd > maxfd) { maxfd = sockfd; }
    HName[sockfd] = argv[1];
    ++hcount;
    FD_SET(sockfd, &afds);

	//This is the main function you want to modify for HWK 2
    StrCli(&afds, maxfd+1, ccount, hcount);
	/* In UNIX, this closes all open descriptors. Not necessarily
	 * clean if ported to other systems. */
	exit(0);
}

int StrCli(fd_set *pafds, int nfds, int ccount, int hcount){

    fd_set ReadFileDesc, WriteFileDesc;
    fd_set ReadFileCpyDesc, WriteFileCpyDesc;

    int sockfd, i;

    for (i = 0; i < MAXLINE; ++i) buf[i] = 'D';

    memcpy(&ReadFileCpyDesc, pafds, sizeof(ReadFileCpyDesc));
    memcpy(&WriteFileCpyDesc, pafds, sizeof(WriteFileCpyDesc));

    for (sockfd = 0; sockfd < nfds; ++sockfd) ReadCount[sockfd] = WriteCount[sockfd] = ccount;

    (void) MsTime((unsigned long*) 0);

    while (hcount){
        memcpy(&ReadFileDesc, &ReadFileCpyDesc, sizeof(ReadFileDesc));
        memcpy(&WriteFileDesc, &WriteFileCpyDesc, sizeof(WriteFileDesc));


        if (select(nfds, &ReadFileDesc, &WriteFileDesc, (fd_set *)0, (struct timeval *)0) < 0) {
            printf("ERROR select: %s\n", strerror(errno));
        }

        for (sockfd = 0; sockfd<nfds; ++sockfd){
            if(FD_ISSET(sockfd, &ReadFileDesc)){
                if(Reader(sockfd, &ReadFileCpyDesc) == 0){
                    hcount--;
                }
            }
            if(FD_ISSET(sockfd, &WriteFileDesc)){
                Writer(sockfd, &WriteFileCpyDesc);
            }
        }
    }
    return 0;
}

//Handles reading from server
int Reader(int fd, fd_set *pfdset) {
    int in_fd, rd_count;
    unsigned long now;
    /* Open the input file and create the output file */
    in_fd = open("testfile", O_RDONLY);  /* open the source file */
    if (in_fd < 0) exit(2);  /* if it cannot be opened, exit */
    /* Copy loop */
    while (true) {
        rd_count = read(in_fd, buf, MAXLINE); /* read a block of data */
        if (rd_count <= 0) break; /* if end of file or error, exit loop */
    }

    ReadCount[fd] -= rd_count;
    if(ReadCount[fd]) return 1;
    (void) MsTime(&now);
    printf("%s: %lu ms\n", HName[fd], now);
    Close(fd);
    FD_CLR(fd, pfdset);


    /* Close the files */
    close(in_fd);
    if (rd_count == 0) /* no error on last read */
        exit(0);
    else
        exit(5);  /* error on last read */

/*    int cc;

    cc = read(fd, buf, sizeof(buf));
    if (cc < 0){
        printf("ERROR: read %s\n", strerror(errno));
    }
    if (cc == 0){
        printf("ERROR: Premature end of file.");
    }
    ReadCount[fd] -= cc;
    if(ReadCount[fd]) return 1;
    (void) MsTime(&now);
    printf("%s: %lu ms\n", HName[fd], now);
    Close(fd);
    FD_CLR(fd, pfdset);
    return 0;*/
}

//Handles writing to server
int Writer(int fd, fd_set *pfdset)
{
    int wt_count , out_fd, rd_count;
    out_fd = creat("testoutfile", OUTPUT_MODE);  /* create the destination file */
    if (out_fd < 0) exit(3);  /* if it cannot be created, exit */

    while (true) {
        wt_count = write(out_fd, buf, rd_count); /* wr ite data */
        if (wt_count <= 0) break;  /* wt_count <= 0 is an error */
    }
    close(out_fd);


    //int cc;

    //cc = write(fd, buf, MIN((int)sizeof(buf), WriteCount[fd]));
    if (wt_count < 0){
        printf("ERROR: write %s\n", strerror(errno));
    }
    WriteCount[fd] -= wt_count;
    if (WriteCount[fd] == 0){
        (void) shutdown(fd, 1);
        FD_CLR(fd, pfdset);
    }
    return 0;
}