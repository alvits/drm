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

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <syslog.h>
#include <sys/mount.h>
#include "drm.h"

int main(int argc, char **argv) {
    int mounted = 1;
    if(!is_mounted("/proc/xen"))
        if((mounted = mount("xenfs", "/proc/xen", "xenfs", 0, NULL)) != 0) {
            printf("Failed to mount /proc/xen.\n");
            exit(1);
        }
    drm_relay((void *)argv[1],strlen(argv[1]));
    if(mounted == 0 )
        umount("/proc/xen");
    exit(0);
}
