project('Lupus', 'c', default_options : ['c_std=c18', 'warning_level=3'])

gnome = import('gnome')

gtk = dependency('gtk+-3.0', required: true)
libsodium = dependency('libsodium', required: true)
toxcore = dependency('toxcore', required: true)

subdir('src')
subdir('include')

resources = gnome.compile_resources(
	'resources', meson.source_root() + '/res/lupus.gresource.xml',
	source_dir: meson.source_root() + '/res/'
	)

executable(
	meson.project_name(),
	sources: [
		'main.c',
		srcs,
		hdrs,
		resources,
		],
	dependencies: [
		gtk,
		libsodium,
		toxcore
		]
	)
