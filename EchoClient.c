#include <fcntl.h>
#define OUTPUT_MODE 0700 /* protection bits for output file */
#include "NP.h"


char buf[MAXLINE];

/* Command Line Arguments */
char *command;
char *port;
char *server;
char **CmdArray;
char *CmdCpy;
int sockfd;


//Client Controll Function
void ClientControll(void);
//Ls function
int ls();


/* argc- stands for the number of arguments
 * argv- stands for the argument vector storing the arguments */
 int main(int argc, char **argv) {
    server = argv[1];
    port = argv[2];
    /* More or less, create a TCP IPv4 socket
     * While PF_INET should be used in this case, for all practical purposes
     * AF_INET can be used instead for the protocol family. AF_INET represents
     * IPv4.
     * SOCK_STREAM provides sequenced,  reliable,  two-way,	connection-based
     * byte streams.
     * 0 is the default protocol version. */
    sockfd = ConnectSocket(server, port, "tcp");

    /* Checks to see that two arguments were passed.
    * The first argument would be the command name.
    * The second argument would be the IP address.*/
    if (argc != 3) {
        err_quit("usage: command <IPaddress> PortNumber");
        exit(1); //We just want to quit in this instance
    }

    /* Run client control function. */
    ClientControll();

    /*Finished*/
	exit(0);
}

