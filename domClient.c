/* * * * * * * * * * * * * * * * * * * * * * * * *
 * Allan Vitangcol <allan.vitangcol@oracle.com>  *
 *                                               *
 * This program is part of the Oracle DRM project*
 * for use on Oracle OVM and Nimbula environment.*
 *                                               *
 * The program can be used for other purposes    *
 * where it fits.                                *
 *                                               *
 * This is the client code for DomUs to allow    *
 * sending messages to Dom0.                     *
 *                                               *
 * The program uses open source XenstoreSocket   *
 * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <stdlib.h>
#include <pthread.h>
#include <string.h>
#include <syslog.h>
#include "drm.h"
#include "xss.h"

static pthread_mutex_t drm_mutex = PTHREAD_MUTEX_INITIALIZER;

ssize_t drm_relay(void *buffer, unsigned int length) {
    ssize_t retval=-1;
    struct xs_handle *xshandle;
    struct xs_sock *xss_listener, xss_dest;
    char xss_instance[ENTRYSIZE], *chardomid;
    if(pthread_mutex_lock(&drm_mutex) == 0) {
        if((xshandle = XS_OPEN(0)) == NULL) {
            syslog(LOG_CRIT, "Failed to open xen domain. %s", strerror(errno));
            return -1;
        }
        memset(xss_dest.addr,0,ENTRYSIZE);
        chardomid=xs_read(xshandle, XBT_NULL, "domid", &xss_dest.domid);
        XS_CLOSE(xshandle);
        sprintf(xss_dest.addr, "xss/%s", chardomid);
        xss_dest.domid=0;
        xss_dest.xsh=NULL;
        memset(xss_instance,0,ENTRYSIZE);
        sprintf(xss_instance, "xss/%u", xss_dest.domid);
        dprintf("Opening listener on %s for domain %s\n", xss_instance, chardomid);
        if((xss_listener=xss_open(xss_instance)) != NULL) {
    	    dprintf("Successful opening listener on %s for domain %s\n", xss_instance, chardomid);
            retval=xss_sendto(xss_listener,buffer,strlen(buffer),&xss_dest);
#ifdef TWO_WAY
            if(((char *)buffer)[0] != EOT)
                xss_recvfrom(xss_listener,buffer,length+10,&xss_dest);
#endif
            if(xss_dest.xsh)
            free(xss_dest.xsh);
            xss_close(xss_listener);
        } else
    	    dprintf("Failed opening listener on %s for domain %s\n", xss_instance, chardomid);
        free(chardomid);
        pthread_mutex_unlock(&drm_mutex);
    }
    return retval;
}
