/* * * * * * * * * * * * * * * * * * * * * * * * *
 * Allan Vitangcol <allan.vitangcol@oracle.com>  *
 *                                               *
 * This program is part of the Oracle DRM project*
 * for use on Oracle OVM and Nimbula environment.*
 *                                               *
 * The program can be used for other purposes    *
 * where it fits.                                *
 *                                               *
 * This is the socket server code that accepts   *
 * connections and relays messages to domU code. *
 * * * * * * * * * * * * * * * * * * * * * * * * */

#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/wait.h>
#include <pthread.h>
#include <signal.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <pwd.h>
#include <fcntl.h>
#include <string.h>
#include <syslog.h>
#include <sys/mount.h>
#include "drm.h"
#ifndef STANDALONE
#ifdef TWO_WAY
#define RELAY_MESSAGE(STRMESSAGE, MESSAGELENGTH) drm_relay(STRMESSAGE, MESSAGELENGTH)>-1 && send(myconn, STRMESSAGE, MESSAGELENGTH, MSG_EOR)>-1
#else
#define RELAY_MESSAGE(STRMESSAGE, MESSAGELENGTH) drm_relay(STRMESSAGE, MESSAGELENGTH)>-1
#endif
#else
#define RELAY_MESSAGE(STRMESSAGE, MESSAGELENGTH) send(myconn, STRMESSAGE, MESSAGELENGTH, MSG_EOR)>-1
#endif

struct sockaddr_un name;
#ifndef STANDALONE
int mounted = 1;
#endif

void cleanupONsignal(sigset_t *arg) {
    sigset_t *signals2catch=arg;
    int caught;
    sigwait(signals2catch, &caught);
#ifndef STANDALONE
    char eot=EOT;
    drm_relay(&eot, 1);
    if(mounted == 0 )
        umount("/proc/xen");
#endif
    unlink(name.sun_path);
    closelog();
    exit(caught);
}

int make_named_socket (const char *const filename) {
    int sock;
    struct stat *sockfile;

    /* Create the socket. */

    sock = socket (PF_UNIX, SOCK_STREAM, 0);
    if (sock < 0) {
        syslog(LOG_ERR, strerror(errno));
        closelog();
        exit (EXIT_FAILURE);
    }

    memset(&name, 0, sizeof(struct sockaddr_un));

    /* Bind a name to the socket. */

    name.sun_family = PF_UNIX;
    strncpy (name.sun_path, filename, sizeof(name.sun_path) - 1);

    sockfile = malloc(sizeof(struct stat));
    if(stat(name.sun_path, sockfile) == 0) {
        if(connect(sock, (struct sockaddr *) &name, sizeof(struct sockaddr_un)) == 0) {
            free(sockfile);
            syslog(LOG_ERR, "Socket %s in use", name.sun_path);
            closelog();
            exit (2);
        }
        unlink(name.sun_path);
    }

    free(sockfile);

    if (bind (sock, (struct sockaddr *) &name, sizeof(struct sockaddr_un)) < 0) {
        syslog(LOG_ERR, strerror(errno));
        closelog();
        exit (EXIT_FAILURE);
    }

    return sock;
}

void process_client(int *newconn) {
    int myconn=*newconn;
    unsigned int i;
    char clntmsg[MSG_BUFLEN+1], *echo=NULL;
    do {
        free(echo);
        echo=malloc(30);
        memset(&clntmsg, 0, sizeof(clntmsg));
        strcpy(echo, "Your message is received: ");
        while(recv(myconn, clntmsg, MSG_BUFLEN, 0) == MSG_BUFLEN) {
            if(*clntmsg) {
                echo=realloc(echo, strlen(echo)+strlen(clntmsg)+1);
                for(i=strlen(echo);i<strlen(echo)+strlen(clntmsg)+1;i++)
                    echo[i]='\0';
                strcat(echo, clntmsg);
            }
            memset(&clntmsg, 0, sizeof(clntmsg));
        }
        echo=realloc(echo, strlen(echo)+strlen(clntmsg)+11);
        for(i=strlen(echo);i<strlen(echo)+strlen(clntmsg)+11;i++)
            echo[i]='\0';
        strcat(echo, clntmsg);
    } while(*clntmsg && RELAY_MESSAGE(echo, strlen(echo)));
    free(echo);
    shutdown(myconn, SHUT_RDWR);
    close(myconn);
    return;
}

