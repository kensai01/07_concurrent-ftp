//
// Created by kensai on 15/07/17.
//

#include "cmd.h"

/* Fetch the next command line. For interactive use or batch use, the lines are read from file.
 * Return 1 if OK, else 0 on error or end-of-file. */

int Getline(FILE *fp)
{
    if(fgets(line,MAXLINE, fp) == NULL) { return 0;} // error or end of file
    lineptr = line;
    return 1;
}

/* Fetch the next token from the input stream. We use the line that was set up in the most previous call to getline()
 * Return a pointer to the token (the argument), or NULL if no more exist. */
char * gettoken(char token[])
{
    register int c;
    register char *tokenptr;

    while ((c = *lineptr++) == ' ' || c == '\t');   /* Skip leading white space. */
    if (c == '\0' || c == '\n') { return NULL; }    /* Nothing there. */
    tokenptr = token;
    *tokenptr++ = c;                                /* First char of token. */

    /* Now collect everything up to the next space, tab, newline, or null.*/

    while ((c = *lineptr++) != ' ' && c != '\t' && c != '\n' && c != '\0')
    {
        *tokenptr++ = c;
    }
    *tokenptr = 0;                                  /* Add a null termination char to the end of token. */
    return(token);
}

/* Verify that there aren't any more tokens left on a command line. */
void checkend()
{
    if(gettoken(temptoken) != NULL){
        usr_err_cmd("trailing garbage");
    }
}

/* Execute a command. */
void docmd(char *cmdptr)
{
    register int i;
    if ((i = binary(cmdptr, ncmds)) < 0) { usr_err_cmd(cmdptr);}
    (*commands[i].cmd_func)();

    checkend();
}

/* Perform a binary search of the command table to see if a given token is a command. */
int binary(char *word, int n)
{
    register int low, high, mid, cond;
    low = 0;
    high = n-1;
    while (low<=high){
        mid = (low+high)/2;
        if((cond = strcmp(word, commands[mid].cmd_name)) < 0) { high = mid - 1; }
        else if (cond > 0) { low = mid + 1; }
        else return mid; /* Found it, return index in array...*/
    }
    return -1; /* Did not find it. */
}

/* Take a "host:file" char str and seperate the "host" portion from the "file portion.
 * input: "host:file" or just "file"
 * host = store "host" name here, if present*/
void striphost(char *fname, char *hname)
{
    char *index();
    register char *ptr1, *ptr2;

    if ((ptr1 = index(fname, ':')) == NULL) { return; } /* There is not a host: present...*/
    /* Copy the entire host:file into the hname array, then replace the colon with a null byte.*/
    strcpy(hname, fname);
    ptr2 = index(hname, ':');
    *ptr2 = 0; /* Null terminates the "host" string. */
    /* Now move the file string left in the fname array, removing the host: portion. */
    strcpy(fname, ptr1+1); /* increment by one to skip over the : */
}

/*User command error. Print out the command line too, for information. */
void usr_err_cmd(char *str)
{
    fprintf(stderr, "%s: '%s' command error", pname, command);
    if(strlen(str)> 0){ fprintf(stderr, ": %s", str);}
    fprintf(stderr, "\n");
    fflush(stderr);
    longjmp(jmp_mainloop, 1); /* 1 -> not a timeout, we've already printed our error message. */
}

/* Equivalent to 'mode ascii' */
//void cmd_ascii(){ modetype = MODE_ASCII; }

/* Equivalent to 'mode binary' */
//void cmd_binary(){ modetype = MODE_BINARY; }

/* connect <hostname> [ <port> ]
 *
 *  Set the hostname and optional port number for future transfers. The port is the well-known port number of the
 *  tftp server on the other system. Normally this will default to the value specified in /etc/services (69). */
void cmd_connect()
{
    register int val;
    if (gettoken(hostname) == NULL){ usr_err_cmd("missing hostname");}
    if (gettoken(temptoken) == NULL) { return; }
    val = atoi(temptoken);
    if (val < 0){ usr_err_cmd("invalid port number");}
    port = val;
}

/* Exit routine. */
void cmd_exit()
{
    exit(0);
}

/* get <remotefilename> <localfilename>
 *
 *  Note that the <remotefilename> may be of the form <host>:<filename> to specify the host also. */
