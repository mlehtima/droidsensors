%define strip /bin/true
%define __requires_exclude  ^.*$
%define __find_requires     %{nil}
%global debug_package       %{nil}
%define __provides_exclude_from ^.*$

%define _target_cpu %{device_rpm_architecture_string}

Name:          droidsensors
Summary:       Android sensor wrapper library
Version:       0.20180414.0
Release:       1
Group:         System/Libraries
License:       ASL 2.0
BuildRequires: ubu-trusty
BuildRequires: sudo-for-abuild
BuildRequires: droid-bin-src-full
Source0:       %{name}-%{version}.tgz
AutoReqProv:   no

%description
%{summary}

%package       devel
Summary:       droidsensors development headers
Requires:      droidsensors = %{version}-%{release}
BuildArch:     noarch

%description   devel
%{summary}

%prep

%if %{?device_rpm_architecture_string:0}%{!?device_rpm_architecture_string:1}
echo "device_rpm_architecture_string is not defined"
exit -1
%endif

%setup -T -c -n droidsensors
sudo chown -R abuild:abuild /home/abuild/src/droid/
mv /home/abuild/src/droid/* .
mkdir -p external
pushd external
tar -zxf %SOURCE0
mv droidsensors* droidsensors
popd

%build

droid-make %{?_smp_mflags} libdroidsensors

%install

mkdir -p $RPM_BUILD_ROOT/%{_libexecdir}/droid-hybris/system/lib/
mkdir -p $RPM_BUILD_ROOT/%{_includedir}/droidsensors/
mkdir -p $RPM_BUILD_ROOT/%{_datadir}/droidsensors/

cp out/target/product/*/system/lib/libdroidsensors.so \
    $RPM_BUILD_ROOT/%{_libexecdir}/droid-hybris/system/lib/

cp external/droidsensors/*.h $RPM_BUILD_ROOT/%{_includedir}/droidsensors/
cp external/droidsensors/hybris.c $RPM_BUILD_ROOT/%{_datadir}/droidsensors/

%files
%defattr(-,root,root,-)
%{_libexecdir}/droid-hybris/system/lib/libdroidsensors.so

%files devel
%defattr(-,root,root,-)
%{_includedir}/droidsensors/*.h
%{_datadir}/droidsensors/hybris.c