char *pidlocation(const uid_t realuid, const char *const progname) {
    char *pidfname;
    if(realuid < 1) {
        pidfname = malloc(snprintf(NULL, 0, "/var/run/%s.pid", strrchr(progname,'/')+1)+1);
        sprintf(pidfname, "/var/run/%s.pid", strrchr(progname,'/')+1);
    } else {
        char *buffer = malloc(2048);
        struct passwd *userptr=malloc(sizeof(struct passwd));
#ifdef SunOS
        if(getpwuid_r(realuid, userptr, buffer, 2048) != NULL) {
#else
        if(!getpwuid_r(realuid, userptr, buffer, 2048, &userptr)) {
#endif
            pidfname = malloc(snprintf(NULL, 0, "%s/.%s.pid", userptr->pw_dir, strrchr(progname,'/')+1)+1);
            sprintf(pidfname, "%s/.%s.pid", userptr->pw_dir, strrchr(progname,'/')+1);
        } else {
            printf("Unable to locate home directory of UID %d\n", (int)realuid);
            pidfname = malloc(snprintf(NULL, 0, "/tmp/.%s.pid", strrchr(progname,'/')+1)+1);
            sprintf(pidfname, "/tmp/.%s.pid", strrchr(progname,'/')+1);
        }
        free(buffer);
        free(userptr);
    }
    return pidfname;
}

int main(int argc, char **argv) {
    pthread_t cleanupThread;
    struct sockaddr peeraddr;
    int mysock, newconn, status;
    unsigned int peeraddrlen=sizeof(peeraddr);
    pthread_t child;
    pthread_attr_t attr;
    pid_t pid;
    uid_t realuid=getuid();
    FILE *pidfile;
    char *pidfname=NULL;
    sigset_t signals2block;
    struct stat script;
    if(argc < 2) {
        printf("Usage: %s {<filename>|stop}\n", argv[0]);
        exit (1);
    }
    pidfname=pidlocation(realuid, argv[0]);
    if(strcmp(argv[1],"stop")==0) {
        if((pidfile=fopen(pidfname,"r"))) {
            fscanf(pidfile, "%d", (int *)&pid);
            fclose(pidfile);
            unlink(pidfname);
            stat(argv[0],&script);
            setgid(script.st_gid);
            setuid(script.st_uid);
            free(pidfname);
            exit(kill(pid,SIGTERM));
        } else {
            printf("Unable to read %s\n", pidfname);
            free(pidfname);
            exit(1);
        }
    }
    if(!stat(pidfname, &script)) {
        printf("PID file %s exists\n", pidfname);
        free(pidfname);
        exit(1);
    }
    free(pidfname);
    stat(argv[0],&script);
    openlog(strrchr(argv[0],'/')+1, LOG_CONS | LOG_PID, LOG_USER);
    if(pthread_attr_init(&attr) || pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
        syslog(LOG_ERR, "Failed to initialize thread attributes. %s", strerror(errno));
        closelog();
        exit (EXIT_FAILURE);
    }
#ifndef STANDALONE
    if(!is_mounted("/proc/xen"))
        if((mounted = mount("xenfs", "/proc/xen", "xenfs", 0, NULL)) != 0) {
            syslog(LOG_ERR, "Failed to mount /proc/xen. %s", strerror(errno));
            printf("Failed to mount /proc/xen.\n");
            exit(1);
        }
#endif
    chdir("/");
    if(!(pid=fork())) {
        setsid();
        if(!(pid=fork())) {
            sigemptyset(&signals2block);
            sigaddset(&signals2block, SIGINT);
            sigaddset(&signals2block, SIGQUIT);
            sigaddset(&signals2block, SIGTERM);
            sigaddset(&signals2block, SIGUSR1);
            sigaddset(&signals2block, SIGUSR2);
            pthread_sigmask(SIG_BLOCK, &signals2block, NULL);
            pthread_create(&cleanupThread, &attr, (void *)&cleanupONsignal, (void *)&signals2block);
            setgid(script.st_gid);
            setuid(script.st_uid);
            fclose(stdout);
            fclose(stderr);
            status=open("/dev/null", O_RDONLY);
            dup2(status, fileno(stdin));
            close(status);
            umask(0);
            mysock = make_named_socket(argv[1]);
            listen(mysock, 10);
            while((newconn = accept(mysock, &peeraddr, (socklen_t *)&peeraddrlen))) {
                if(newconn > 0) {
                    if(pthread_create(&child, &attr, (void *)&process_client, (void *)&newconn))
                        syslog(LOG_CRIT, "Unable to create thread. %s", strerror(errno));
                }
            }
        } else {
            pidfname=pidlocation(realuid, argv[0]);
            if((pidfile = fopen(pidfname,"w"))) {
                fprintf(pidfile, "%d", (int)pid);
                fclose(pidfile);
                chown(pidfname, script.st_uid, script.st_gid);
            } else
                printf("Unable to create %s\n", pidfname);
            free(pidfname);
            waitpid(pid, &status, WNOHANG);
            exit(status);
        }
    } else {
        waitpid(pid, &status, 0);
        exit(status);
    }
    exit(0);
}
