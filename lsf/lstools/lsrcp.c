/*
 * Copyright (C) 2015 David Bigagli
 * Copyright (C) 2007 Platform Computing Inc
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of version 2 of the GNU General Public License as
 * published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.

 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA
 *
 */

#include "../lsf.h"
#include "../lib/lib.rcp.h"

extern void usage(char *cmd);

extern int mystat_(char *, struct stat *, struct hostent *);
extern int myopen_(char *, int, int, struct hostent *);

extern char **environ;

void displayXfer(lsRcpXfer *lsXfer);
void doXferUsage(void);
int createXfer(lsRcpXfer *lsXfer);
int destroyXfer(lsRcpXfer *lsXfer);
int doXferOptions(lsRcpXfer *lsXfer, int argc, char **argv);

int
main(int argc, char **argv)
{
    lsRcpXfer lsXfer;
    int iCount;
    char *buf;

    Signal_(SIGUSR1, SIG_IGN);

    if (ls_initdebug(argv[0]) < 0) {
        ls_perror("ls_initdebug");
        return -1;
    }

    if (ls_initrex(1,0) == -1) {
        ls_perror("lsrcp: ls_initrex");
        return -1;
    }

    ls_rfcontrol(RF_CMD_RXFLAGS, REXF_CLNTDIR);

    if (setuid(getuid()) < 0) {
        perror("lsrcp: setuid");
        goto handle_error;
    }

    if (createXfer(&lsXfer)) {
        perror("lsrcp");
        goto handle_error;
    }

    doXferOptions(&lsXfer, argc, argv);

    buf = malloc(LSRCP_MSGSIZE);

    for (iCount = 0; iCount < lsXfer.iNumFiles; iCount++) {

        if (copyFile(&lsXfer, buf, 0)) {
                ls_donerex();
                ls_syslog(LOG_ERR, "main(), res copy file failed, try rcp...");
                if (doXferRcp(&lsXfer, 0) < 0)
		    return -1;
                return 0;
        }
    }
    free(buf);

    ls_donerex();
    if (destroyXfer(&lsXfer)) {
        perror("lsrcp");
        return -1;
    }

    return 0;

handle_error:
    ls_donerex();
    return -1;
}


void
doXferUsage()
{
    fprintf(stderr, "usage: lsrcp [-h] [-a] [-V] f1 f2\n");
}

void
displayXfer(lsRcpXfer *lsXfer)
{
  if (lsXfer->szSourceArg)
        fprintf(stderr, "Source arg: %s\n", lsXfer->szSourceArg);

    if (lsXfer->szDestArg)
        fprintf(stderr, "Dest arg: %s\n", lsXfer->szDestArg);

    if (lsXfer->szHostUser)
        fprintf(stderr, "Host User: %s\n", lsXfer->szHostUser);

    if (lsXfer->szDestUser)
        fprintf(stderr, "Dest User: %s\n", lsXfer->szDestUser);

    if (lsXfer->szHost)
        fprintf(stderr, "Host: %s\n", lsXfer->szHost);

    if (lsXfer->szDest)
        fprintf(stderr, "Dest: %s\n", lsXfer->szDest);

    fprintf(stderr, "Num, Files: %d\n", lsXfer->iNumFiles);

    if (lsXfer->ppszHostFnames[0])
        fprintf(stderr, "Source Filename: %s\n", lsXfer->ppszHostFnames[0]);

    if (lsXfer->ppszDestFnames[0])
        fprintf(stderr, "Dest Filename: %s\n", lsXfer->ppszDestFnames[0]);


    fprintf(stderr, "Options: %d\n", lsXfer->iOptions);

}

int
doXferOptions(lsRcpXfer *lsXfer, int argc, char **argv)
{
    int c;

    while ((c = getopt(argc, argv,"ahV")) != -1) {
        switch(c) {
            case 'a':
                lsXfer->iOptions |= O_APPEND;
                break;
            case 'V':
                fputs(_LS_VERSION_,stderr);
                return -1;
            case '?':
                doXferUsage();
                return -1;
            case 'h':
                doXferUsage();
                return -1;
            case ':':
                doXferUsage();
                return -1;
        }
    }

    if (argc >= 3 && argv[argc-2]) {
        lsXfer->szSourceArg = strdup(argv[argc-2]);
        parseXferArg(argv[argc-2],&(lsXfer->szHostUser),
                     &(lsXfer->szHost),
                     &(lsXfer->ppszHostFnames[0]));
    } else {
        doXferUsage();
        return -1;
    }

    if (argc >= 3 && argv[argc-1]) {
    lsXfer->szDestArg = strdup(argv[argc-1]);
        parseXferArg(argv[argc-1],&(lsXfer->szDestUser),
                     &(lsXfer->szDest),
                     &(lsXfer->ppszDestFnames[0]));
    } else {
        doXferUsage();
        return -1;
    }

    return 0;
}

