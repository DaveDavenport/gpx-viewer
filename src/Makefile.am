bin_PROGRAMS=gpxviewer

gpxviewer_SOURCES=\
	gpx-viewer.c\
    gpx-graph.c\
	gpx-parser.c

BUILT_SOURCES=\
	gpx-parser.vala.stamp\
    gpx-graph.vala.stamp

LIBS=\
	@modules_LIBS@

AM_CFLAGS=\
	@modules_CFLAGS@\
	-DDATA_DIR="\"$(pkgdatadir)\""
	
EXTRA_DIST=\
	gpx-parser.vala\
    gpx-graph.vala

gpx-parser.vala.stamp: gpx-parser.vala
	$(VALAC) -C -H gpx-parser.h $< --pkg=gtk+-2.0 --pkg=libxml-2.0 --pkg=glib-2.0 --library=gpx-parser --basedir=$(top_builddir)/src
# Update time stamps, even if vala didn't find need to rebuild them.
	touch gpx-parser.c gpx-parser.h $@

gpx-graph.vala.stamp: gpx-graph.vala gpx-parser.vala
	$(VALAC) -C -H gpx-graph.h $< --pkg=gtk+-2.0 --pkg=libxml-2.0 --pkg=glib-2.0  --pkg=gpx-parser --vapidir=$(top_builddir)/src --basedir=$(top_builddir)/src
# Update time stamps, even if vala didn't find need to rebuild them.
	touch gpx-graph.c gpx-graph.h $@

CLEANFILES=\
    gpx-parser.vala.stamp\
    gpx-parser.c\
    gpx-parser.h\
    gpx-parser.vapi\
    gpx-graph.vala.stamp\
    gpx-graph.c\
    gpx-graph.h
