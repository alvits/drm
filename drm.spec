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
make clean
make EXEDIR=exe DOMSERVER=dom-serverd DOMCLIENT=dom-clientd exe/dom-serverd exe/dom-clientd
make clean.obj
make EXEDIR=exe TESTDOMU=domU TESTSERVER=threadserverd TESTCLIENT=testclient TESTPE=testpe DEBUG=-DDEBUG_DRM exe/domU exe/threadserverd exe/testclient exe/testpe

%install
rm -rf %{buildroot}
mkdir -p %{buildroot}/usr/sbin
install -m 755 exe/dom-serverd %{buildroot}/usr/sbin
install -m 755 exe/dom-clientd %{buildroot}/usr/sbin
mkdir -p %{buildroot}/usr/local/bin
install -m 755 exe/domU %{buildroot}/usr/local/bin
install -m 755 exe/threadserverd %{buildroot}/usr/local/bin
install -m 755 exe/testclient %{buildroot}/usr/local/bin
install -m 755 exe/testpe %{buildroot}/usr/local/bin

%clean
rm -rf %{buildroot}

%package server
Summary:	DRM Server Binary
Requires:	/boot/xen.gz
Conflicts:	%{name}-client
Conflicts:	%{name}-tools
Group:		System Environment/Daemons
%description server
This package contains dynamic resource management hypervisor executable

%package client
Summary:	DRM Client Binary
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

%files server
%defattr(755,root,root,-)
/usr/sbin/dom-serverd

%files client
%defattr(755,root,root,-)
/usr/sbin/dom-clientd

%files tools
%defattr(755,root,root,-)
/usr/local/bin/domU
/usr/local/bin/threadserverd
/usr/local/bin/testclient
/usr/local/bin/testpe

%changelog
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
