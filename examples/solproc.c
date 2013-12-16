

#include <unistd.h>
#include <dirent.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/param.h>
#include <sys/procfs.h>
#include <sys/stat.h>

int
main(int argc, char **argv)
{
    DIR *dir;
    struct dirent *dr;
    struct prpsinfo info;
    struct prstatus status;
    char buf[128];

    dir = opendir("/proc");

    while ((dr = readdir(dir))) {
        int cc;
        int d;

        if (strcmp(dr->d_name, ".") == 0
            || strcmp(dr->d_name, "..") == 0)
            continue;

        sprintf(buf, "/proc/%s", dr->d_name);

        d = open(buf, O_RDONLY);
        if (d < 0)
            continue;

        cc = ioctl(d, PIOCPSINFO, &info);
        if (cc < 0)
            continue;

        cc = ioctl(d, PIOCSTATUS, &status);
        if (cc < 0)
            continue;

        printf("\
%d %d %d %d %d %d %d %d %d %d %d %d\n",
               info.pr_pid, info.pr_ppid, info.pr_gid, 0,
               status.pr_utime.tv_sec, status.pr_stime.tv_sec,
               status.pr_cutime.tv_sec, status.pr_cstime.tv_sec,
               info.pr_bysize, info.pr_byrssize, status.pr_stksize,
               info.pr_state);

        close(d);

    } /* while() */

    closedir(dir);

    return 0;
}
