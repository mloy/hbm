################################################
## Tests
################################################

boost_dep = dependency('boost', modules : ['unit_test_framework'])
thread_dep = dependency('threads')

multicast_test_exe = executable('multicast_test',
				'communication/multicastserver_test.cpp',
				dependencies: [ boost_dep,thread_dep,hbm_dep],
				link_with : shlib
			       )
test('multicast server', multicast_test_exe)

nonblock_socket_test_exe = executable('socket_non_blocking_test',
				      'communication/socketnonblocking_test.cpp',
				      dependencies: [ boost_dep, thread_dep,hbm_dep ],
				      link_with : shlib)
test('socket non-blocking', nonblock_socket_test_exe)


netadapter_test_exe = executable('netadapter_test',
				 'communication/netadapter_test.cpp',
				dependencies: [ boost_dep, thread_dep,hbm_dep],
				 link_with : shlib)
test('netadapter', netadapter_test_exe)



replace_test_exe = executable('replace_test',
				 'string/replacetest.cpp',
				dependencies: [ boost_dep, thread_dep,hbm_dep],
				 link_with : shlib)
test('replacetest', replace_test_exe)



split_test_exe = executable('split_test',
				 'string/splittest.cpp',
				dependencies: [ boost_dep, thread_dep,hbm_dep],
				 link_with : shlib)
test('splittest', split_test_exe)


trim_test_exe = executable('trim_test',
				 'string/trimtest.cpp',
				dependencies: [ boost_dep, thread_dep,hbm_dep],
				 link_with : shlib)
test('trimtest', trim_test_exe)


execute_test_exe = executable('execute_test',
				 'sys/executecommand_test.cpp',
				dependencies: [ boost_dep, thread_dep,hbm_dep],
				 link_with : shlib)
test('executetest', execute_test_exe)
