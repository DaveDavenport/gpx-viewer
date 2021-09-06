project(
	'gpx-viewer',
	'vala', 'c',
	version: '0.4.0'
)

gnome = import('gnome')
i18n = import('i18n')

add_global_arguments('-DGETTEXT_PACKAGE="@0@"'.format (meson.project_name()), language:'c')

vapi_dir = join_paths(meson.current_source_dir(), 'vapi')
add_project_arguments(['--vapidir', vapi_dir, '--pkg', 'config', '--use-header'], language: 'vala')
add_project_arguments(['--include', 'config.h'], language: 'c')

# https://gitlab.gnome.org/GNOME/libchamplain/-/merge_requests/11
add_project_arguments(['--disable-assert'], language: 'vala')

config_data = configuration_data()
config_data.set('package', meson.project_name())
config_data.set('package_name', 'GPX Viewer')
config_data.set('package_url', 'http://blog.sarine.nl/gpx-viewer/')
config_data.set('datadir', get_option('prefix') / get_option('datadir'))
config_data.set('localedir', get_option('prefix') / get_option('datadir') / 'locale')
config_data.set('version', meson.project_version())

config_file = configure_file(
    input: 'config.h.in',
    output: 'config.h',
    configuration: config_data
)

resources = gnome.compile_resources(
	'resources', 'data/gpx-viewer.gresource.xml',
	source_dir: 'data',
	c_name: 'resources'
)

gpx_deps = [
	dependency ('gmodule-2.0'),
	dependency ('gtk+-3.0', version: '>=3.0'),
	dependency ('libxml-2.0'),
	dependency ('gdl-3.0'),
	dependency ('champlain-0.12', version: '>=0.12.3'),
	dependency ('champlain-gtk-0.12', version: '>=0.12.3'),
	dependency ('clutter-gtk-1.0', version: '>=1.4.0'),
	meson.get_compiler('c').find_library('m'),
]

subdir('libgpx')
subdir('src')

install_data(
	'mime' / 'gpx-viewer.xml',
	install_dir: join_paths(get_option('datadir'), 'mime', 'packages')
)

subdir('data')
subdir('po')
