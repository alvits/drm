/* * * * * * * * * * * * * * * * * * * * * * * * *
 * Allan Vitangcol <allan.vitangcol@oracle.com>  *
 *                                               *
 * This program is part of the Oracle DRM project*
 * for use on Oracle OVM and Nimbula environment.*
 *                                               *
 * The program can be used for other purposes    *
 * where it fits.                                *
 *                                               *
 * This is the client code of the Policy Engine. *
 * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <string.h>
#include <syslog.h>
#include "drm.h"

int make_inet_socket (const char *ip_addr, const char *port) {
    struct addrinfo hints;
    struct addrinfo *result, *rp;
    int sock;

    /* Obtain address(es) matching host/port */

    memset(&hints, 0, sizeof(struct addrinfo));
    hints.ai_family = AF_UNSPEC;    /* Allow IPv4 or IPv6 */
    hints.ai_socktype = SOCK_STREAM; /* Stream socket */
    hints.ai_flags = 0;
    hints.ai_protocol = 0;          /* Any protocol */

    if((sock=getaddrinfo(ip_addr, port, &hints, &result)) != 0) {
        syslog(LOG_CRIT, "%s:%s. %s", ip_addr, port, gai_strerror(sock));
        return -1;
    }

    rp = result;

    sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    while(rp != NULL && (sock < 0 || connect(sock, rp->ai_addr, rp->ai_addrlen) < 0)) {
        close(sock);
        if((rp = rp->ai_next) != NULL)
            sock = socket(rp->ai_family, rp->ai_socktype, rp->ai_protocol);
    }

    freeaddrinfo(result);

    if(rp == NULL) {
        syslog(LOG_CRIT, "Could not connect to policy engine %s on port %s", ip_addr, port);
        return -1;
    }

    return sock;

}
