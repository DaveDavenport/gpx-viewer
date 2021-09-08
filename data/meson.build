icon_sizes = ['16', '22', '24', '32', '48', '128', '256']
foreach i : icon_sizes
	install_data(
		'icons' / i + 'x' + i / 'apps' / 'gpx-viewer.png',
		install_dir: get_option('datadir') / 'icons' / 'hicolor' / i + 'x' + i / 'apps',
	)
endforeach
install_data(
	'icons' / 'scalable' / 'apps' / 'gpx-viewer.svg',
	install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'apps',
)

pin_colors = ['blue', 'green', 'grey', 'orange', 'purple', 'red', 'yellow']
foreach i : pin_colors
	install_data(
		'icons' / 'scalable' / 'status' / 'pin-' + i + '.svg',
		install_dir: get_option('datadir') / 'icons' / 'hicolor' / 'scalable' / 'status',
	)
endforeach

install_data(
	'default-layout.xml',
	install_dir: get_option('datadir') / meson.project_name(),
)

i18n = import('i18n')
desktop_file = i18n.merge_file(
	'desktop',
	input: meson.project_name() + '.desktop.in',
	output: meson.project_name() + '.desktop',
	install: true,
	install_dir: get_option('datadir') / 'applications',
	po_dir: meson.source_root() / 'po',
	type: 'desktop'
)
