
#include <lsf.h>
#include <stdio.h>

int
main(int argc, char **argv)
{
    char *masterHost;

    masterHost = ls_getmastername();
    if (masterHost == NULL) {
	printf("MasterHost is unknown\n");
	return -1;
    }

    printf("MasterHost is: %s\n", masterHost);
    return 0;
}
