# make with GCOV='-fprofile-arcs -ftest-coverage' to test coverage
CC:=gcc
UNAME:=$(shell uname)
XENVERSION:=$(shell rpm --quiet -q xen-devel && rpm -q --qf "%{version}" xen-devel | cut -c1)
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
TOPDIR:=$(shell rpm --eval '%_topdir')
SIGN:=$(shell (rpm --eval '%_gpg_name' | grep -q '%_gpg_name') || ([ -d ${HOME}/.gnupg ] && [ -n "$$(gpg --list-keys 2>/dev/null)" ] && echo --sign))

ifndef XENVERSION
    $(error xen-devel package is not installed)
endif

all:	$(EXEDIR)/$(DOMSERVER) $(EXEDIR)/$(DOMCLIENT) $(EXEDIR)/$(TESTDOMU) $(EXEDIR)/$(TESTSERVER) $(EXEDIR)/$(TESTCLIENT) $(EXEDIR)/$(TESTPE)

clean:
	rm -rf $(OBJDIR) *.gc?? $(EXEDIR)

clean.obj:
	rm -rf $(OBJDIR)

$(OBJDIR):
	@mkdir -p $@

$(EXEDIR):
	@mkdir -p $@

$(OBJDIR)/standalone.o: $(SRCDIR)/threadserver.c $(SRCDIR)/drm.h | $(OBJDIR)
	$(CC) -o $@ $(CFLAGS) -DSTANDALONE -c $<

$(OBJDIR)/%.o: $(SRCDIR)/%.c $(SRCDIR)/*.h | $(OBJDIR)
	$(CC) $(CFLAGS) -c -o $@ $<

$(EXEDIR)/$(DOMCLIENT): $(OBJDIR)/xss.o $(OBJDIR)/domClient.o $(OBJDIR)/threadserver.o $(OBJDIR)/mntent.o | $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) -lxenstore $^

$(EXEDIR)/$(TESTSERVER): $(OBJDIR)/standalone.o | $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) $^

$(EXEDIR)/$(TESTDOMU): $(OBJDIR)/xss.o $(OBJDIR)/domClient.o $(OBJDIR)/testdomU.o $(OBJDIR)/mntent.o | $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) -lxenstore $^

$(EXEDIR)/$(DOMSERVER): $(OBJDIR)/xss.o $(OBJDIR)/domServer.o | $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) -lxenstore $^

$(EXEDIR)/$(TESTCLIENT): $(OBJDIR)/testclient.o | $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) $^

$(EXEDIR)/$(TESTPE): $(OBJDIR)/testpe.o $(OBJDIR)/peClient.o | $(EXEDIR)
	$(CC) -o $@ $(LDFLAGS) $^

$(TOPDIR)/SOURCES/drm-1.tgz: *
	@mkdir -p $(@D)
	@tar czf $@ $^

package: $(TOPDIR)/SOURCES/drm-1.tgz
	@rpmbuild $(SIGN) -ba drm.spec
