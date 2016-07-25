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

static void
usage(void)
{
    fprintf(stderr, "blaunch: [-h] [-V] [-n] [-z hosts...]\n");
}

static int arg_count;
static char **arg_char;
static char **host_list;

static int redirect_stdin(void);
static int get_host_list(const char *);
static int get_host_list_from_file(const char *);
static int count_lines(FILE *);
static int get_hosts_from_env(void);
static int get_hosts_from_arg(void);
static void print_hosts(void);
static void print_args(void);
static void free_hosts(void);

int
main(int argc, char **argv)
{
    int cc;
    char z;
    char u;

    arg_count = argc;
    arg_char = argv;

    if (argc <= 1) {
        usage();
        return -1;
    }

    u = z = 0;
    while ((cc = getopt(argc, argv, "hVnz:u:")) != EOF) {
        switch (cc) {
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

    print_args();

    free_hosts();

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

/* get_host_from_args()
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
    int cc;
    char *h;

    cc = 0;
    while ((h = host_list[cc])) {
        printf("%s\n", h);
        ++cc;
    }
}

static void
print_args(void)
{
    int cc;

    for (cc = optind; cc < arg_count; cc++)
        printf(" %s \n", arg_char[cc]);
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
