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

#include "lib.h"
#include "lproto.h"
#include <unistd.h>
#include <fcntl.h>

extern int totsockets_;
extern int currentsocket_;

int
CreateSock_(int protocol)
{
    struct sockaddr_in cliaddr;
    int s;
    int cc;

    if ((s = Socket_(AF_INET, protocol, 0)) < 0) {
        lserrno = LSE_SOCK_SYS;
        return -1;
    }

    memset(&cliaddr, 0, sizeof(cliaddr));
    cliaddr.sin_family  = AF_INET;
    cliaddr.sin_addr.s_addr = htonl(INADDR_ANY);
    cc = bind(s, (struct sockaddr *)&cliaddr, sizeof(cliaddr));
    if (cc < 0) {
        close(s);
        return -1;
    }

    fcntl(s, F_SETFD, (fcntl(s, F_GETFD) | FD_CLOEXEC));

    return s;
}

int
CreateSockEauth_(int protocol)
{
    return CreateSock_(protocol);
}

int
get_nonstd_desc_(int desc)
{
    int s0 = -1;
    int s1 = -1;
    int s2 = -1;

    while (desc <= 2) {
        switch (desc) {
        case 0:
            s0 = desc;
            break;
        case 1:
            s1 = desc;
            break;
        case 2:
            s2 = desc;
            break;
        default:
            return -1;
        }

        desc = dup(desc);
    }

    if (s0 >= 0)
        close(s0);
    if (s1 >= 0)
        close(s1);
    if (s2 >= 0)
        close(s2);

    return desc;
}


int
TcpCreate_(int service, int port)
{
    register int s;
    struct sockaddr_in sin;

    if ((s = Socket_(AF_INET, SOCK_STREAM, 0)) < 0) {
        lserrno = LSE_SOCK_SYS;
        return -1;
    }

    if (service) {

	memset(&sin, 0, sizeof(sin));
        sin.sin_family      = AF_INET;
        sin.sin_port        = htons(port);
        sin.sin_addr.s_addr = htonl(INADDR_ANY);

        if (bind(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
            close(s);
            lserrno = LSE_SOCK_SYS;
            return -2;
        }
        if (listen(s, 1024) < 0) {
            close(s);
            lserrno = LSE_SOCK_SYS;
            return -3;
        }
    }

    return s;
}

int
io_nonblock_(int s)
{
    return (fcntl(s, F_SETFL, O_NONBLOCK));
}

int
io_block_(int s)
{
    return (fcntl(s, F_SETFL, fcntl(s, F_GETFL) & ~O_NONBLOCK));
}


int
Socket_(int domain, int type, int protocol)
{
    int s0, s1;

    if ((s0 = socket(domain, type, protocol)) < 0)
        return -1;

    if (s0 < 0)
        return -1;

    if (s0 >= 3)
        return s0;

    s1 = get_nonstd_desc_(s0);
    if (s1 < 0)
        close(s0);

    return s1;
}

int
TcpConnect_(char *hostname, u_short port, struct timeval *timeout)
{
    int sock;
    int nwRdy, i;
    struct sockaddr_in server;
    struct hostent *hp;
    fd_set wm;

    server.sin_family = AF_INET;
    if ((hp = Gethostbyname_(hostname)) == NULL) {
        lserrno = LSE_BAD_HOST;
        return -1;
    }

    memcpy((char *) &server.sin_addr,
           (char *) hp->h_addr,
           (int) hp->h_length);

    server.sin_port = htons(port);

    if ((sock = socket(AF_INET, SOCK_STREAM, 0))<0) {
	lserrno = LSE_SOCK_SYS;
        return -1;
    }
    if (io_nonblock_(sock) < 0) {
	lserrno = LSE_MISC_SYS;
        closesocket(sock);
        return -1;
    }

    if (connect(sock, (struct sockaddr *) &server, sizeof(server)) < 0
        && errno != EINPROGRESS) {
	lserrno = LSE_CONN_SYS;
        closesocket(sock);
        return -1;
    }

    for (i = 0; i < 2; i++) {
        FD_ZERO(&wm);
        FD_SET(sock, &wm);
        nwRdy = select(sock+1, NULL, &wm, NULL, timeout);

        if (nwRdy < 0) {
            if (errno == EINTR)
                continue;
            lserrno = LSE_SELECT_SYS;
            closesocket(sock);
            return -1;
        } else if (nwRdy == 0) {
            lserrno = LSE_TIME_OUT;
            closesocket(sock);
            return -1;
        }
        break;
    }

    return sock;
}
