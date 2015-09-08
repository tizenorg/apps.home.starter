#sbs-git:slp/pkgs/s/starter starter 0.3 f75832f2c50c8930cf1a6bfcffbac648bcde87d9
Name:       starter
Summary:    starter
Version: 0.5.52
Release:    1
Group:      TO_BE/FILLED_IN
License:    Apache-2.0
Source0:    starter-%{version}.tar.gz
Source1:    starter.service
Source2:    wait-lock.service
BuildRequires:  cmake
BuildRequires:  pkgconfig(ail)
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(capi-appfw-app-manager)
BuildRequires:  pkgconfig(capi-system-media-key)
BuildRequires:  pkgconfig(capi-network-bluetooth)

%if "%{?tizen_profile_name}" == "mobile"
BuildRequires:  tts
BuildRequires:  tts-devel
BuildRequires:  pkgconfig(capi-message-port)
%endif

BuildRequires:  pkgconfig(feedback)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(edje)
BuildRequires:	pkgconfig(edbus)
BuildRequires:  pkgconfig(eet)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(ui-gadget-1)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xcomposite)
BuildRequires:  pkgconfig(xext)
BuildRequires:	pkgconfig(alarm-service)
BuildRequires:	pkgconfig(pkgmgr-info)
BuildRequires:	pkgconfig(deviced)
BuildRequires:	pkgconfig(edbus)
BuildRequires:  pkgconfig(dbus-1)
BuildRequires:  pkgconfig(dbus-glib-1)
BuildRequires: model-build-features
BuildRequires:  cmake
BuildRequires:  edje-bin
BuildRequires:  gettext
BuildRequires:  gettext-tools
Requires(post): /usr/bin/vconftool
Requires: sys-assert

%description
Description: Starter


%prep
%setup -q

%build
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%if "%{?tizen_profile_name}" == "mobile"
echo "tizen_profile_name is 'mobile'"
%define STARTER_FEATURE_LITE "ENABLE"
export CFLAGS="$CFLAGS -DFEATURE_LITE"
export CXXFLAGS="$CXXFLAGS -DFEATURE_LITE"
%endif

cmake . -DSTARTER_FEATURE_LITE=%{STARTER_FEATURE_LITE} -DCMAKE_INSTALL_PREFIX=%{_prefix}

make
make -j1
%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/starter.service
ln -s ../starter.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/starter.service
mkdir -p %{buildroot}%{_libdir}/systemd/system/tizen-system.target.wants
install -m 0644 %SOURCE2 %{buildroot}%{_libdir}/systemd/system/wait-lock.service
ln -s ../wait-lock.service %{buildroot}%{_libdir}/systemd/system/tizen-system.target.wants/
mkdir -p %{buildroot}/usr/share/license
cp -f LICENSE %{buildroot}/usr/share/license/%{name}
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
SOPTION="-s system::vconf_inhouse"
POPTION="-s starter_private::vconf"
LOPTION="-s system::vconf_setting"

vconftool set -t string file/private/lockscreen/pkgname "org.tizen.lockscreen" $GOPTION $POPTION
vconftool set -t string file/private/lockscreen/default_pkgname "org.tizen.lockscreen" $GOPTION $POPTION

vconftool set -t int memory/idle_lock/state "0" -i $GOPTION $LOPTION
vconftool set -t bool memory/lockscreen/phone_lock_verification 0 -i $GOPTION $SOPTION

vconftool set -t int memory/idle-screen/safemode "0" -i $GOPTION $SOPTION

vconftool set -t int "memory/starter/sequence" 0 -i $GOPTION $SOPTION
vconftool set -t int "memory/starter/use_volume_key" 0 -i $GOPTION $SOPTION
vconftool set -t int db/starter/missed_call "0" -i -u 5000 -f $SOPTION
vconftool set -t int db/starter/unread_message "0" -i -u 5000 -f $SOPTION

vconftool set -t string db/svoice/package_name "com.samsung.svoice" -i -u 5000 -f -s svoice::vconf

vconftool set -t string memory/idle-screen/focused_package "" -i $GOPTION $POPTION
vconftool set -t int memory/idle-screen/is_idle_screen_launched 0 -i $GOPTION $POPTION

vconftool set -t bool memory/lockscreen/sview_state 0 -i $GOPTION $SOPTION

vconftool set -t int memory/lockscreen/security_auto_lock 1 -i $GOPTION $SOPTION

vconftool set -t int file/private/lockscreen/bt_out -70 $GOPTION $POPTION
vconftool set -t int file/private/lockscreen/bt_in -60 $GOPTION $POPTION

mkdir -p %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/
ln -s %{_libdir}/systemd/system/wait-lock.service %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/

#ln -sf /etc/init.d/rd4starter /etc/rc.d/rc4.d/S81starter
#ln -sf /etc/init.d/rd4starter /etc/rc.d/rc3.d/S81starter

sync

%files
%manifest starter.manifest
%defattr(-,root,root,-)
%{_sysconfdir}/init.d/rd4starter
%{_sysconfdir}/init.d/rd3starter
%{_bindir}/starter
%{_libdir}/systemd/system/starter.service
%{_libdir}/systemd/system/multi-user.target.wants/starter.service
%{_libdir}/systemd/system/wait-lock.service
%{_libdir}/systemd/system/tizen-system.target.wants/wait-lock.service
/usr/share/license/%{name}
/opt/data/home-daemon
/usr/share/locale/*/LC_MESSAGES/*
