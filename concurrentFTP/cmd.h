/* Header file for user command processing functions. */


#ifndef CONCURRENTFTP_CMD_H
#define CONCURRENTFTP_CMD_H

#include "myallpurposeUNIXhelper.h"

extern char temptoken[]; /* temporary token for anyone to use. */

typedef struct Cmds {
    char *cmd_name;     /* Actual command string. */
    void (*cmd_func)();  /*Pointer to function. */

} Cmds;

extern Cmds commands[];
extern int ncmds;

/* All of the following functions are in cmd.c */
void cmd_connect(), cmd_exit(), cmd_get(), cmd_help(),
        cmd_put(), cmd_status(), cmd_trace();
int binary(char *word, int n);
void usr_err_cmd(char *str);

Cmds commands[] = {
        "?",        cmd_help,
        "binary",   cmd_connect,
        "exit",     cmd_exit,
        "get",      cmd_get,
        "help",     cmd_help,
        "put",      cmd_put,
        "quit",     cmd_exit,
        "status",   cmd_status,
        "trace",    cmd_trace,
};

#define NCMDS (sizeof(commands) / sizeof(Cmds))

int ncmds = NCMDS;

static char line[MAXLINE] = { 0 };
static char *lineptr = NULL;
/* Fetch the next command line. For interactive use or batch use, the lines are read from file.
 * Return 1 if OK, else 0 on error or end-of-file. */
int Getline(FILE *fp);

/* Fetch the next token from the input stream. We use the line that was set up in the most previous call to getline()
 * Return a pointer to the token (the argument), or NULL if no more exist. */
char * gettoken(char token[]);

/* Verify that there aren't any more tokens left on a command line. */
void checkend();

/* Execute a command. */
void docmd(char *cmdptr);

/* Perform a binary search of the command table to see if a given token is a command. */
int binary(char *word, int n);

/* Take a "host:file" char str and seperate the "host" portion from the "file portion.
 * input: "host:file" or just "file"
 * host = store "host" name here, if present*/
void striphost(char *fname, char *hname);

/*User command error. Print out the command line too, for information. */
void usr_err_cmd(char *str);

void do_get(char *remfname, char *locfname);
void do_put(char *remfname, char *locfname);

#endif //CONCURRENTFTP_CMD_H
