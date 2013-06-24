Name:       starter
Summary:    starter
Version: 0.4.62
Release:    3
Group:      TO_BE/FILLED_IN
License:    TO_BE/FILLED_IN
Source0:    starter-%{version}.tar.gz
Requires(post): /usr/bin/vconftool
BuildRequires:  cmake
BuildRequires:  pkgconfig(ail)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(capi-system-media-key)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(edje)
BuildRequires:  pkgconfig(eet)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(sysman)
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(ui-gadget-1)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xcomposite)
BuildRequires:  pkgconfig(xext)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:	pkgconfig(pkgmgr-info)
BuildRequires:	pkgconfig(libsystemd-daemon)

BuildRequires:  cmake
BuildRequires:  edje-bin
BuildRequires: gettext-tools

Requires(post): /usr/bin/vconftool

%description
Description: Starter


%prep
%setup -q

%cmake .

%build

make -j1
%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants
mkdir -p %{buildroot}%{_libdir}/systemd/user/sockets.target.wants
ln -s ../starter.path %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants/starter.path
ln -s ../starter.service %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants/starter.service
ln -s ../starter.socket %{buildroot}%{_libdir}/systemd/user/sockets.target.wants/starter.socket
mkdir -p %{buildroot}/usr/share/license
mkdir -p %{buildroot}/opt/data/home-daemon

%post
change_file_executable()
{
    chmod +x $@ 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "Failed to change the perms of $@"
    fi
}

GOPTION="-u 5000 -f"

vconftool set -t int "memory/starter/sequence" 0 -i $GOPTION
vconftool set -t int "memory/starter/use_volume_key" 0 -i $GOPTION
vconftool set -t string file/private/lockscreen/pkgname "org.tizen.lockscreen" -u 5000 -g 5000 -f
vconftool set -t int memory/idle_lock/state "0" -i $GOPTION
vconftool set -t bool memory/lockscreen/phone_lock_verification 0 -i $GOPTION

vconftool set -t bool db/lockscreen/event_notification_display 1 $GOPTION
vconftool set -t bool db/lockscreen/clock_display 1 $GOPTION
vconftool set -t bool db/lockscreen/help_text_display 0 $GOPTION

vconftool set -t int memory/idle-screen/is_idle_screen_launched "0" -i -u 5000 -f
vconftool set -t int memory/idle-screen/top "0" -i -f
vconftool set -t int memory/idle-screen/safemode "0" -i -f

ln -sf /etc/init.d/rd4starter /etc/rc.d/rc4.d/S81starter
ln -sf /etc/init.d/rd3starter /etc/rc.d/rc3.d/S43starter


%files
%manifest starter.manifest
%defattr(-,root,root,-)
%{_sysconfdir}/init.d/rd4starter
%{_sysconfdir}/init.d/rd3starter
%{_bindir}/starter
/usr/ug/lib/libug-lockscreen-options.so
/usr/ug/lib/libug-lockscreen-options.so.0.1.0
/usr/ug/res/locale/*/LC_MESSAGES/*
%{_libdir}/systemd/user/starter.path
%{_libdir}/systemd/user/starter.service
%{_libdir}/systemd/user/starter.socket
%{_libdir}/systemd/user/core-efl.target.wants/starter.path
%{_libdir}/systemd/user/core-efl.target.wants/starter.service
%{_libdir}/systemd/user/sockets.target.wants/starter.socket
/usr/share/license/%{name}
/opt/data/home-daemon
/opt/etc/smack/accesses.d/starter.rule
