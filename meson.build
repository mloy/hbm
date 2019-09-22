project('hbm', 'cpp',
	version: '1.0.33',
	default_options : ['warning_level=3',
			   'cpp_std=c++11',
			   'prefix=/usr',
			  ])


version_array = meson.project_version().split('.')

conf_data = configuration_data()
conf_data.set('HBM_MAJOR_VERSION',version_array[0] )
conf_data.set('HBM_MINOR_VERSION' , version_array[1] )
conf_data.set('HBM_TAG_NUMBER' , version_array[2])

version_h = configure_file(input : 'lib/version.h.in',
               output : 'version.h',
               configuration : conf_data)

#include_dirs = include_directories('.')
include_dirs = include_directories('include')

install_headers(version_h, subdir : 'hbm')

# These arguments are only used to build the shared library
# not the executables that use the library.
lib_args = ['-DBUILDING_HBM']

lib_src = [
  'lib/communication/ipv4address.cpp',
  'lib/communication/ipv6address.cpp',
  'lib/exception/jsonrpc_exception.cpp',
  'lib/string/replace.cpp',
  'lib/string/split.cpp',
  'lib/string/trim.cpp',
  'lib/sys/pidfile.cpp'
  ]

if target_machine.system() == 'linux'
  lib_src += [  'lib/communication/linux/bufferedreader.cpp',
		'lib/communication/linux/multicastserver.cpp',
		'lib/communication/linux/netadapter.cpp',
		'lib/communication/linux/netadapterlist.cpp',
		'lib/communication/linux/netlink.cpp',
		'lib/communication/linux/socketnonblocking.cpp',
		'lib/communication/linux/tcpserver.cpp',
		'lib/sys/linux/eventloop.cpp',
		'lib/sys/linux/executecommand.cpp',
		'lib/sys/linux/notifier.cpp',
		'lib/sys/linux/timer.cpp'
	     ]
endif
#if meson.system() == 'windows'
#  lib_src += [
#
#  ]

shlib = shared_library('hbm', lib_src,
  install : true,
  cpp_args : lib_args,
  include_directories: include_dirs,
  build_options: [
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

jsoncpp_dep = dependency('jsoncpp')

executable('netadapterview','tool/communication/netadapterview.cpp', dependencies: [hbm_dep,jsoncpp_dep], install:true )

executable('netlinkview','tool/communication/netlinkview.cpp', dependencies: [hbm_dep,jsoncpp_dep],install:true  )

executable('server','tool/communication/Server.cpp', dependencies: [hbm_dep,jsoncpp_dep],install:true  )

executable('client','tool/communication/Client.cpp', dependencies: [hbm_dep,jsoncpp_dep],install:true  )

executable('eventloopperformance','tool/sys/eventloopperformance.cpp', dependencies: hbm_dep,install:true  )




################################################
## Tests
################################################

boost_dep = dependency('boost', modules : ['unit_test_framework'])
thread_dep = dependency('threads')

multicast_test_exe = executable('multicast_test',
				'test/communication/multicastserver_test.cpp',
				dependencies: [ boost_dep,thread_dep,hbm_dep],
				link_with : shlib
			       )
test('multicast server', multicast_test_exe)

nonblock_socket_test_exe = executable('socket_non_blocking_test',
				      'test/communication/socketnonblocking_test.cpp',
				      dependencies: [ boost_dep, thread_dep,hbm_dep ],
				      link_with : shlib)
test('socket non-blocking', nonblock_socket_test_exe)


netadapter_test_exe = executable('netadapter_test',
				 'test/communication/netadapter_test.cpp',
				dependencies: [ boost_dep, thread_dep,hbm_dep],
				 link_with : shlib)
test('netadapter', netadapter_test_exe)



replace_test_exe = executable('replace_test',
				 'test/string/replacetest.cpp',
				dependencies: [ boost_dep, thread_dep,hbm_dep],
				 link_with : shlib)
test('replacetest', replace_test_exe)



split_test_exe = executable('split_test',
				 'test/string/splittest.cpp',
				dependencies: [ boost_dep, thread_dep,hbm_dep],
				 link_with : shlib)
test('splittest', split_test_exe)


trim_test_exe = executable('trim_test',
				 'test/string/trimtest.cpp',
				dependencies: [ boost_dep, thread_dep,hbm_dep],
				 link_with : shlib)
test('trimtest', trim_test_exe)


execute_test_exe = executable('execute_test',
				 'test/sys/executecommand_test.cpp',
				dependencies: [ boost_dep, thread_dep,hbm_dep],
				 link_with : shlib)
test('executetest', execute_test_exe)


#################################################
## Installetion
#################################################

# Make this library usable from the system's
# package manager.

comm_header = [
'include/hbm/communication/ipv4address.h',
'include/hbm/communication/ipv6address.h',
'include/hbm/communication/bufferedreader.h',
'include/hbm/communication/multicastserver.h',
'include/hbm/communication/netadapter.h',
'include/hbm/communication/netadapterlist.h',
'include/hbm/communication/netlink.h',
'include/hbm/communication/socketnonblocking.h',
'include/hbm/communication/tcpserver.h',
'include/hbm/debug/stack_trace.hpp',
'include/hbm/exception/errno_exception.hpp',
'include/hbm/exception/exception.hpp',
'include/hbm/exception/jsonrpc_exception.h',
'include/hbm/jsonrpc/jsonrpc_defines.h',
'include/hbm/string/replace.h',
'include/hbm/string/trim.h',
'include/hbm/string/split.h',
'include/hbm/sys/defines.h',
'include/hbm/sys/eventloop.h',
'include/hbm/sys/executecommand.h',
'include/hbm/sys/notifier.h',
'include/hbm/sys/pidfile.h',
'include/hbm/sys/timer.h'
]




install_subdir('include/hbm',install_dir:'include/')

pkg_mod = import('pkgconfig')

pkg_mod.generate(
  name : 'hbm',
  filebase : 'hbm',
  description : 'HBM SCAN DLL.',
  subdirs : 'hbm',
  libraries : shlib,
  version : meson.project_version() ,
)
