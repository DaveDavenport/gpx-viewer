libgpx = static_library(
	'gpx',
	'gpx-parser-fit.vala',
	'gpx-convert.vala',
	'gpx-graph.vala',
	'gpx-parser.vala',
	'gpx-parser-xml.vala',
	'gpx-parser-json.vala',
	'gpx-playback.vala',
	'gpx-settings.vala',
	'gpx-track-treemodel.vala',
	'gpx-viewer-map-view.vala',
	'gpx-viewer-path-layer.vala',
	dependencies: gpx_deps,
)

libgpx_dep = declare_dependency(
    link_with: libgpx,
    include_directories: include_directories('.'),
    dependencies : [ gpx_deps ],
)
