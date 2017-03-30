Name:		drm
Version:	1
Release:	1.2.1%{?dist}
Summary:	Dynamic Resource Management

License:	Allan Vitangcol
Source0:	drm-%{version}.tgz
BuildRoot:	%(mktemp -ud %{_tmppath}/%{name}-%{version}-%{release}-XXXXXX)

%description
This package contains dynamic resource management executables and test executables

%prep
%setup -c

%build
%define debug_package %{nil}
make EXEDIR=exe clean
make EXEDIR=exe DOMSERVER=dom-serverd DOMCLIENT=dom-clientd exe/dom-serverd exe/dom-clientd
make EXEDIR=exe TESTDOMU=domU TESTSERVER=threadserverd TESTCLIENT=testclient TESTPE=testpe DEBUG=-DDEBUG_DRM exe/domU exe/threadserverd exe/testclient exe/testpe

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}%{_sbindir}
install -m 755 exe/dom-serverd %{buildroot}%{_sbindir}
install -m 755 exe/dom-clientd %{buildroot}%{_sbindir}
mkdir -p %{buildroot}/usr/local/bin
install -m 755 exe/domU %{buildroot}/usr/local/bin
install -m 755 exe/threadserverd %{buildroot}/usr/local/bin
install -m 755 exe/testclient %{buildroot}/usr/local/bin
install -m 755 exe/testpe %{buildroot}/usr/local/bin
%if "%dist" >= ".el7"
mkdir -p %{buildroot}%{_unitdir}
install -m 644 scripts/dom-serverd.service %{buildroot}%{_unitdir}
install -m 644 scripts/dom-clientd.service %{buildroot}%{_unitdir}
mkdir -p %{buildroot}%{_sysconfdir}/systemd/system/dom-clientd.d/
install -m 644 scripts/dom-clientd.conf %{buildroot}%{_sysconfdir}/systemd/system/dom-clientd.d/
%else
mkdir -p %{buildroot}%{_sysconfdir}/sysconfig
install -m 644 scripts/dom-clientd.sysconfig %{buildroot}%{_sysconfdir}/sysconfig/dom-clientd
mkdir -p %{buildroot}%{_initrddir}
install -m 755 scripts/dom-serverd.init %{buildroot}%{_initrddir}/dom-serverd
install -m 755 scripts/dom-clientd.init %{buildroot}%{_initrddir}/dom-clientd
%endif

%clean
rm -rf %{buildroot}
make EXEDIR=exe clean

%package server
Summary:	DRM Server Binary
Requires:	/boot/xen.gz
%if "%dist" >= ".el7"
%systemd_requires
%else
Requires:	chkconfig
%endif
Conflicts:	%{name}-client
Conflicts:	%{name}-tools
Group:		System Environment/Daemons
%description server
This package contains dynamic resource management hypervisor executable

%package client
Summary:	DRM Client Binary
%if "%dist" >= ".el7"
%systemd_requires
%else
Requires:	chkconfig
%endif
Conflicts:	/boot/xen.gz
Conflicts:	%{name}-server
Group:		System Environment/Daemons
%description client
This package contains dynamic resource management domU executable

%package tools
Summary:	DRM Debugging Tools Binary
Conflicts:	/boot/xen.gz
Conflicts:	%{name}-server
Group:		System Tools
%description tools
This package contains dynamic resource management debugging tools executable

%pre server
rpm -qf /boot/xen.gz > /dev/null 2>&1 || exit $?

%pre client
rpm -qf /boot/xen.gz > /dev/null 2>&1 || exit 0
exit 1

%pre tools
rpm -qf /boot/xen.gz > /dev/null 2>&1 || exit 0
exit 1

%preun server
%if "%dist" >= ".el7"
%systemd_preun dom-serverd.service
%else
/sbin/service dom-serverd stop
%endif

%post server
%if "%dist" >= ".el7"
%systemd_post dom-serverd.service
%else
/sbin/chkconfig --add dom-serverd
%endif

%postun server
%if "%dist" >= ".el7"
%systemd_postun dom-serverd.service
%else
/sbin/chkconfig --del dom-serverd
%endif

%preun client
%if "%dist" >= ".el7"
%systemd_preun dom-clientd.service
%else
/sbin/service dom-clientd stop
%endif

%post client
%if "%dist" >= ".el7"
%systemd_post dom-clientd.service
%else
/sbin/chkconfig --add dom-clientd
%endif

%postun client
%if "%dist" >= ".el7"
%systemd_postun dom-clientd.service
%else
/sbin/chkconfig --del dom-clientd
%endif

%files server
%attr(0755,root,root) %{_sbindir}/dom-serverd
%if "%dist" >= ".el7"
%attr(0644,root,root) %{_unitdir}/dom-serverd.service
%else
%attr(0755,root,root) %{_initrddir}/dom-serverd
%endif

%files client
%attr(0755,root,root) %{_sbindir}/dom-clientd
%if "%dist" >= ".el7"
%attr(0644,root,root) %{_unitdir}/dom-clientd.service
%dir %attr(0755,root,root) %{_sysconfdir}/systemd/system/dom-clientd.d
%config(noreplace) %attr(0644,root,root) %{_sysconfdir}/systemd/system/dom-clientd.d/dom-clientd.conf
%else
%attr(0755,root,root) %{_initrddir}/dom-clientd
%config(noreplace) %attr(0644,root,root) %{_sysconfdir}/sysconfig/dom-clientd
%endif

%files tools
%defattr(755,root,root,-)
/usr/local/bin/domU
/usr/local/bin/threadserverd
/usr/local/bin/testclient
/usr/local/bin/testpe

%changelog
* Thu Mar 30 2017 Allan Vitangcol <allan.vitangcol@oracle,com>
- Streamline Makefile and spec file.
- Added init scripts for sysV and service files for systemd.

* Tue Mar 28 2017 Allan Vitangcol <allan.vitangcol@oracle.com>
- Create separate obj, exe directories during build

* Thu Jul 09 2015 Allan Vitangcol <allan.vitangcol@oracle.com>
- Disabled useless debuginfo.

* Tue Jul 07 2015 Allan Vitangcol <allan.vitangcol@oracle.com>
- Added preinstall script to avoid spoofed /boot/xen.gz.

* Mon Jun 29 2015 Allan Vitangcol <allan.vitangcol@oracle.com>
- Added test tools.

* Fri Jun 26 2015 Allan Vitangcol <allan.vitangcol@oracle.com>
- First version.
