/* * * * * * * * * * * * * * * * * * * * * * * * *
 * Allan Vitangcol <allan.vitangcol@oracle.com>  *
 *                                               *
 * This program is part of the Oracle DRM project*
 * for use on Oracle OVM and Nimbula environment.*
 *                                               *
 * The program can be used for other purposes    *
 * where it fits.                                *
 *                                               *
 * This code receives the messages from DomUs.   *
 *                                               *
 * The program uses open source XenstoreSocket.  *
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
#include "drm.h"
#include "xss.h"

struct xs_handle *xshandle;
char **xs_path;

struct threadChild {
    pthread_t child;
    unsigned int domid;
    struct threadChild *next;
};

void cleanupONsignal(void) {
    sigset_t signals2catch;
    int caught;
    sigemptyset(&signals2catch);
    sigaddset(&signals2catch, SIGINT);
    sigaddset(&signals2catch, SIGQUIT);
    sigaddset(&signals2catch, SIGTERM);
    sigaddset(&signals2catch, SIGUSR1);
    sigaddset(&signals2catch, SIGUSR2);
    sigwait(&signals2catch, &caught);
    xs_rm(xshandle, XBT_NULL, "/local/domain/0/xss");
    free(xshandle);
    if(xs_path != NULL)
        free(xs_path);
    closelog();
    exit(caught);
}

bool newListener(const struct threadChild *listenerThreads, const unsigned int domid) {
    bool retval=true;
    while(retval && listenerThreads) {
        retval=(listenerThreads->domid != domid);
        listenerThreads=listenerThreads->next;
    }
    return retval;
}

bool notFound(const unsigned int domid, const char **const xs_path, const unsigned int numindex) {
    unsigned int counter;
    bool retval=true;
    for(counter=0; counter<numindex && retval; counter++)
        retval=(domid != strtol(xs_path[counter],NULL,10));
    return retval;
}

struct threadChild *pushThread(struct threadChild *const listenerThreads, const unsigned int domid) {
    struct threadChild *newThread;
    newThread=(struct threadChild *)malloc(sizeof(struct threadChild));
    newThread->domid=domid;
    newThread->next=listenerThreads;
    return newThread;
}

void shutdownListener(const struct threadChild *const listenerThread) {
    struct xs_handle *xsh;
    xs_transaction_t th;
    char *xss_path, eot=EOT;
    unsigned int len;
    xsh=XS_OPEN(0);
    len=snprintf(NULL, 0, "xss/%d/buffer", listenerThread->domid)+1;
    xss_path=malloc(len);
    memset(xss_path,0,len);
    sprintf(xss_path, "xss/%d/buffer", listenerThread->domid);
    th = xs_transaction_start(xsh);
    xs_write(xsh, th, xss_path, &eot, 1);
    memset(xss_path,0,len);
    sprintf(xss_path, "xss/%d/evtchn", listenerThread->domid);
    xs_write(xsh, th, xss_path, "0:xss/0", 7);
    xs_transaction_end(xsh, th, false);
    XS_CLOSE(xsh);
    free(xss_path);
}

struct threadChild *popThreads(struct threadChild *const listenerThreads, const char **const xs_path, const unsigned int numindex) {
    struct threadChild *retval;
    if(listenerThreads) {
        if(pthread_kill(listenerThreads->child, 0)) {
            retval=listenerThreads->next;
            free(listenerThreads);
        } else
            if(notFound(listenerThreads->domid, xs_path, numindex)) {
                shutdownListener(listenerThreads);
                retval=listenerThreads->next;
                free(listenerThreads);
            } else
                retval=listenerThreads;
        if(retval)
            retval->next=popThreads(retval->next, xs_path, numindex);
        return retval;
    } else
        return NULL;
}

void create_listener(const unsigned int *const domid) {
    const unsigned int const domainID=*domid;
    struct xs_sock *xss_listener, *xss_source;
    char *xss_instance, buffer[ENTRYSIZE];
    unsigned int len;
    xss_source=malloc(sizeof(struct xs_sock));
    memset(xss_source->addr,0,ENTRYSIZE);
    sprintf(xss_source->addr, "xss/%u", 0);
    xss_source->domid=domainID;
    xss_source->xsh=NULL;
    len=snprintf(NULL, 0, "xss/%u", domainID)+1;
    xss_instance=malloc(len);
    memset(xss_instance,0,len);
    sprintf(xss_instance, "xss/%u", domainID);
    if((xss_listener=xss_open(xss_instance)) != NULL) {
        memset(buffer,0,ENTRYSIZE);
        while(xss_recvfrom(xss_listener,buffer,ENTRYSIZE,xss_source) > 0 && *buffer != EOT) {
#ifdef TWO_WAY
            char *repbuffer;
            repbuffer=malloc(strlen(buffer)+10);
            memset(repbuffer,0,strlen(buffer)+10);
            sprintf(repbuffer, "Domain0: %s", buffer);
            xss_sendto(xss_listener,repbuffer,strlen(repbuffer),xss_source);
            free(repbuffer);
#endif
            dprintf("%s\n", buffer);
            memset(buffer,0,ENTRYSIZE);
        }
        xss_close(xss_listener);
    }
    free(xss_instance);
    if(xss_source->xsh)
    free(xss_source->xsh);
    free(xss_source);
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
    struct threadChild *listenerThreads=NULL;
    pthread_attr_t attr;
    uid_t realuid=getuid();
    FILE *pidfile;
    char *pidfname=NULL;
    sigset_t signals2block;
    struct stat script;
    xs_transaction_t th;
    struct xs_permissions perms[1];
    unsigned int domid, numindex, counter;
    char *dompath, *token, *xss_path;
    pid_t pid;
    int status;
    if(argc < 2) {
        printf("Usage: %s {start|stop}\n", argv[0]);
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
    if(strcmp(argv[1],"start")!=0) {
        printf("Usage: %s {start|stop}\n", argv[0]);
        free(pidfname);
        exit (1);
    }
    if(!stat(pidfname, &script)) {
        printf("PID file %s exists\n", pidfname);
        free(pidfname);
        exit(1);
    }
    free(pidfname);
    stat(argv[0],&script);
    perms[0].perms = XS_PERM_READ|XS_PERM_WRITE;
    openlog(strrchr(argv[0],'/')+1, LOG_CONS | LOG_PID, LOG_USER);
    if(pthread_attr_init(&attr) || pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED)) {
        syslog(LOG_ERR, "Failed to set thread attributes. %s", strerror(errno));
        closelog();
        exit (EXIT_FAILURE);
    }
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
            pthread_create(&cleanupThread, &attr, (void *)&cleanupONsignal, NULL);
#ifndef DEBUG_DRM
            fclose(stdout);
            fclose(stderr);
            status=open("/dev/null", O_RDONLY);
            dup2(status, 0);
            close(status);
#endif
            if((xshandle = XS_OPEN(0)) == NULL) {
                syslog(LOG_ERR, "Cannot open xen domain. %s", strerror(errno));
                closelog();
                exit (EXIT_FAILURE);
            }
            th = xs_transaction_start(xshandle);
            dompath = xs_read(xshandle, th, "domid", &domid);
            domid = strtol(dompath,NULL,10);
            free(dompath);
            dompath = xs_get_domain_path(xshandle,domid);
            if((token = strrchr(dompath, '/')))
                *token='\0';
            xs_transaction_end(xshandle, th, false);
            while (true) {
                th = xs_transaction_start(xshandle);
                free(xs_path); xs_path=NULL;
                xs_path = xs_directory(xshandle, th, dompath, &numindex);
                xs_transaction_end(xshandle, th, false);
                for(counter=0; counter < numindex; counter ++) {
                    domid = strtol(xs_path[counter],NULL,10);
                    if(domid > 0)
                        if(newListener(listenerThreads, domid)) {
                            listenerThreads=pushThread(listenerThreads, domid);
                            th = xs_transaction_start(xshandle);
                            int len = snprintf(NULL, 0,"%s/%u/xss",dompath,domid)+1;
                            xss_path=malloc(len);
                            memset(xss_path,0,len);
                            sprintf(xss_path,"%s/%u/xss",dompath,domid);
                            perms[0].id = domid;
                            if(xs_mkdir(xshandle,th,xss_path))
                                xs_set_permissions(xshandle, th, xss_path, perms, 1);
                            free(xss_path);
                            xs_transaction_end(xshandle, th, false);
                            if(pthread_create(&(listenerThreads->child), &attr, (void *)&create_listener, (void *)&domid))
                                syslog(LOG_CRIT, "Unable to create thread. %s", strerror(errno));
                            sleep(1);
                        }
                }
                listenerThreads=popThreads(listenerThreads, (const char **)xs_path, numindex);
                sleep(30);
            };
            closelog();
            exit(0);
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
            closelog();
            exit(status);
        }
    } else {
        waitpid(pid, &status, 0);
        closelog();
        exit(status);
    }
    closelog();
    exit(0);
}
