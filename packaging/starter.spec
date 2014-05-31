#sbs-git:slp/pkgs/s/starter starter 0.3 f75832f2c50c8930cf1a6bfcffbac648bcde87d9
Name:       starter
Summary:    starter
Version: 0.6.7
Release:    1
Group:      TO_BE/FILLED_IN
License:    TO_BE/FILLED_IN
Source0:    starter-%{version}.tar.gz
Source1:    starter.service
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
BuildRequires:	pkgconfig(edbus)
BuildRequires:  pkgconfig(eet)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(x11)
BuildRequires:  pkgconfig(xcomposite)
BuildRequires:  pkgconfig(xext)
BuildRequires:	pkgconfig(pkgmgr-info)
BuildRequires:	pkgconfig(alarm-service)
BuildRequires:	pkgconfig(feedback)
BuildRequires:	pkgconfig(syspopup-caller)
BuildRequires:	pkgconfig(bundle)
BuildRequires:	pkgconfig(deviced)
%if "%{_repository}" == "mobile"
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(sysman)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(ui-gadget-1)
BuildRequires:  pkgconfig(capi-system-info)
BuildRequires:	pkgconfig(libsystemd-daemon)
%endif
BuildRequires:  cmake
BuildRequires:  edje-bin
BuildRequires: gettext-tools
Requires(post): /usr/bin/vconftool
Requires: sys-assert


%description
Description: Starter


%prep
%setup -q

%build
%if 0%{?tizen_build_binary_release_type_eng}
export CFLAGS="$CFLAGS -DTIZEN_ENGINEER_MODE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_ENGINEER_MODE"
export FFLAGS="$FFLAGS -DTIZEN_ENGINEER_MODE"
%endif
%if 0%{?sec_build_binary_debug_enable}
export CFLAGS="$CFLAGS -DTIZEN_DEBUG_ENABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_DEBUG_ENABLE"
export FFLAGS="$FFLAGS -DTIZEN_DEBUG_ENABLE"
%endif

%if "%{_repository}" == "wearable"
export BUILD_PATH="."
export MOBILE=Off
export WEARABLE=On
%if ("%{sec_build_project_name}" == "tizenw2_master")
	export FEATURE_TIZENW2="YES"
%endif
%elseif "%{_repository}" == "mobile"
export BUILD_PATH="mobile"
export MOBILE=On
export WEARABLE=Off
%endif

%cmake ${BUILD_PATH} -DARCH=%{ARCH} -DFEATURE_TIZENW2=${FEATURE_TIZENW2} -DMOBILE=${MOBILE} -DWEARABLE=${WEARABLE}
make %{?jobs:-j%jobs}

%install
rm -rf %{buildroot}
%make_install

%if "%{_repository}" == "wearable"
mkdir -p %{buildroot}%{_libdir}/systemd/system/tizen-system.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/starter.service
ln -s ../starter.service %{buildroot}%{_libdir}/systemd/system/tizen-system.target.wants/starter.service
mkdir -p %{buildroot}/usr/share/license
cp -f LICENSE %{buildroot}/usr/share/license/%{name}
mkdir -p %{buildroot}/opt/data/home-daemon
%else
mkdir -p %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants
mkdir -p %{buildroot}%{_libdir}/systemd/user/sockets.target.wants
ln -s ../starter.path %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants/starter.path
ln -s ../starter.service %{buildroot}%{_libdir}/systemd/user/core-efl.target.wants/starter.service
ln -s ../starter.socket %{buildroot}%{_libdir}/systemd/user/sockets.target.wants/starter.socket
mkdir -p %{buildroot}/usr/share/license
mkdir -p %{buildroot}/opt/data/home-daemon
%endif

%post
change_file_executable()
{
    chmod +x $@ 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "Failed to change the perms of $@"
    fi
}

GOPTION="-u 5000 -f"
%if "%{_repository}" == "wearable"
SOPTION="-s system::vconf_inhouse"
POPTION="-s starter_private::vconf"
LOPTION="-s starter::vconf"

vconftool set -t string file/private/lockscreen/pkgname "com.samsung.lockscreen" $GOPTION $POPTION
vconftool set -t string file/private/lockscreen/default_pkgname "com.samsung.lockscreen" $GOPTION $POPTION

vconftool set -t int memory/idle_lock/state "0" -i $GOPTION $LOPTION
vconftool set -t bool memory/lockscreen/phone_lock_verification 0 -i $GOPTION $SOPTION

vconftool set -t int memory/idle-screen/safemode "0" -i -f $SOPTION

vconftool set -t int "memory/starter/sequence" 0 -i $GOPTION $SOPTION
vconftool set -t int "memory/starter/use_volume_key" 0 -i $GOPTION $SOPTION
vconftool set -t int db/starter/missed_call "0" -i -u 5000 -f $SOPTION
vconftool set -t int db/starter/unread_message "0" -i -u 5000 -f $SOPTION

vconftool set -t string db/svoice/package_name "com.samsung.svoice" -i -u 5000 -f -s svoice::vconf

vconftool set -t string memory/idle-screen/focused_package "" -i $GOPTION $POPTION
vconftool set -t int memory/idle-screen/is_idle_screen_launched 0 -i $GOPTION $POPTION
%else
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
%endif

#ln -sf /etc/init.d/rd4starter /etc/rc.d/rc4.d/S81starter
#ln -sf /etc/init.d/rd4starter /etc/rc.d/rc3.d/S81starter

%files
%manifest starter.manifest
%defattr(-,root,root,-)

%if "%{_repository}" == "wearable"
%{_sysconfdir}/init.d/rd4starter
%{_sysconfdir}/init.d/rd3starter
%{_bindir}/starter
%{_libdir}/systemd/system/starter.service
%{_libdir}/systemd/system/tizen-system.target.wants/starter.service
/usr/share/license/%{name}
/opt/data/home-daemon
%else
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
%endif
