#include <stdio.h>
#include <mntent.h>
#include <string.h>
#ifndef MTAB
#ifdef Linux
#define MTAB "/proc/self/mounts"
#else
#ifdef SunOS
#define MTAB "/etc/mnttab"
#else
#define MTAB "/etc/mtab"
#endif
#endif
#endif
int is_mounted (char *mount_path) {
    FILE *mtab = NULL;
    struct mntent *part = NULL;
    int is_mounted = 0;
    if ((mtab = setmntent(MTAB, "r")) != NULL) {
        while ((part = getmntent(mtab)) != NULL) {
            if ((part->mnt_dir != NULL) && (strcmp(part->mnt_dir, mount_path)) == 0) {
                is_mounted = 1;
            }
        }
        endmntent(mtab);
    }
    return is_mounted;
}
