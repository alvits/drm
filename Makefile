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
SRCDIR:=src
OBJDIR:=obj
EXEDIR:=exe
TOPDIR:=$(shell rpmbuild --showrc|sed 's/^-14: _topdir\s*//p;d'|sed 's/%{getenv:\([^}]*\)}\(\/.*\)/$$\1\2/')

all:	$(EXEDIR)/$(DOMSERVER) $(EXEDIR)/$(DOMCLIENT) $(EXEDIR)/$(TESTDOMU) $(EXEDIR)/$(TESTSERVER) $(EXEDIR)/$(TESTCLIENT) $(EXEDIR)/$(TESTPE)

clean:
	rm -rf $(OBJDIR) *.gc?? $(EXEDIR)

clean.obj:
	rm -rf $(OBJDIR)

$(OBJDIR)/standalone.o: $(SRCDIR)/threadserver.c $(SRCDIR)/drm.h
	@mkdir -p $(OBJDIR)
	$(CC) -o $@ $(CFLAGS) -DSTANDALONE -c $<

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/*.h
	@mkdir -p $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXEDIR)/$(DOMCLIENT): $(OBJDIR)/xss.o $(OBJDIR)/domClient.o $(OBJDIR)/threadserver.o $(OBJDIR)/mntent.o
	@mkdir -p $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) -lxenstore $^

$(EXEDIR)/$(TESTSERVER): $(OBJDIR)/standalone.o
	@mkdir -p $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) $^

$(EXEDIR)/$(TESTDOMU): $(OBJDIR)/xss.o $(OBJDIR)/domClient.o $(OBJDIR)/testdomU.o $(OBJDIR)/mntent.o
	@mkdir -p $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) -lxenstore $^

$(EXEDIR)/$(DOMSERVER): $(OBJDIR)/xss.o $(OBJDIR)/domServer.o
	@mkdir -p $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) -lxenstore $^

$(EXEDIR)/$(TESTCLIENT): $(OBJDIR)/testclient.o
	@mkdir -p $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) $^

$(EXEDIR)/$(TESTPE): $(OBJDIR)/testpe.o $(OBJDIR)/peClient.o
	@mkdir -p $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) $^

package:
	tar czf $(TOPDIR)/SOURCES/drm-1.tgz *
	rpmbuild --sign -ba drm.spec