void ClientControll(void) {

    printf("Welcome to TCP...\n");
    printf("Usage.\n");
    printf("ls: to list directory in client side\n");
    printf("!ls: to list directory on the server side\n");
    printf("put <infilename> <outfilename>\n");
    printf("get <outfilename> <infilename>\n");

    char sendline[MAXLINE];
    int outchars;
    char **CmdArray;
    char **TmpArray;
    char *CmdCpy;
    FILE *fd;
    long fileSize;
    char *fileBuf = NULL;
    char *tempBuf = NULL;
    int fileNameLen;
    int outfileNameLen;
    char fileBuf1[1000000];

    while (fgets(sendline, MAXLINE, stdin) != NULL) {
        /*Setup of variables and arrays.*/
        CmdArray = malloc(128 * sizeof(char *));
        TmpArray = malloc(1000000 * sizeof(char *));
        CmdCpy = malloc(255 * sizeof(char));
        strcpy(CmdCpy, sendline);

        /*Tokenize Command and strip whitespace. */
        Tokenize(CmdCpy, CmdArray, " ");
        if (TokenCount >= 1) CmdArray[0] = StripWhite(CmdArray[0]);
        if (TokenCount >= 2) CmdArray[1] = StripWhite(CmdArray[1]);
        if (TokenCount >= 3) CmdArray[2] = StripWhite(CmdArray[2]);

        if ((strcmp(CmdArray[0], "ls")) == 0) { ls(); }

        if ((strcmp(CmdArray[0], "!ls")) == 0) {
            printf("Sending: %s\n", CmdArray[0]);
            outchars = strlen(CmdArray[0]);
            Writen(sockfd, CmdArray[0], outchars);
            close(sockfd);
            exit(0);
        }

        if ((strcmp(CmdArray[0], "get")) == 0) {
            /*Lock writing so only one thread can write to a file.*/
            (void) pthread_mutex_lock(&stats.Mutex);

            if (TokenCount != 3) { printf("ERROR: usage <get> <output file name> <input file name>\n"); }
            int metaLen = 5; // get and two spaces

            fileNameLen = strlen(CmdArray[1]);
            outfileNameLen = strlen(CmdArray[2]);

            /* Initialize the buffer with file size. */
            int bufferSize = metaLen + fileNameLen + outfileNameLen;

            if ((tempBuf = calloc(1, fileSize)) == NULL) {
                fputs("Failed to allocate memory.", stderr);
            }

            /* Fill buffer. */
            strcat(tempBuf, "get");
            strcat(tempBuf, " ");
            strcat(tempBuf, CmdArray[1]);
            strcat(tempBuf, " ");
            strcat(tempBuf, CmdArray[2]);
            strcat(tempBuf, "\0");

            printf("%s\n", tempBuf);

            /*Write the request to the server asking for the file.*/
            Writen(sockfd, tempBuf, bufferSize);
            shutdown(sockfd, 1);
            printf("Written completed.");

            /*Clear out the buffer the file will go into*/
            bzero(fileBuf1, 1000000);

            /*Read back the file and the checksum.*/
            Readn(sockfd, fileBuf1, sizeof(fileBuf1));
            shutdown(sockfd, 0);
            printf("Readn completed.");

            /*Remove the file from the checksum so we can check it.*/
            Tokenize(fileBuf1, TmpArray, "~");

            /*Checksum setup and calculation.*/
            int chcksum = 0;
            chcksum = checksum(TmpArray[0]);
            fprintf(stdout, "Buffers checksum %u\n", chcksum);

            int sentChcksum = 0;
            /*Received cheksum value as int.*/
            if (TmpArray[1] != NULL) {
                sentChcksum = atoi(TmpArray[1]);
                fprintf(stdout, "Sent checksum %u\n", sentChcksum);
            }

            if(chcksum != sentChcksum){
                fprintf(stdout, "Checksums don't match! Not writing file to client, connect & try again.\n");
            }

            //printf("inc: %s\n", fileBuf1);

            if ((fd = fopen(CmdArray[2], "w")) == NULL) {
                fputs("File open failed.", stderr);
            }
            int bufSize = strlen(TmpArray[0]);

            fprintf(stdout, "buffer size: %u\n", bufSize);
            if (fd) {
                fwrite(TmpArray[0], bufSize, 1, fd);
            }
            /*Free memory*/
            fclose(fd);

            bzero(fileBuf1, 1000000);
            bufferSize = 0;
            bufSize = 0;

            /*Unlock writing.*/
            (void) pthread_mutex_unlock(&stats.Mutex);
            printf("File writen to client correctly!\n");
            printf("To run again: ./filename <ip address> <port>\n");
            exit(0);
        }

        if ((strcmp(CmdArray[0], "put")) == 0) {
            if (TokenCount != 3) { printf("ERROR: usage <put> <input file name> <output file name>\n"); }
            int metaLen = 7; //("put <in filename> <out filename> ~")

            /* Open file */
            if ((fd = fopen(CmdArray[1], "rb")) == NULL) {
                printf("ERROR: %s\n", strerror(errno));
            }

            /* Determine the size of the file to set buffer. */
            fseek(fd, 0L, SEEK_END);
            fileSize = ftell(fd);
            rewind(fd);
            printf("File size: %ld", fileSize);

            fileNameLen = strlen(CmdArray[1]);
            outfileNameLen = strlen(CmdArray[2]);

            /* Initialize the buffer with file size. */
            int bufferSize = fileSize + 1 + fileNameLen + outfileNameLen + metaLen + 1;
            if ((fileBuf = calloc(1, bufferSize)) == NULL) {
                fclose(fd);
                fputs("Failed to allocate memory.", stderr);
            }
            if ((tempBuf = calloc(1, fileSize)) == NULL) {
                fclose(fd);
                fputs("Failed to allocate memory.", stderr);
            }

            /* Add meta data to prefix buffer. */
            strcat(fileBuf, "put ");
            strcat(fileBuf, CmdArray[1]);
            strcat(fileBuf, " ");
            strcat(fileBuf, CmdArray[2]);
            strcat(fileBuf, " ");

            printf("%s", fileBuf);

            /*Copy file into the temp buffer*/
            if ((fread(tempBuf, fileSize, 1, fd)) != 1) {
                fclose(fd);
                free(tempBuf);
                fputs("Read failed.", stderr);
            }

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
            strcat(fileBuf, chcksumChar);
            strcat(fileBuf, " ");
            strcat(fileBuf, "~");

            /*Add checksum size to the buffer calculation.*/
            bufferSize = bufferSize + numDigits + 1; // +1 for the extra space

            /*Add temp buffer(file) to header(all other stuff).*/
            strcat(fileBuf, tempBuf);
            /* Add null terminated character. */
            strcat(fileBuf, "\0");

            fclose(fd);

            /* A wrapper around writen. Returns true if write is successful. */
            Writen(sockfd, fileBuf, bufferSize);

            free(fileBuf);
            free(tempBuf);
            bufferSize = 0;

            /*If this exit is removed, it hangs... why!?*/
            printf("File writen to client correctly!\n");
            printf("To run again: ./filename <ip address> <port>\n");
            exit(0);

        } else {
            printf("Command doesn't match any available.");
        }
    }
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