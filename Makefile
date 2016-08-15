# make with GCOV='-fprofile-arcs -ftest-coverage' to test coverage
CC:=gcc
UNAME:=$(shell uname)
XENVERSION:=$(shell rpm -q --qf "%{version}" xen-devel | cut -c1)
CFLAGS:=-O2 -Wall -Wextra -D_XOPEN_SOURCE=700 -D__USE_POSIX=2000 -D$(UNAME) -DXENVERSION=$(XENVERSION) -DMTAB=\"/proc/self/mounts\" -DTWO_WAY $(DEBUG) $(GCOV)
LDFLAGS:=-Wall -Wextra -lpthread -pthread $(GCOV)
DOMSERVER := dom-serverd
DOMCLIENT := dom-clientd
TESTDOMU := domU
TESTCLIENT := testclient
TESTPE := testpe
TESTSERVER := threadserverd

all:	$(DOMSERVER) $(DOMCLIENT) $(TESTDOMU) $(TESTSERVER) $(TESTCLIENT) $(TESTPE)

clean:
	rm -f *.o *.gc?? $(DOMSERVER) $(DOMCLIENT) $(TESTDOMU) $(TESTSERVER) $(TESTCLIENT) $(TESTPE)

clean.obj:
	rm -f *.o

%.o: %.c %.h
	$(CC) $(CFLAGS) -c -o $@ $<

standalone.o: threadserver.c drm.h
	$(CC) -o $@ $(CFLAGS) -DSTANDALONE -c $<

$(DOMCLIENT): xss.o domClient.o threadserver.o mntent.o
	$(CC) -o $@ $(LDFLAGS) -lxenstore $^
$(TESTSERVER): standalone.o
	$(CC) -o $@ $(LDFLAGS) $^
$(TESTDOMU): xss.o domClient.o testdomU.o mntent.o
	$(CC) -o $@ $(LDFLAGS) -lxenstore $^
$(DOMSERVER): xss.o domServer.o
	$(CC) -o $@ $(LDFLAGS) -lxenstore $^
$(TESTCLIENT): testclient.o
	$(CC) -o $@ $(LDFLAGS) $^
$(TESTPE): testpe.o peClient.o
	$(CC) -o $@ $(LDFLAGS) $^
