
version_array = meson.project_version().split('.')

conf_data = configuration_data()
conf_data.set('HBM_MAJOR_VERSION',version_array[0] )
conf_data.set('HBM_MINOR_VERSION' , version_array[1] )
conf_data.set('HBM_TAG_NUMBER' , version_array[2])

version_h = configure_file(input : 'version.h.in',
               output : 'version.h',
               configuration : conf_data)

install_headers(version_h, subdir : 'hbm')

#include_dirs = include_directories('.')
include_dirs = include_directories('../include')

lib_src = [
  'communication/ipv4address.cpp',
  'communication/ipv6address.cpp',
  'exception/jsonrpc_exception.cpp',
  'string/replace.cpp',
  'string/split.cpp',
  'string/trim.cpp',
  'sys/pidfile.cpp'
  ]

if target_machine.system() == 'linux'
  lib_src += [  'communication/linux/bufferedreader.cpp',
		'communication/linux/multicastserver.cpp',
		'communication/linux/netadapter.cpp',
		'communication/linux/netadapterlist.cpp',
		'communication/linux/netlink.cpp',
		'communication/linux/socketnonblocking.cpp',
		'communication/linux/tcpserver.cpp',
		'sys/linux/eventloop.cpp',
		'sys/linux/executecommand.cpp',
		'sys/linux/notifier.cpp',
		'sys/linux/timer.cpp'
	     ]
endif
#if meson.system() == 'windows'
#  lib_src += [
#
#  ]

shlib = shared_library('hbm', lib_src,
  install : true,
#  cpp_args : lib_args,
  include_directories: include_dirs,
  options: [
    'strip=true',
    'werror=true',
    'warning_level=3',
    'optimization=s',
    'unity=on',
    'debug=false',
    'buildtype=minsize',
    'b_coverage=true',
    'b_lto=true',
    'b_sanitize=address'
  ]
#  gnu_symbol_visibility : 'hidden',
)

# Make this library usable as a Meson subproject.
hbm_dep = declare_dependency(
  include_directories: include_dirs,
  link_with : shlib)


pkg_mod = import('pkgconfig')

pkg_mod.generate(
  name : 'hbm',
  filebase : 'hbm',
  description : 'HBM SCAN DLL.',
  subdirs : 'hbm',
  libraries : shlib,
  version : meson.project_version() ,
)
