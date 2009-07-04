bin_PROGRAMS=gpxviewer

gpxviewer_SOURCES=\
	gpx-viewer.c\
    gpx-graph.c\
	gpx-parser.c

BUILT_SOURCES=\
	gpx-parser.c\
    gpx-graph.c

LIBS=\
	@modules_LIBS@

AM_CFLAGS=\
	@modules_CFLAGS@\
	-DDATA_DIR="\"$(pkgdatadir)\""
	
EXTRA_DIST=\
	gpx-parser.vala\
    gpx-graph.vala

gpx-parser.c: gpx-parser.vala
	$(VALAC) -C -H gpx-parser.h gpx-parser.vala --pkg=gtk+-2.0 --pkg=libxml-2.0 --pkg=glib-2.0 --library=gpx-parser
# Update time stamps, even if vala didn't find need to rebuild them.
	touch gpx-parser.c gpx-parser.h

gpx-graph.c: gpx-graph.vala
	$(VALAC) -C -H gpx-graph.h gpx-graph.vala --pkg=gtk+-2.0 --pkg=libxml-2.0 --pkg=glib-2.0  --pkg=gpx-parser --vapidir=.
# Update time stamps, even if vala didn't find need to rebuild them.
	touch gpx-graph.c gpx-graph.h
