#
# RPM Spec file for libtnc
#
# Author: Mike McCauley (mikem@open.com.au)
# Copyright (C) 2001-2006 Open System Consultants
# $Id: libtnc.spec.in,v 1.1 2006/04/11 01:19:41 mikem Exp mikem $

Summary: TNC Interface library
Name: libtnc
Version: 1.25
Release: 1
Copyright: Proprietary, Open System Consultants Pty Ltd
Group: Development/Libraries
Source: %{name}-%{version}.tgz
URL: http://www.open.com.au
Vendor: Open System Consultants Pty. Ltd.
Packager: Open System Consultants, Mike McCauley <mikem@open.com.au>
Provides: libtnc

%description
Libtnc provides dynamic loading and access to Integrity Measurement Collector (IMC)
loadable modules. Loadables modules must conform to TCG Trusted Network
Connect TNC IF-IMC specification version 1.0

%prep
%setup -q

%build
./configure
make

%install
make install

%files
%defattr(-,root,root)
%{_includedir}/libtnc.h
%{_libdir}/libtnc.a
%{_libdir}/libsample_imc.a
%{_libdir}/libsample_imc.la
%{_libdir}/libsample_imc.so
%{_libdir}/libsample_imc.so.0
%{_libdir}/libsample_imc.so.0.0.0
%config /etc/tnc_config

%changelog
* Thu Mar 30 2006 Mike McCauley <mikem@open.com.au>
