Name:       starter
Summary:    starter
Version:    0.3.44
Release:    1
Group:      TO_BE/FILLED_IN
License:    Apache-2.0
Source0:    starter-%{version}.tar.gz
Requires(post): /usr/bin/vconftool
BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(ecore-evas)
BuildRequires:  pkgconfig(eet)
BuildRequires:  pkgconfig(ecore-input)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(ui-gadget)

%description
Description: Starter


%prep
%setup -q


%build
cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}
make 

%install
%make_install


%post
vconftool set -t int "memory/startapps/sequence" 0 -i -u 5000 -g 5000
vconftool set -t string db/lockscreen/pkgname "org.tizen.idle-lock" -u 5000 -g 5000
vconftool -i set -t int memory/idle_lock/state "0" -u 5000 -g 5000

ln -sf /etc/init.d/rd4starter /etc/rc.d/rc4.d/S81starter
ln -sf /etc/init.d/rd3starter /etc/rc.d/rc3.d/S43starter

%files
%{_sysconfdir}/init.d/rd4starter
%{_sysconfdir}/init.d/rd3starter
%{_datadir}/applications/starter.desktop
%{_bindir}/starter
%{_libdir}/liblock-daemon.so
