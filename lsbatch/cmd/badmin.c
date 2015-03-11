/*
 * Copyright (C) 2007 Platform Computing Inc
 * Copyright (C) 2014 David Bigagli
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

#include "badmin.h"
#include <unistd.h>

extern char *myGetOpt (int nargc, char **nargv, char *ostr);
extern int checkConf (int, int);
extern int getConfirm (char *msg);
extern int lsb_debugReq(struct debugReq *pdebug , char *host);
extern int linux_optind;
extern int linux_opterr;
static int doBatchCmd (int argc, char *argv[]);
static int badminDebug (int nargc, char *nargv[], int opCode);


int
main(int argc, char **argv)
{
    int cc,  myIndex;
    char *prompt = "badmin>";
    static char line[MAXLINELEN];

    if (lsb_init(argv[0]) < 0) {
        lsb_perror("lsb_init");
        exit(-1);
    }

    setbuf(stdout, NULL);

    while ((cc = getopt(argc, argv, "Vh")) != EOF) {
        switch (cc) {
            case 'V':
                fputs(_LS_VERSION_, stderr);
                exit(0);
            case 'h':
            default:

                cmdsUsage("badmin",
                          cmdList,
                          cmdInfo);
        }
    }
    if (argc > optind) {
        int rc;

        if ((myIndex=adminCmdIndex(argv[optind], cmdList)) == -1) {
            fprintf(stderr, "Invalid command <%s> \n", argv[optind]);

            cmdsUsage("badmin", cmdList, cmdInfo);
        }
        optind++;
        rc = doBatchCmd (argc, argv);
        _i18n_end ( ls_catd );
        exit ( rc );
    }

    for (;;) {
        printf("%s", prompt);
        if (fgets(line, MAXLINELEN, stdin) == NULL) {
            printf("\n");
            exit(-1);
        }

        parseAndDo (line, doBatchCmd);
    }
    return 0;
}

static int
doBatchCmd(int argc, char *argv[])
{
    int cmdRet = 0, myIndex;

    if ((myIndex=adminCmdIndex(argv[optind-1], cmdList)) == -1) {
        fprintf(stderr,
                _i18n_msg_get(ls_catd,NL_SETN,2554, "Invalid command <%s>. Try help\n"),/* catgets  2554  */
                argv[optind-1]);
        return -1;
    }
    switch (myIndex) {
        case BADMIN_MBDRESTART :
            cmdRet = breconfig (argc, argv, MBD_RESTART);
            break;
        case BADMIN_RECONFIG :
            cmdRet = breconfig (argc, argv, MBD_RECONFIG);
            break;
        case BADMIN_CKCONFIG :
            cmdRet = breconfig (argc, argv, MBD_CKCONFIG);
            break;
        case BADMIN_QOPEN :
        case BADMIN_QCLOSE :
        case BADMIN_QACT :
        case BADMIN_QINACT :
            cmdRet = bqc (argc, argv, opCodeList[myIndex]);
            break;
        case BADMIN_HOPEN :
        case BADMIN_HCLOSE :
        case BADMIN_HREBOOT :
        case BADMIN_HSHUT :
            cmdRet = bhc (argc, argv, opCodeList[myIndex]);
            break;
        case BADMIN_HSTARTUP :
            startup (argc, argv, myIndex);
            break;
        case BADMIN_QHIST:
        case BADMIN_HHIST:
        case BADMIN_MBDHIST:
        case BADMIN_HIST:
            cmdRet = sysHist(argc, argv, opCodeList[myIndex]);
            break;
        case BADMIN_MBDDEBUG:
        case BADMIN_MBDTIME:
        case BADMIN_SBDDEBUG:
        case BADMIN_SBDTIME:
            cmdRet = badminDebug(argc, argv, opCodeList[myIndex]);
            break;

        case BADMIN_HELP :
        case BADMIN_QES:
            cmdHelp (argc, argv, cmdList,
                     _i18n_msgArray_get(ls_catd,NL_SETN, cmdInfo_ID, cmdInfo),
                     cmdSyntax);
            break;
        case BADMIN_QUIT:
            exit(0);
        default :
            fprintf(stderr, I18N_FUNC_S_ERROR, "adminCmdIndex()");
            exit(-1);
    }
    if (cmdRet == -2)
        oneCmdUsage(myIndex, cmdList, cmdSyntax);

    fflush(stderr);
    return (cmdRet);

}

