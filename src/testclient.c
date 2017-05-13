/* * * * * * * * * * * * * * * * * * * * * * * * *
 * Allan Vitangcol <allan.vitangcol@oracle.com>  *
 *                                               *
 * This program is part of the Oracle DRM project*
 * for use on Oracle OVM and Nimbula environment.*
 *                                               *
 * The program can be used for other purposes    *
 * where it fits.                                *
 *                                               *
 * This is a test code used in the absence of JVM*
 *                                               *
 * The program uses open source XenstoreSocket.  *
 * * * * * * * * * * * * * * * * * * * * * * * * */

#include <string.h>
#include <stddef.h>
#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <syslog.h>
#include <unistd.h>
#define MAX_MSG_SIZE 4096

int make_named_socket (const char *filename) {
  struct sockaddr_un name;
  int sock;

  /* Create the socket. */
  
  sock = socket (PF_UNIX, SOCK_STREAM, 0);
  if (sock < 0)
    {
      syslog(LOG_ERR, strerror(errno));
      closelog();
      exit (EXIT_FAILURE);
    }

  memset(&name, 0, sizeof(struct sockaddr_un));

  /* Bind a name to the socket. */

  name.sun_family = PF_UNIX;
  strncpy (name.sun_path + 1, filename, sizeof(name.sun_path) - 2);

  if(connect(sock, (struct sockaddr *) &name, sizeof(struct sockaddr_un)) != 0)
    {
      syslog(LOG_ERR, strerror(errno));
      closelog();
      exit (EXIT_FAILURE);
    }
  return sock;
}
int main(int argc, char **argv) {
  int mysock, i;
  char clntmsg[MAX_MSG_SIZE+1];
  if(argc < 2)
    {
      printf("Usage: %s <filename> [text] ... [text]\n", argv[0]);
      exit (1);
    }
  umask(0);
  mysock = make_named_socket(argv[1]);
  openlog(strrchr(argv[0],'/')+1, LOG_CONS | LOG_PID, LOG_USER);
  for(i=2; i < argc; i++) {
    memset(&clntmsg, 0, sizeof(clntmsg));
    strncpy(clntmsg, argv[i], MAX_MSG_SIZE);
    send(mysock, clntmsg, strlen(clntmsg), MSG_EOR);
#ifdef TWO_WAY
    memset(&clntmsg, 0, sizeof(clntmsg));
    recv(mysock, clntmsg, MAX_MSG_SIZE, 0);
    printf("%s\n", clntmsg);
#endif
    fflush(NULL);
  }
  shutdown(mysock, SHUT_RDWR);
  close(mysock);
  closelog();
  exit(0);
}
