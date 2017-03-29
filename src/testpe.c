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

#include <stdio.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <string.h>
#define MAX_MSG_SIZE 4096

#include "drm.h"

int main(int argc, char **argv) {
    int mysock, i;
    char clntmsg[MAX_MSG_SIZE+1],*token;
    if(argc < 2)
        {
            printf("Usage: %s <ip addr>[:port] ... [text]\n", argv[0]);
            exit (1);
        }
    if((token=strrchr(argv[1],':'))) {
        *token='\0';
        token++;
    } else
        token=strdup("80");
    if((mysock = make_inet_socket(argv[1], token)) > 0) {
        for(i=2; i < argc; i++) {
            memset(&clntmsg, 0, sizeof(clntmsg));
            strncpy(clntmsg, argv[i], MAX_MSG_SIZE);
            if(send(mysock, clntmsg, strlen(clntmsg), MSG_EOR)>0)
                dprintf("Message sent\n");
#ifdef TWO_WAY
        memset(&clntmsg, 0, sizeof(clntmsg));
        recv(mysock, clntmsg, MAX_MSG_SIZE, 0);
        dprintf("%s\n", clntmsg);
#endif
        fflush(NULL);
        }
        exit(0);
    } else {
        printf("Unable to connect to %s:%s\n",argv[1],token);
        exit(1);
    }
}
