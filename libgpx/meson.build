libgpx = static_library(
	'gpx',
	'fit-parser.vala',
	'gpx-convert.vala',
	'gpx-file-base.vala',
	'gpx-graph.vala',
	'gpx-parser.vala',
	'gpx-parser-xml.vala',
	'gpx-parser-json.vala',
	'gpx-playback.vala',
	'gpx-point.vala',
	'gpx-settings.vala',
	'gpx-track.vala',
	'gpx-track-treemodel.vala',
	'gpx-viewer-mapwidget.vala',
	'gpx-viewer-path-layer.vala',
	dependencies: gpx_deps,
)

libgpx_dep = declare_dependency(
    link_with: libgpx,
    include_directories: include_directories('.'),
    dependencies : [ gpx_deps ],
)
