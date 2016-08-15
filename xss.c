/*
 * Copyright (C) 2010 Zhigang Wang <w1z2g3@gmail.com>
 * 
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 * 02110-1301, USA.
 *
 * Modified by Allan Vitangcol to support both xen 3 and xen 4
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "drm.h"
#include "xss.h"

struct xs_sock *xss_open(char *addr)
{
    struct xs_sock *sock;
    char *domid;
    char buffer[ENTRYSIZE], evtchn[ENTRYSIZE];
    unsigned int count;
    struct xs_permissions perms[1];
    xs_transaction_t th;

    sock = malloc(sizeof(struct xs_sock));
    if (sock == NULL)
        return NULL;

    sock->xsh = XS_OPEN(0);
    if (sock->xsh == NULL) {
        free(sock);
        return NULL;
    }

    th = xs_transaction_start(sock->xsh);
    domid = xs_read(sock->xsh, th, "domid", &count);
    sock->domid = strtol(domid,NULL,10);
    xs_transaction_end(sock->xsh, th, false);
    free(domid);

    memset(sock->addr, 0, ENTRYSIZE);
    strncpy(sock->addr, addr, ENTRYSIZE);

    memset(buffer, 0, ENTRYSIZE);
    memset(evtchn, 0, ENTRYSIZE);
    snprintf(buffer, ENTRYSIZE, "%s/buffer", sock->addr);
    snprintf(evtchn, ENTRYSIZE, "%s/evtchn", sock->addr);
    perms[0].id = sock->domid;
    perms[0].perms = XS_PERM_READ|XS_PERM_WRITE;

    th = xs_transaction_start(sock->xsh);
    if (!xs_write(sock->xsh, th, buffer, "", 0)) goto error;
    if (!xs_write(sock->xsh, th, evtchn, "", 0)) goto error;
    if (!xs_set_permissions(sock->xsh, th, evtchn, perms, 1)) goto error;
    if (!xs_set_permissions(sock->xsh, th, buffer, perms, 1)) goto error;
    if (!xs_watch(sock->xsh, evtchn, "evtchn")) goto error;
    xs_transaction_end(sock->xsh, th, false);

    return sock;

error:

    XS_CLOSE(sock->xsh);
    free(sock);
    return NULL;
}

int xss_close(struct xs_sock *sock)
{
    char addr[ENTRYSIZE], evtchn[ENTRYSIZE];
    xs_transaction_t th;

    memset(addr, 0, ENTRYSIZE);
    memset(evtchn, 0, ENTRYSIZE);
    snprintf(addr, ENTRYSIZE, "%s", sock->addr);
    snprintf(evtchn, ENTRYSIZE, "%s/evtchn", sock->addr);

    /* FIXME: unwatch is not working in domU. */
    th = xs_transaction_start(sock->xsh);
    xs_unwatch(sock->xsh, evtchn, "evtchn");
    xs_rm(sock->xsh, th, addr);
    xs_transaction_end(sock->xsh, th, false);
    XS_CLOSE(sock->xsh);

    free(sock);

    return 0;
}

ssize_t xss_sendto(struct xs_sock *sock, void *buf, size_t len, struct xs_sock *dest_sock)
{
    char *value = NULL, *dompath;
    char **vec = NULL;
    xs_transaction_t th;
    unsigned int wcount = 0, rcount = 0, num;
    char uri[ENTRYSIZE], evtchn[ENTRYSIZE], buffer[ENTRYSIZE];
    char dest_uri[ENTRYSIZE], dest_evtchn[ENTRYSIZE], dest_buffer[ENTRYSIZE];

    memset(uri, 0, ENTRYSIZE);
    memset(evtchn, 0, ENTRYSIZE);
    memset(buffer, 0, ENTRYSIZE);
    memset(dest_uri, 0, ENTRYSIZE);
    memset(dest_evtchn, 0, ENTRYSIZE);
    memset(dest_buffer, 0, ENTRYSIZE);
    snprintf(uri, ENTRYSIZE, "%d:%s", sock->domid, sock->addr);
    snprintf(evtchn, ENTRYSIZE, "%s/evtchn", sock->addr);
    snprintf(buffer, ENTRYSIZE, "%s/buffer", sock->addr);
    snprintf(dest_uri, ENTRYSIZE, "%d:%s", dest_sock->domid, dest_sock->addr);
    th = xs_transaction_start(sock->xsh);
    dompath = xs_get_domain_path(sock->xsh, dest_sock->domid);
    xs_transaction_end(sock->xsh, th, false);
    snprintf(dest_evtchn, ENTRYSIZE, "%s/%s/evtchn", dompath, dest_sock->addr);
    snprintf(dest_buffer, ENTRYSIZE, "%s/%s/buffer", dompath, dest_sock->addr);
    free(dompath);

    wcount = len > ENTRYSIZE ? ENTRYSIZE : len;
    th = xs_transaction_start(sock->xsh);
    if (!xs_write(sock->xsh, th, dest_buffer, buf, wcount)) goto error;
    if (!xs_write(sock->xsh, th, dest_evtchn, uri, strlen(uri))) goto error;
    xs_transaction_end(sock->xsh, th, false);

    dprintf("dest buffer and evtchn: %s:%s:\n", dest_buffer, dest_evtchn);

    while (((char *)buf)[0] != EOT && rcount == 0) {
        vec = xs_read_watch(sock->xsh, &num);
        if (vec == NULL) {
            dprintf("xs_read_watch failed.\n");
            return -1;
        }
        dprintf("watch triggerred: %s:%s:%d\n", vec[XS_WATCH_PATH], vec[XS_WATCH_TOKEN], num);
        th = xs_transaction_start(sock->xsh);
        value = xs_read(sock->xsh, th, evtchn, &rcount);
        xs_transaction_end(sock->xsh, th, false);
    }

    if (strncmp(value, dest_uri, strlen(dest_uri)) != 0) {
        dprintf("Invalid address: %s\n", value);
        goto error;
    }

    free(value);
    free(vec);

    return wcount;

error:

    dprintf("Error occured!\n");
    if (value)
        free(value);
    if (vec)
        free(vec);

    return -1;
}

