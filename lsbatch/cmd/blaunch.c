/*
 * Copyright (C) 2016 David Bigagli
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


#include "cmd.h"
#include "../lib/lsb.h"

static void
usage(void)
{
    fprintf(stderr, "blaunch: [-h] [-V] [-n] [-z hosts...]\n");
}

/* Global variables we revolve around
 */
static int arg_count;
static char **arg_char;
static char **host_list;
static char **command;
static int *tasks;
struct jRusage **jrus;
static int verbose;
static int rest;
extern char **environ;
static char buf[BUFSIZ];
static int jobID;
static int job_step;
static char *hostname;

/* Functions using global variables
 */
static int redirect_stdin(void);
static int get_host_list(const char *);
static int get_host_list_from_file(const char *);
static int count_lines(FILE *);
static int get_hosts_from_env(void);
static int get_hosts_from_arg(void);
static void make_tasks(void);
static void build_command(void);
static int run_job(void);
static void free_hosts(void);
static void free_command(void);
static int rusage_task(int);
static void print_hosts(void);
static int size_rusage(struct jRusage *);
static int get_job_id(void);
static int send2sbd(struct jRusage *);
static void free_rusage(struct jRusage *);

int
main(int argc, char **argv)
{
    int cc;
    char z;
    char u;

    /* If this is set, then option processing stops as soon as a
     * non-option argument is encountered.
     */
    setenv("POSIXLY_CORRECT", "Y", 1);

    arg_count = argc;
    arg_char = argv;

    if (argc <= 1) {
        usage();
        return -1;
    }

    rest = 60;
    u = z = 0;
    while ((cc = getopt(argc, argv, "hVvnz:u:t:")) != EOF) {
        switch (cc) {
            case 'v':
                ++verbose;
                break;
            case 'n':
                redirect_stdin();
                break;
            case 'z':
                if (u == 0)
                    get_host_list(optarg);
                ++z;
                break;
            case 'u':
                if (z == 0)
                    get_host_list_from_file(optarg);
                ++u;
                break;
            case 't':
                rest = atoi(optarg);
                break;
            case 'h':
            case '?':
                usage();
                return(-1);
        };
    }

    Signal_(SIGUSR1, SIG_IGN);

    /* initialize the remote execution
     * library
     */
    if (ls_initrex(1, 0) < 0) {
        fprintf(stderr, "blaunch: ls_initrex() failed %s", ls_sysmsg());
        return -1;
    }

    /* Since we call SBD initialize the batch library as well.
     */
    if (lsb_init("blaunch") < 0) {
        fprintf(stderr, "blaunch: lsb_init() failed %s", lsb_sysmsg());
        return -1;
    }

    /* Open log to tell user what's going on
     */
    ls_openlog("blaunch", NULL, true, "LOG_INFO");

    if (get_job_id()) {
        ls_syslog(LOG_ERR, "%s: cannot run without jobid", __func__);
        return -1;
    }

    if (z > 0
        && u > 0) {
        ls_syslog(LOG_ERR, "blaunch: -u and -z are mutually exclusive");
        usage();
        return -1;
    }

    hostname = ls_getmyhostname();

    /* Neither -z nor -u were specified on
     * the command line, so the host is the
     * next argv[] params
     */
    if (! host_list)
        get_hosts_from_env();

    if (!host_list)
        get_hosts_from_arg();

    if (!host_list) {
        usage();
        return -1;
    }

    print_hosts();

    make_tasks();

    build_command();
    if (! command) {
        ls_syslog(LOG_ERR, "blaunch: no command to run?");
        usage();
        return -1;
    }

    run_job();

    free_hosts();
    free_command();

    return 0;
}

/* run_job()
 */
static int
run_job(void)
{
    int cc;
    int tid;
    int num_tasks;
    int task_active;

    /* Start all jobs first
     */
    num_tasks = cc = 0;
    while (host_list[cc]) {
        tid = ls_rtask(host_list[cc],
                       command,
                       0);
        if (tid < 0) {
            ls_syslog(LOG_ERR, "\
%s: task %d on host %s failed %s", __func__,
                    tid, host_list[cc], ls_sysmsg());
            return -1;
        }
        tasks[cc] = tid;
        ls_syslog(LOG_INFO, "%s: task %d started", __func__, tid);
        ++cc;
        ++num_tasks;
    }

znovu:
    /* Check if they are still alive
     */
    task_active = cc = 0;
    for (cc = 0; tasks[cc]; cc++) {
        LS_WAIT_T stat;
        struct rusage ru;
        int tid;

        if (tasks[cc] < 0)
            continue;

        tid = ls_rwaittid(tasks[cc], &stat, WNOHANG, &ru);
        if (tid == 0) {
            if (rusage_task(tasks[cc]) < 0) {
                ls_syslog(LOG_ERR, "\
%s: failed to get rusage from task %d %s", __func__, cc, ls_sysmsg());
                tasks[cc] = -1;
                continue;
            }
        }
        if (tid < 0) {
            ls_syslog(LOG_ERR, "\
%s: ls_rwaittid() failed task %d %s", __func__, tid, ls_sysmsg());
            return -1;
        }
        if (tid > 0) {
            ls_syslog(LOG_INFO, "%s: task %d done", __func__, tid);
            tasks[cc] = -1;
        }
    }

    for (cc = 0; tasks[cc]; cc++) {
        if (tasks[cc] > 0) {
            ls_syslog(LOG_INFO, "\
%s: task %d still active", __func__, tasks[cc]);
            ++task_active;
        }
    }

    if (task_active > 0) {
        sleep(rest);
        goto znovu;
    }

    ls_syslog(LOG_INFO, "%s: all %d tasks gone", __func__, num_tasks);

    return 0;
}