void cmd_get()
{
    char remfname[MAXFILENAME], locfname[MAXFILENAME];
    char *index();

    if (gettoken(remfname) == NULL) {usr_err_cmd("the remote filename must be specified");}
    if (gettoken(locfname) == NULL) {usr_err_cmd("the local filename must be specified");}
    if (index(locfname, ':') != NULL) {usr_err_cmd("can't have 'host:' in local filename.");}
    striphost(remfname, hostname); /*check for 'host:' and process. */
    if(hostname[0] == 0) {usr_err_cmd("no host has been specified");}
    do_get(remfname, locfname);
}

/* Help. */
void cmd_help()
{
    register int i;
    for (i = 0; i < ncmds; i++) { printf(" %s\n", commands[i].cmd_name);}
}

/* mode ascii
 * mode binary
 *
 *  Set the mode for file transfers.
 *  */

/*void cmd_mode()
{
    if (gettoken(temptoken) == NULL){ usr_err_cmd("a mode type must be specified");}
    else {
        if(strcmp(temptoken, "ascii") == 0) {modetype = MODE_ASCII;}
        if(strcmp(temptoken, "binary") == 0) {modetype = MODE_BINARY;}
        else { usr_err_cmd("mode must be 'ascii' or 'binary'");}
    }
}*/

/* put <localfilename> <remotefilename>
 *
 *  Note that the <remotefilename> may be of the form <host>:<filename> to
 *  specify the host also. */
void cmd_put()
{
    char remfname[MAXFILENAME], locfname[MAXFILENAME];

    if(gettoken(locfname) == NULL) { usr_err_cmd("the local filename must be specified.");}
    if(gettoken(remfname) == NULL) { usr_err_cmd("the remote filename must be specified.");}
    if(index(locfname, ':') != NULL) { usr_err_cmd("can't have 'host:' in local filename");}
    striphost(remfname, hostname); /* check for 'host:' and process.*/
    if(hostname[0] == 0) { usr_err_cmd("no host has been specified.");}
    do_put(remfname, locfname);
}

/* Show current status. */
void cmd_status()
{
    if(connected){printf("Connected\n");}
    else {printf("Not connected\n");}

    //printf("mode = ");
    //switch (modetype) {
        //case MODE_ASCII: printf("netascii");    break;
        //case MODE_BINARY: printf("octet (binary)"); break;
        //default: net_err_dump("unknown modetype");
    //}
    //printf(", verbose = %s", verboseflag ? "on" : "off");
    printf(", trace = %s\n", traceflag ? "on" : "off");
}

/* Toggle debug mode. */
void cmd_trace() { traceflag = !traceflag;}

/* Toggle verbose mode. */
//void cmd_verbose() { verboseflag = !verboseflag;}


/* Execute a get command - read a remote file and store on the local system. */
void do_get(char *remfname, char *locfname)
{
    if ((localfp = file_open(locfname, "w", 1)) == NULL) {
        net_err_ret("can't fopen %s for writing", locfname);
        return;
    }
    if (cli_net_open(hostname, TFTP_SERVICE, port) < 0) return;
    totnbytes = 0;
    t_start(); /* start timer for statistics. */

    //send_RQ(OP_RRQ, remfname, modetype);
    //fsm_loop(OP_RRQ);

    t_stop(); /* stop timer for statistics */

    //net_close();
    //file_close(localfp);
    printf("Received %ld bytes in %.lf seconds\n", totnbytes, t_getrtime()); /* Print statistics. */
}

/* Execute a put command - read a remote file and store on the remote system. */
void do_put(char *remfname, char *locfname)
{
/*    if ((localfp = file_open(locfname, "r", 0)) == NULL) {
        net_err_ret("can't fopen %s for reading", locfname);
        return;
    }
    if (net_open(hostname, TFTP_SERVICE, port) < 0) return;
    totnbytes = 0;
    t_start(); *//* start timer for statistics. *//*

    lastsend = MAXDATA;
    send_RQ(OP_WRQ, remfname, modetype);
    fms_loop(OP_WRQ);

    t_stop(); *//* stop timer for statistics *//*

    net_close();
    file_close(localfp);
    printf("Sent %ld bytes in %.lf seconds\n", totnbytes, t_getrtime()); *//* Print statistics. */
}