ssize_t xss_recvfrom(struct xs_sock *sock, void *buf, size_t len, struct xs_sock *src_sock)
{
    char *value = NULL;
    void *data;
    char **vec = NULL;
    char *delim = ":", *token, *saveptr, *dompath;
    int src_domid;
    xs_transaction_t th;
    char src_addr[ENTRYSIZE];
    unsigned int count = 0, num;
    char uri[ENTRYSIZE], evtchn[ENTRYSIZE], buffer[ENTRYSIZE], src_evtchn[ENTRYSIZE];

    memset(uri, 0, ENTRYSIZE);
    memset(evtchn, 0, ENTRYSIZE);
    memset(buffer, 0, ENTRYSIZE);
    snprintf(uri, ENTRYSIZE, "%d:%s", sock->domid, sock->addr);
    snprintf(evtchn, ENTRYSIZE, "%s/evtchn", sock->addr);
    snprintf(buffer, ENTRYSIZE, "%s/buffer", sock->addr);

    while (count == 0) {
        vec = xs_read_watch(sock->xsh, &num);
        if (vec == NULL) {
            dprintf("xs_read_watch failed.\n");
            return -1;
        }
        dprintf("watch triggerred: %s:%s:%d\n", vec[XS_WATCH_PATH], vec[XS_WATCH_TOKEN], num);
        th = xs_transaction_start(sock->xsh);
        value = xs_read(sock->xsh, th, evtchn, &count);
        xs_transaction_end(sock->xsh, th, false);
    }

    /* parse domid and addr */
    if(value != NULL)
        token = strtok_r(value, delim, &saveptr);
    else
        token = NULL;
    if (token == NULL) {
        dprintf("Invalid path\n");
        goto error;
    }
    src_domid = strtol(token,NULL,10);
    token = strtok_r(NULL, delim, &saveptr);
    if (token == NULL) {
        dprintf("Invalid path\n");
        goto error;
    }
    memset(src_addr, 0, ENTRYSIZE);
    strcpy(src_addr, token);
    if (src_sock != NULL) {
        src_sock->domid = src_domid;
        memset(src_sock->addr, 0, ENTRYSIZE);
        strcpy(src_sock->addr, src_addr);
    }
    memset(src_evtchn, 0, ENTRYSIZE);
    th = xs_transaction_start(sock->xsh);
    dompath = xs_get_domain_path(sock->xsh, src_domid);
    xs_transaction_end(sock->xsh, th, false);
    snprintf(src_evtchn, ENTRYSIZE, "%s/%s/evtchn", dompath, src_addr);
    free(dompath);

    th = xs_transaction_start(sock->xsh);
    data = xs_read(sock->xsh, th, buffer, &count);
    xs_transaction_end(sock->xsh, th, false);
    if (data == NULL) {
        dprintf("xs_read failed.\n");
        goto error;
    }
    memcpy(buf, data, count);
    th = xs_transaction_start(sock->xsh);
    if(((char *)buf)[0] != EOT)
        if (!xs_write(sock->xsh, th, src_evtchn, uri, strlen(uri))) {
            xs_transaction_end(sock->xsh, th, false);
            dprintf("write error\n");
            free(data);
            goto error;
        }
    xs_transaction_end(sock->xsh, th, false);

    free(data);
    free(value);
    free(vec);

    return count;

error:

    free(value);
    free(vec);

    return -1;
}