static int
redirect_stdin(void)
{
    int cc;

    cc = open("/dev/null", O_RDONLY);
    if (cc < 0)
        return -1;

    dup2(cc, STDIN_FILENO);

    return 0;
}

/* get_host_list()
 *
 * Get host list from -z option
 */
static int
get_host_list(const char *hosts)
{
    char *p;
    char *p0;
    char *h;
    int n;

    p0 = p = strdup(hosts);
    n = 0;
    while (getNextWord_(&p))
        ++n;

    host_list = calloc(n + 1, sizeof(char *));
    _free_(p0);

    n = 0;
    p0 = p = strdup(hosts);
    while ((h = getNextWord_(&p))) {
        host_list[n] = strdup(h);
        ++n;
    }

    _free_(p0);

    return 0;
}

/* get_host_list_from_file()
 *
 * Get host list from the host file
 */
static int
get_host_list_from_file(const char *file)
{
    char s[MAXHOSTNAMELEN];
    FILE *fp;
    int n;

    fp = fopen(file, "r");
    if (fp == NULL) {
        return -1;
    }

    n = count_lines(fp);
    host_list = calloc(n + 1, sizeof(char *));

    rewind(fp);
    n = 0;
    while ((fgets(s, sizeof(s), fp))) {
        s[strlen(s) - 1] = 0;
        host_list[n] = strdup(s);
        ++n;
    }

    return 0;
}

static int
count_lines(FILE *fp)
{
    int n;
    int cc;

    n = 0;
    while ((cc = fgetc(fp)) != EOF) {
        if (cc == '\n')
            ++n;
    }

    return n;
}

/* get_host_from_env()
 *
 * Get host list from the process env
 */
static int
get_hosts_from_env(void)
{
    char *p;

    if (! (p = getenv("LSB_MCPU_HOSTS")))
        return -1;

    get_host_list(p);

    return 0;
}

/* get_hosts_from_args()
 *
 * Get host list from commmand line arguments
 */
static int
get_hosts_from_arg(void)
{
    char *host;
    struct hostent *hp;

    /* We have no -u nor -z so optind
     * points to the first non option
     * parameter which must be a host.
     */

    host = arg_char[optind];
    hp = gethostbyname(host);
    if (hp == NULL) {
        ls_syslog(LOG_ERR, "\
%s: gethostbyname(%s) failed %s", __func__, host, hstrerror(h_errno));
        return -1;
    }

    host_list = calloc(2, sizeof(char *));
    host_list[0] = strdup(host);
    /* increase so we skip over the next argv[] since
     * it is the host we get from the command line on which
     * we are going to run
     */
    ++optind;

    return 0;
}

static void
print_hosts(void)
{
    char *h;
    int cc;

    sprintf(buf, "host(s): ");
    cc = 0;
    while ((h = host_list[cc])) {
        sprintf(buf + strlen(buf), "%s ", h);
        ++cc;
    }

    ls_syslog(LOG_INFO, "%s: host list: %s", __func__, buf);

}

/* make_tasks()
 */
static void
make_tasks(void)
{
    int cc;

    cc = 0;
    while (host_list[cc])
        ++cc;

    tasks = calloc(cc + 1, sizeof(int));
    jrus = calloc(cc + 1, sizeof(struct jRusage *));
}

static void
build_command(void)
{
    int cc;
    int n;

    /* no command to build, optind now points
     * to the first non option argument of blaunch
     */
    if (arg_count - optind <= 0)
        return;

    command = calloc((arg_count - optind) + 1, sizeof(char *));

    n = 0;
    for (cc = optind; cc < arg_count; cc++) {
        command[n] = strdup(arg_char[cc]);
        ++n;
    }

    sprintf(buf, "command: ");
    cc = 0;
    while (command[cc]){
        sprintf(buf + strlen(buf), " %s ", command[cc]);
        ++cc;
    }
    ls_syslog(LOG_INFO, "%s: user command: %s", __func__, buf);

}

