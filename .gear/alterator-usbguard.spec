%define _altdata_dir %_datadir/alterator

Name: alterator-usbguard
Version: 0.1
Release: alt1
Summary: alterator module to control usb devices
Group: System/Configuration/Other
License: %gpl2plus
Url: https://gitlab.basealt.space/proskurinov/alterator_usbguard

Source: %name-%version.tar
Source1: %name-%version-thirdparty-cppcodec.tar
Source2: %name-%version-thirdparty-rapidcsv.tar

Requires: usbids usbguard
BuildPreReq: gcc-c++ cmake ninja-build rpm-macros-cmake rpm-build-licenses 

BuildRequires: usbguard-devel boost-devel-headers  libsdbus-cpp-devel libsystemd-devel doxygen

%description
Alterator Module to control USB devices via USBGuard.

%prep
%setup -a0 -a1 -a2  

%build
%cmake -DCMAKE_BUILD_TYPE:STRING=Release -G Ninja 
%cmake_build

%install
%cmake_install --config Release

%files
%_altdata_dir/applications/USBGuard.desktop
%_altdata_dir/design/scripts/alt-usb-guard.js
%_altdata_dir/design/styles/alt_usb_guard.css
%_altdata_dir/ui/usbguard/ajax.scm
%_altdata_dir/ui/usbguard/index.html
%_usr/lib/alterator/backend3/usbguard
%_sysconfdir/usbguard/android_vidpid.json

%changelog
* Mon Mar 04 2024 Oleg Proskurin <proskur@altlinux.org> 0.1-alt1
- Initial build