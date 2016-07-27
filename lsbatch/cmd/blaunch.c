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
#include <unistd.h>

static void
usage(void)
{
    fprintf(stderr, "blaunch: [-h] [-V] [-n] [-z hosts...]\n");
}

static int arg_count;
static char **arg_char;
static char **host_list;
static char **command;
static int verbose;
extern char **environ;
static int redirect_stdin(void);
static int get_host_list(const char *);
static int get_host_list_from_file(const char *);
static int count_lines(FILE *);
static int get_hosts_from_env(void);
static int get_hosts_from_arg(void);
static void print_hosts(void);
static void build_command(void);
static int run_job(void);
static void free_hosts(void);
static void free_command(void);

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

    u = z = 0;
    while ((cc = getopt(argc, argv, "hVvnz:u:")) != EOF) {
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
            case 'h':
            case '?':
                usage();
                return(-1);
        };
    }

    if (z > 0
        && u > 0) {
        fprintf(stderr, "-u and -z are mutually exclusive\n");
        usage();
        return -1;
    }

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
    build_command();
    if (! command) {
        fprintf(stderr, "blaunch: no command to run?\n");
        usage();
        return -1;
    }

    run_job();

    free_hosts();
    free_command();

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
        fprintf(stderr, "\
%s: gethostbyname(%s) failed %s\n", __func__, host, hstrerror(h_errno));
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

    printf("host(s): ");
    cc = 0;
    while ((h = host_list[cc])) {
        printf("%s ", h);
        ++cc;
    }
    printf("\n");
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

    printf("command: ");
    cc = 0;
    while (command[cc]){
        printf(" %s ", command[cc]);
        ++cc;
    }
    printf("\n");
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

/* run_job()
 */
static int
run_job(void)
{
    int cc;
    int  tid;
    int num;

    Signal_(SIGUSR1, SIG_IGN);
    /* initialize the remote execution
     * library
     */
    cc = ls_initrex(1, 0);
    if (cc < 0) {
        ls_perror("ls_initrex()");
        return -1;
    }

    cc = 0;
    while (host_list[cc]) {
        tid = ls_rtask(host_list[cc],
                       command,
                       0);
        if (tid < 0) {
            fprintf(stderr, "\
%s: task %d on host %s failed %s", __func__,
                    tid, host_list[cc], ls_sysmsg());
            return -1;
        }
        ++cc;
    }

    num = cc;
    while (num) {
        LS_WAIT_T stat;
        struct rusage ru;
        int tid;

        tid = ls_rwait(&stat, 0, &ru);
        if (tid < 0) {
            ls_perror("ls_rwait()");
            break;
        }
        --num;
    }
    return 0;
}