static void
free_hosts(void)
{
    int cc;

    if (! host_list)
        return;

    cc = 0;
    while (host_list[cc]) {
        _free_(host_list[cc]);
        ++cc;
    }

    _free_(host_list);
}

static void
free_command(void)
{
    int cc;

    if (! command)
        return;

    cc = 0;
    while (command[cc]) {
        _free_(command[cc]);
        ++cc;
    }

    _free_(command);
}

static int
rusage_task(int tid)
{
    struct jRusage *jru;

    jru = ls_getrusage(tid);
    if (! jru)
        return -1;

    ls_syslog(LOG_DEBUG, "%s: got rusage for task %d", __func__, tid);

    if (send2sbd(jru) < 0) {
        ls_syslog(LOG_ERR, "\
%s: failed to send jRusage data to SBD on %s", __func__, hostname);
        free_rusage(jru);
        return -1;
    }

    free_rusage(jru);

    return 0;
}

/* send2sbd()
 */
static int
send2sbd(struct jRusage *jru)
{
    XDR xdrs;
    int len;
    int len2;
    int cc;
    struct LSFHeader hdr;
    char *req_buf;
    char *reply_buf;

    len = sizeof(LS_LONG_INT) + sizeof(struct LSFHeader);
    len = len + size_rusage(jru);
    len = len * sizeof(int);

    initLSFHeader_(&hdr);
    hdr.opCode = SBD_BLAUNCH_RUSAGE;

    req_buf = calloc(len, sizeof(char));
    xdrmem_create(&xdrs, req_buf, len, XDR_ENCODE);

    XDR_SETPOS(&xdrs, sizeof(struct LSFHeader));

    /* encode the jobID
     */
    if (! xdr_int(&xdrs, &jobID)
        || ! xdr_int(&xdrs, &job_step)) {
        ls_syslog(LOG_ERR, "\
%: failed encoding jobid % or stepid %d", __func__, jobID, job_step);
        _free_(req_buf);
        xdr_destroy(&xdrs);
        return -1;
    }

    /* encode the rusage
     */
    if (! xdr_jRusage(&xdrs, jru, &hdr)) {
        ls_syslog(LOG_ERR, "\
%: failed encoding jobid % or stepid %d", __func__, jobID, job_step);
        _free_(req_buf);
        xdr_destroy(&xdrs);
        return -1;

    }

    len2 = XDR_GETPOS(&xdrs);
    hdr.length =  len2 - sizeof(struct LSFHeader);
    XDR_SETPOS(&xdrs, 0);

    if (!xdr_LSFHeader(&xdrs, &hdr)) {
        ls_syslog(LOG_ERR, "\
%: failed encoding jobid % or stepid %d", __func__, jobID, job_step);
        _free_(req_buf);
        xdr_destroy(&xdrs);
        return -1;
    }

    XDR_SETPOS(&xdrs, len2);

    /* send 2 sbatchd
     */
    reply_buf = NULL;
    cc = cmdCallSBD_(hostname, req_buf, len, &reply_buf, &hdr, NULL);
    if (cc < 0) {
        ls_syslog(LOG_ERR, "\
%s: failed calling SBD on %s %s", __func__, hostname, lsb_sysmsg());
        _free_(req_buf);
        xdr_destroy(&xdrs);
        return -1;
    }

    if (hdr.opCode != LSBE_NO_ERROR) {
        ls_syslog(LOG_ERR, "\
%s: SBD on %s returned %s", __func__, hostname, lsb_sysmsg());
        _free_(req_buf);
        xdr_destroy(&xdrs);
        return -1;
    }

    _free_(req_buf);
    xdr_destroy(&xdrs);

    return 0;
}

/* size_rusage()
 */
static int
size_rusage(struct jRusage *jru)
{
    int cc;

    cc = 0;
    cc = 5 * sizeof(int);
    cc = cc + jru->npids * sizeof(struct pidInfo);
    cc = cc + jru->npgids * sizeof(int *);

    return cc;
}

/* get_job_id()
 */
static int
get_job_id(void)
{
    char *j;
    char *s;

    if (! (j = getenv("LSB_JOBID")))
        return -1;

    jobID = atoi(j);

    if (! (s = getenv("LSB_JOBINDEX_STEP"))) {
        job_step  = 0;
        return 0;
    }

    job_step = atoi(s);

    return 0;
}

static void
free_rusage(struct jRusage *jru)
{
    if (!jru)
        return;

    _free_(jru->pidInfo);
    _free_(jru->pgid);
    _free_(jru);
}
