Name:       starter
Summary:    starter
Version:    0.3
Release:    1
Group:      TO_BE/FILLED_IN
License:    TO_BE/FILLED_IN
Source0:    starter-%{version}.tar.gz
Requires(post): /usr/bin/vconftool
BuildRequires:  cmake
BuildRequires:  pkgconfig(glib-2.0)
BuildRequires:  pkgconfig(tapi)
BuildRequires:  pkgconfig(tapi-priv)
BuildRequires:  pkgconfig(vconf)
BuildRequires:  pkgconfig(heynoti)
BuildRequires:  pkgconfig(aul)
BuildRequires:  pkgconfig(x11)

%description
Description: Starter


%prep
%setup -q

cmake . -DCMAKE_INSTALL_PREFIX=%{_prefix}

%build

make %{?jobs:-j%jobs}
%install
rm -rf %{buildroot}
%make_install


%post
vconftool set -t int "memory/startapps/sequence" 0 -i -u 5000 -g 5000

ln -sf /etc/init.d/rd4starter /etc/rc.d/rc4.d/S81starter
ln -sf /etc/init.d/rd3starter /etc/rc.d/rc3.d/S43starter

sync

%files
%defattr(-,root,root,-)
%{_sysconfdir}/init.d/rd4starter
%{_sysconfdir}/init.d/rd3starter
%{_datadir}/applications/starter.desktop
%{_bindir}/starter

