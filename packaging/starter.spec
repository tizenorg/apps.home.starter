#sbs-git:slp/pkgs/s/starter starter 0.3 f75832f2c50c8930cf1a6bfcffbac648bcde87d9
Name:       starter
Summary:    starter
Version: 0.5.52
Release:    1
Group:      TO_BE/FILLED_IN
License:    Apache-2.0
Source0:    starter-%{version}.tar.gz
Source1:    starter.service
Source2:    starter-pre.service

%if "%{?tizen_profile_name}"=="tv"
ExcludeArch: %{arm} %ix86 x86_64
%endif

BuildRequires:  cmake
BuildRequires:  pkgconfig(appcore-efl)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(capi-appfw-application)
BuildRequires:  pkgconfig(capi-appfw-app-manager)
BuildRequires:  pkgconfig(capi-system-media-key)
BuildRequires:  pkgconfig(capi-network-bluetooth)
BuildRequires:  pkgconfig(capi-system-system-settings)
BuildRequires:  pkgconfig(capi-ui-efl-util)

%if "%{?tizen_profile_name}" == "mobile"
BuildRequires:  tts
BuildRequires:  tts-devel
BuildRequires:  pkgconfig(capi-message-port)
BuildRequires:  pkgconfig(security-server)
BuildRequires:  pkgconfig(efl-extension)
BuildRequires:  pkgconfig(callmgr_client)
%endif

BuildRequires:  pkgconfig(feedback)
BuildRequires:  pkgconfig(db-util)
BuildRequires:  pkgconfig(dlog)
BuildRequires:  pkgconfig(ecore)
BuildRequires:  pkgconfig(edje)
BuildRequires:	pkgconfig(edbus)
BuildRequires:  pkgconfig(eina)
BuildRequires:  pkgconfig(elementary)
BuildRequires:  pkgconfig(evas)
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(syspopup-caller)
BuildRequires:  pkgconfig(ui-gadget-1)
BuildRequires:  pkgconfig(utilX)
BuildRequires:  pkgconfig(vconf)
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
%define TIZEN_PROFILE_NAME "MOBILE"
export CFLAGS="$CFLAGS -DTIZEN_PROFILE_MOBILE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_PROFILE_MOBILE"
%else if "%{?tizen_profile_name}" == "wearable"
echo "tizen_profile_name is 'wearable'"
%define TIZEN_PROFILE_NAME "WEARABLE"
export CFLAGS="$CFLAGS -DTIZEN_PROFILE_WEARABLE"
export CXXFLAGS="$CXXFLAGS -DTIZEN_PROFILE_WEARABLE"
%endif

%ifarch %{arm}
export CFLAGS="$CFLAGS -DTIZEN_BUILD_TARGET"
export CXXFLAGS="$CXXFLAGS -DTIZEN_BUILD_TARGET"
%else
export CFLAGS="$CFLAGS -DTIZEN_BUILD_EMULATOR"
export CXXFLAGS="$CXXFLAGS -DTIZEN_BUILD_EMULATOR"
%endif

cmake . -DTIZEN_PROFILE_NAME=%{TIZEN_PROFILE_NAME} -DCMAKE_INSTALL_PREFIX=%{_prefix}

make
make -j1
%install
rm -rf %{buildroot}
%make_install

mkdir -p %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants
install -m 0644 %SOURCE1 %{buildroot}%{_libdir}/systemd/system/starter.service
install -m 0644 %SOURCE2 %{buildroot}%{_libdir}/systemd/system/starter-pre.service
ln -s ../starter.service %{buildroot}%{_libdir}/systemd/system/multi-user.target.wants/starter.service
mkdir -p %{buildroot}%{_libdir}/systemd/system/tizen-system.target.wants
mkdir -p %{buildroot}/usr/share/license
cp -f LICENSE %{buildroot}/usr/share/license/%{name}
mkdir -p %{buildroot}/opt/data/home-daemon

mkdir -p %{buildroot}%{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/
ln -s %{_libdir}/systemd/system/starter.service %{buildroot}%{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/
ln -s %{_libdir}/systemd/system/starter-pre.service %{buildroot}%{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/

%post
change_file_executable()
{
    chmod +x $@ 2>/dev/null
    if [ $? -ne 0 ]; then
        echo "Failed to change the perms of $@"
    fi
}

mkdir -p %{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/

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
%{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/starter.service
%{_libdir}/systemd/system/starter-pre.service
%{_sysconfdir}/systemd/default-extra-dependencies/ignore-units.d/starter-pre.service
/usr/share/license/%{name}
/opt/data/home-daemon
/usr/share/locale/*/LC_MESSAGES/*
/etc/smack/accesses.d/starter.efl

#FOR PASSWORD LOCKSCREEN
%if "%{?tizen_profile_name}" == "mobile"
/usr/share/starter/lock_pwd/res/edje/*
%endif
