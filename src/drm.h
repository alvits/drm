#ifndef _DRM_H
#define _DRM_H

#define EOT 0x4
#define MSG_BUFLEN 2048

#ifndef MSG_EOR
#define MSG_EOR 0x80
#endif

#ifdef DEBUG_DRM
#define dprintf(fmt, ...) \
    printf("DRM: " fmt, ## __VA_ARGS__)
#else
#define dprintf(fmt, ...) \
    (void) 0
#endif

ssize_t drm_relay(void *buffer, unsigned int length);
int make_inet_socket (const char *ip_addr, const char *port);
int is_mounted (char * mount_path);

#endif	/* _DRM_H */