int
breconfig(int argc, char **argv, int configFlag)
{
    char *optName;
    int vFlag = 0;
    int fFlag = 0;
    int checkReply;
    int stdoutsave;
    int fd;
    FILE *fp;
    char *linep;

    while ((optName = myGetOpt(argc, argv, "f|v|")) != NULL) {
        switch(optName[0]) {
            case 'v':
                vFlag = 1;
                break;
            case 'f':
                fFlag = 1;
                break;
            default:
                return(-2);
        }
    }
    if ( optind < argc ) {
        return(-2);
    }

    if (!vFlag && !fFlag) {

        fprintf(stderr, "\nChecking configuration files ...\n\n");

        stdoutsave = dup(1);
        fd = mkstemp("lsbatch");
        if (fd > 0 ) {

            dup2(fd, 1);
            dup2(fd, 2);
            checkReply = checkConf(1, 2);
            fflush(stderr);
            close(fd);
            dup2(stdoutsave, 1);
            dup2(stdoutsave, 2);

            fp = fdopen(fd, "r");
            if (fp != 0) {
                if (checkReply == EXIT_FATAL_ERROR
                    || checkReply == EXIT_WARNING_ERROR) {
                    if (checkReply == EXIT_FATAL_ERROR)
                        fprintf(stderr, "There are fatal errors.\n\n");
                    else
                        fprintf(stderr, "There are warning errors.\n\n");
                    fflush(stderr);

                    if (getConfirm("Do you want to see detailed messages? [y/n] "))
                        while ((linep = getNextLine_(fp, 0)))
                            fprintf(stderr, "%s\n", linep);
                }
                else
                    fprintf(stderr, "No errors found.\n\n");
                fflush(stderr);
            }
            fclose(fp);
            close(fd);
        }
        else
            checkReply = checkConf(0, 2);
    } else
        checkReply = checkConf(vFlag, 2);

    if (configFlag == MBD_CKCONFIG ) {
        return 0;
    }

    switch (checkReply)  {
        case EXIT_FATAL_ERROR :
            return -1;
        case EXIT_WARNING_ERROR :
            if (fFlag)
                break;
            if ( configFlag == MBD_RECONFIG ) {
                if (!getConfirm("\nDo you want to reconfigure? [y/n] ")) {
                    fprintf(stderr, "Reconfiguration aborted.\n");
                    return -1;;
                }
            } else {
                if (!getConfirm("\nDo you want to restart MBD? [y/n] ")) {
                    fprintf(stderr, "MBD restart aborted.\n");
                    return -1;;
                }
            }
        default :
            ;
    }

    if (lsb_reconfig(configFlag) < 0) {
        lsb_perror("Failed");
        return -1;;
    }

    if ( configFlag == MBD_RECONFIG ) {
        printf("%s\n",
               _i18n_msg_get(ls_catd,NL_SETN,2567, "Reconfiguration initiated")); /* catgets  2567  */
    } else {
        printf("%s\n",
               I18N(2569, "MBD restart initiated")); /* catgets  2569  */
    }
    return 0;

}



