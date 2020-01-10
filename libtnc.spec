Name:		libtnc
Version:	1.25
Release:	6%{?dist}
Summary:	Library implementation of the Trusted Network Connect (TNC) specification
License:	GPLv2
Group:		System Environment/Libraries
Source0:	http://dl.sourceforge.net/sourceforge/%{name}/%{name}-%{version}.tar.gz
URL:		http://libtnc.sourceforge.net/
BuildRoot:	%{_tmppath}/%{name}-%{version}-%{release}-root-%(%{__id_u} -n)
BuildRequires:	libxml2-devel, zlib-devel

%description
This library provides functions for loading and interfacing with loadable IMC
Integrity Measurement Collector (IMC) and Integrity Measurement Verifier (IMV)
modules as required by the Trusted Network Computing (TNC) IF-IMC and IF-IMV 
interfaces as described in: https://www.trustedcomputinggroup.org/specs/TNC

%package devel
Summary:	Development headers and libraries for libtnc
Group:		Development/Libraries
Requires:	%{name} = %{version}-%{release}

%description devel
Header and library files used for developing with (or linking to) libtnc.

%prep
%setup -q

%build
CFLAGS="%{optflags} -fPIC -DPIC"
%configure --with-pic
sed -i 's|^hardcode_libdir_flag_spec=.*|hardcode_libdir_flag_spec=""|g' libtool
sed -i 's|^runpath_var=LD_RUN_PATH|runpath_var=DIE_RPATH_DIE|g' libtool
make %{?_smp_mflags}

%install
rm -rf %{buildroot}
make DESTDIR=%{buildroot} install
rm -rf %{buildroot}/%{_libdir}/*.la
# It is easier to delete the static libs here than to disable them in configure
# Autoconf makes my brain bleed.
rm -rf %{buildroot}/%{_libdir}/*.a

%clean
rm -rf %{buildroot}

%post -p /sbin/ldconfig

%postun -p /sbin/ldconfig

%files
%defattr(-,root,root,-)
%doc COPYING README
%{_libdir}/libosc_im*.so.*
%{_libdir}/libsample_im*.so.*
%{_libdir}/libtnc.so.*

%files devel
%defattr(-,root,root,-)
%doc doc/libtnc.pdf
%{_includedir}/libtnc*.h
%{_includedir}/tnc*.h
%{_libdir}/libosc_im*.so
%{_libdir}/libsample_im*.so
%{_libdir}/libtnc.so

%changelog
* Fri Jan 24 2014 Daniel Mach <dmach@redhat.com> - 1.25-6
- Mass rebuild 2014-01-24

* Fri Dec 27 2013 Daniel Mach <dmach@redhat.com> - 1.25-5
- Mass rebuild 2013-12-27

* Thu Feb 14 2013 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.25-4
- Rebuilt for https://fedoraproject.org/wiki/Fedora_19_Mass_Rebuild

* Thu Jul 19 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.25-3
- Rebuilt for https://fedoraproject.org/wiki/Fedora_18_Mass_Rebuild

* Fri Jan 13 2012 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.25-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_17_Mass_Rebuild

* Thu Feb 17 2011 Tom Callaway <tcallawa@redhat.com> 1.25-1
- update to 1.25

* Tue Feb 08 2011 Fedora Release Engineering <rel-eng@lists.fedoraproject.org> - 1.24-2
- Rebuilt for https://fedoraproject.org/wiki/Fedora_15_Mass_Rebuild

* Fri Mar 19 2010 Tom "spot" Callaway <tcallawa@redhat.com> 1.24-1
- update to 1.24

* Fri Jan 29 2010 Tom "spot" Callaway <tcallawa@redhat.com> 1.23-1
- update to 1.23

* Wed Sep  2 2009 Tom "spot" Callaway <tcallawa@redhat.com> 1.22-1
- update to 1.22

* Tue May 12 2009 Tom "spot" Callaway <tcallawa@redhat.com> 1.19-1
- initial Fedora package
