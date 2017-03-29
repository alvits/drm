#ifndef _XSS_H
#define _XSS_H

#if XENVERSION > 3
#include <xenstore.h>
#define XS_OPEN(param) xs_open(param)
#define XS_CLOSE(param) xs_close(param)
#else
#include <xs.h>
#define XS_OPEN(param) xs_domain_open()
#define XS_CLOSE(param) xs_daemon_close(param)
#endif

#include <unistd.h>
#include <xen/xen.h>

#define ENTRYSIZE MAX_GUEST_CMDLINE

struct xs_sock {
    struct xs_handle *xsh;
    unsigned int domid;
    char addr[ENTRYSIZE];
};

struct xs_sock *xss_open(char *addr);

int xss_close(struct xs_sock *sock);

ssize_t xss_sendto(struct xs_sock *sock, void *buf, size_t len, struct xs_sock *dest_sock);

ssize_t xss_recvfrom(struct xs_sock *sock, void *buf, size_t len, struct xs_sock *src_sock);

#endif /* _XSS_H */