static int
badminDebug(int nargc, char *nargv[], int opCode)
{
    struct hostInfoEnt *hostInfo;
    char  opt[10];
    char **hostPoint;
    char *word;
    char **hosts=NULL;
    int i, c;
    int send;
    int retCode = 0;
    int  all = FALSE, numHosts = 0;
    struct debugReq  debug;

    debug.opCode = opCode ;
    debug.logClass = 0;
    debug.level = 0;
    debug.hostName = NULL;

    debug.logFileName[0] = '\0';
    debug.options = 0;

    if (opCode == MBD_DEBUG || opCode == SBD_DEBUG)
        strcpy(opt, "oc:l:f:");
    else if (opCode == MBD_TIMING || opCode == SBD_TIMING)
        strcpy (opt, "ol:f:");
    else
        return (-2);
    linux_optind = 1;
    linux_opterr = 1;
    if (strstr(nargv[0],"badmin")) {
        linux_optind++;
    }
    while ((c = getopt(nargc, nargv, opt )) != EOF) {

        switch (c) {
            case 'c':
                while (optarg != NULL && (word = getNextWord_(&optarg))) {
                    if (strcmp(word, "LC_SCHED") == 0)
                        debug.logClass |= LC_SCHED;

                    if (strcmp(word, "LC_EXEC") == 0)
                        debug.logClass |= LC_EXEC;

                    if (strcmp(word, "LC_TRACE") == 0)
                        debug.logClass |= LC_TRACE;

                    if (strcmp(word, "LC_COMM") == 0)
                        debug.logClass |= LC_COMM;

                    if (strcmp(word, "LC_XDR") == 0)
                        debug.logClass |= LC_XDR;

                    if (strcmp(word, "LC_CHKPNT") == 0)
                        debug.logClass |= LC_CHKPNT;

                    if (strcmp(word, "LC_FILE") == 0)
                        debug.logClass |= LC_FILE;

                    if (strcmp(word, "LC_AUTH") == 0)
                        debug.logClass |= LC_AUTH;

                    if (strcmp(word, "LC_HANG") == 0)
                        debug.logClass |= LC_HANG;

                    if (strcmp(word, "LC_SIGNAL") == 0)
                        debug.logClass |= LC_SIGNAL;

                    if (strcmp(word, "LC_PREEMPT") == 0)
                        debug.logClass |= LC_PREEMPT;

                    if (strcmp(word, "LC_SYS") == 0)
                        debug.logClass |= LC_SYS;

                    if (strcmp(word, "LC_JLIMIT") == 0)
                        debug.logClass |= LC_JLIMIT;

                    if (strcmp(word, "LC_PEND") == 0)
                        debug.logClass |= LC_PEND;

                    if (strcmp(word, "LC_LOADINDX") == 0)
                        debug.logClass |= LC_LOADINDX;

                    if (strcmp(word, "LC_M_LOG") == 0) {
                        debug.logClass |= LC_M_LOG;
                    }

                    if (strcmp(word, "LC_PERFM") == 0) {
                        debug.logClass |= LC_PERFM;
                    }

                    if (strcmp(word, "LC_JGRP") == 0) {
                        debug.logClass |= LC_JGRP;
                    }

                }
                if (debug.logClass == 0) {
                    fprintf(stderr,I18N(2572,"Command denied.Invalid class name\n")); /* catgets 2572 */
                    return -1;
                }
                break;

            case 'l':
                for (i = 0; i < strlen(optarg); i++) {
                    if (!isdigit(optarg[i])) {
                        fprintf(stderr,I18N(2573,"Command denied. Invalid level value\n")); /* catgets 2573 */
                        return -1;;
                    }
                }
                debug.level = atoi(optarg);
                if (opCode == MBD_DEBUG || opCode == SBD_DEBUG) {
                    if (debug.level < 0 || debug.level > 3) {
                        fprintf(stderr,I18N(2574,"Command denied. Valid debug level is [0-3] \n")); /* catgets 2574 */
                        return -1;;
                    }
                }
                else if (debug.level < 1 || debug.level > 5) {
                    fprintf(stderr,I18N(2575,"Command denied. Valid timing level is [1-5]\n")); /* catgets 2575 */
                    return -1;;
                }
                break;

            case 'f':
                if (strstr(optarg,"/") && strstr(optarg,"\\")) {
                    fprintf(stderr,I18N(2576,
                                        "Command denied. Invalid file name\n")); /*  catgets 2576 */
                    return -1;;
                }
                memset(debug.logFileName,0,sizeof(debug.logFileName));
                ls_strcat(debug.logFileName,sizeof(debug.logFileName),optarg);
                if (debug.logFileName[strlen(debug.logFileName)-1] == '/' ||
                    debug.logFileName[strlen(debug.logFileName)-1] == '\\') {
                    fprintf(stderr,I18N(2577,
                                        "Command denied. File name is needed after the path\n")); /*  catgets 2577 */
                    return -1;;
                }
                break;
            case 'o':
                debug.options = 1;
                break;

            default:
                return -2;
        }
    }


    if (opCode == SBD_DEBUG || opCode == SBD_TIMING) {

        numHosts = getNames(nargc, nargv, optind, &hosts, &all, "hostC");
        hostPoint = NULL;
        if (!numHosts && !all)
            numHosts = 1;
        else if (numHosts)
            hostPoint = hosts;

        if ((hostInfo = lsb_hostinfo (hostPoint, &numHosts)) == NULL) {
            lsb_perror(NULL);
            return -1;
        }

        for (i = 0; i < numHosts; i++) {
            if (strcmp(hostInfo[i].host, "lost_and_found") == 0 ) {
                if (!all)
                    fprintf(stderr, "%s.\n",
                            _i18n_msg_get(ls_catd,NL_SETN,2568, "<lost_and_found> is not a real host, ignored")); /* catgets  2568  */
                continue;
            }

            if (hostInfo[i].hStatus
                & (HOST_STAT_UNAVAIL | HOST_STAT_UNREACH)) {
                if (hostInfo[i].hStatus & HOST_STAT_UNAVAIL)
                    fprintf(stderr, "\
failed : LSF daemon (LIM) is unavailable on host %s\n",
                            hostInfo[i].host);
                else
                    fprintf(stderr, "\
failed : Slave batch daemon (sbatchd) is unreachable now on host %s\n",
                            hostInfo[i].host);
                continue;
            }

            if ((send = lsb_debugReq (&debug, hostInfo[i].host)) < 0) {
                char msg[118];
                sprintf(msg, "Operation denied by SBD on <%s>",
                        hostInfo[i].host);
                lsb_perror(msg);
                retCode = -1;
            }
        }
        return retCode;
    }

    else {
        numHosts = getNames(nargc, nargv, optind, &hosts, &all, "hostC");
        if (numHosts > 0) {
            fprintf(stderr, "\
Host name does not need to be specified, set debug to the host which runs MBD\n");
        }

        if ((send = lsb_debugReq (&debug, NULL)) < 0) {

            char msg[128];
            sprintf(msg, "Operation denied by MBD");
            lsb_perror(msg);
            return -1;
        }
    }
    return 0;

}

