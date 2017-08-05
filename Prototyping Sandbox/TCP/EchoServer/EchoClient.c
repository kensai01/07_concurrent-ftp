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

int PutCommand(), LsCommand();

//Client Controll Function
void ClientControll(void);
//Ls function
int ls();


/* argc- stands for the number of arguments
 * argv- stands for the argument vector storing the arguments */
 int main(int argc, char **argv) {
    int one = 1;


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

void ClientControll(void){

    printf("Welcome to TCP...\n");
    printf("Usage.\n");
    printf("ls: to list directory in client side\n");
    printf("!ls: to list directory on the server side\n");
    printf("put <infilename> <outfilename>\n");
    printf("get <outfilename> <infilename>\n");

    char sendline[MAXLINE], recvline[MAXLINE];
    char *buf = NULL;
    int outchars, inchars, n;
    char **CmdArray;
    char *CmdCpy;
    char *infile, *outfile;
    FILE *fd;
    long fileSize;
    char *fileBuf = NULL;
    char *tempBuf = NULL;
    int fileNameLen;
    int outfileNameLen;

    while (fgets(sendline, MAXLINE, stdin) != NULL) {
        CmdArray = malloc(128 * sizeof(char*));
        CmdCpy = malloc(255 * sizeof(char));
        strcpy(CmdCpy, sendline);
        Tokenize(CmdCpy, CmdArray, " ");

        if(TokenCount >= 1) CmdArray[0] = StripWhite(CmdArray[0]);
        if(TokenCount >= 2) CmdArray[1] = StripWhite(CmdArray[1]);
        if(TokenCount >= 3) CmdArray[2] = StripWhite(CmdArray[2]);

        //CmdArray[1] = StripWhite(CmdArray[1]);
        printf("token: %s\n", CmdArray[0]);
        printf("length of cmd %zu\n", strlen(CmdArray[0]));
        if((strcmp(CmdArray[0],"ls")) == 0) {
            ls();
            free(CmdArray);
            free(CmdCpy);
        }
        else if((strcmp(CmdArray[0], "!ls")) == 0){
            printf("Sending: %s\n", CmdArray[0]);
            //sendline[MAXLINE] = '\0';
            strcat(CmdArray[0], "\0");
            outchars = strlen(CmdArray[0]);
            Writen(sockfd, CmdArray[0], outchars);
            free(CmdArray);
            free(CmdCpy);
        }
        else if((strcmp(CmdArray[0], "put")) == 0 ){
            if(TokenCount != 3){ printf("ERROR: usage <put> <input file name> <output file name>\n");}
            int metaLen = 17; //("put <in filename> <out filename> ***text****")

            /* Open file */
            if ((fd = fopen(CmdArray[1], "rb")) == NULL) {
                printf("ERROR: %s\n", strerror(errno));
                //return 1;
            }

            /* Determine the size of the file to set buffer. */
            fseek(fd, 0L, SEEK_END );
            fileSize = ftell(fd);
            rewind(fd);

            fileNameLen = strlen(CmdArray[1]);
            outfileNameLen = strlen(CmdArray[2]);

            /* Initialize the buffer with file size. */
            int bufferSize = fileSize+1+fileNameLen+outfileNameLen+metaLen+1;
            if ((fileBuf = calloc(1, bufferSize)) == NULL) {
                fclose(fd);
                fputs("Failed to allocate memory.", stderr);
                //return 1;
            }
            if ((tempBuf = calloc(1, fileSize)) == NULL) {
                fclose(fd);
                fputs("Failed to allocate memory.", stderr);
                //return 1;
            }

            /* Add meta data to prefix buffer. */
            strcat(fileBuf, "put ");
            strcat(fileBuf, CmdArray[1]);
            strcat(fileBuf, " ");
            strcat(fileBuf, CmdArray[2]);
            strcat(fileBuf, " ");
            strcat(fileBuf, "***text****");

            printf("%s", fileBuf);

            /*Copy file into the temp buffer*/
            if((fread(tempBuf, fileSize, 1, fd)) != 1){
                fclose(fd);
                free(tempBuf);
                fputs("Read failed.", stderr);
                //return 1;
            }

            /*Add temp buffer to header.*/
            strcat(fileBuf, tempBuf);
            /* Add null terminated character. */
            strcat(fileBuf, "\0");

            /* A wrapper around writen. Returns true if write is successful. */
            Writen(sockfd, fileBuf, bufferSize);

            fclose(fd);
            free(fileBuf);
            free(CmdArray);
            free(CmdCpy);
            bufferSize = 0;
        }
        else{
            continue;
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

int GetCommand()
{
    if(TokenCount != 3){ printf("ERROR: usage <get> <fileonserver> <outputfile>\n"); return 0;}

    FILE *out_fd;
    int in_fd, rd_count, wt_count;
    char buffer[MAXLINE];
    printf("Receiving file from Server %s and saving as %s\n.", CmdArray[1], CmdArray[2]);
    /* Open the input file and create the output file */
    if ((out_fd = fopen(CmdArray[2], "w")) == NULL) {
        printf("ERROR: %s\n", strerror(errno));
        return 0;
    }
    /* Copy loop */
    bzero(buffer, MAXLINE);
    rd_count = 0;
    while ((rd_count = recv(sockfd, buffer, MAXLINE, 0)) >= 0) {
        /* read a block of data */
        printf("Read Count: %d:", rd_count);
        wt_count = fwrite(buffer, sizeof(char), rd_count, out_fd);
        printf("Write Count: %d:", wt_count);
        if(wt_count < rd_count){
            printf("File write failed!");
        }
        bzero(buffer, MAXLINE);
        if(rd_count <= 0) { break; }
    }
    if(rd_count < 0){
        if (errno == EAGAIN){ printf("ERROR: recv() timed out %s", strerror(errno));
        }
        else printf("recv() timed out.\n");
    }

    if(fclose(out_fd) == -1){
        printf("ERROR: %s\n", strerror(errno));
        return 0;
    }
    /* no error on last read */
    if (rd_count == 0)
        return 1;
    else
        return 0;  /* error on last read */
}

int PutCommand() {
    char *buffer = NULL;
    int string_size, read_size;
    int metaLen = 15; //("put <filename>***text****")
    int fileNameLen;
    int fileNameLen2;

    FILE *handler = fopen(CmdArray[1], "r");

    if (handler) {
        // Seek the last byte of the file
        fseek(handler, 0, SEEK_END);
        // Offset from the first to the last byte, or in other words, filesize
        string_size = ftell(handler);
        // go back to the start of the file
        rewind(handler);

        fileNameLen = strlen(CmdArray[1]);
        fileNameLen2 = strlen(CmdArray[2]);

        // Allocate a string that can hold it all
        buffer = (char *) malloc(sizeof(char) * (1 + fileNameLen2 + fileNameLen + string_size + 1 + metaLen));

        /* Add meta data to prefix buffer. */
        strcat(buffer, "put ");
        strcat(buffer, CmdArray[1]);
        strcat(buffer, " ");
        strcat(buffer, CmdArray[2]);
        strcat(buffer, "***text****");

        // Read it all in one operation
        read_size = fread(buffer, sizeof(char), string_size, handler);

        // fread doesn't set it so put a \0 in the last position
        // and buffer is now officially a string
        buffer[string_size] = '\0';

        if (string_size != read_size) {
            // Something went wrong, throw away the memory and set
            // the buffer to NULL
            free(buffer);
            buffer = NULL;
        }
    }

    /* A wrapper around writen. Returns true if write is successful. */
    Writen(sockfd, buffer, strlen(buffer));

    //fclose(fd);
    //free(buf);
}
