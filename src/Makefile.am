bin_PROGRAMS=gpxviewer

gpxviewer_SOURCES=\
	gpx-parser.vala\
    gpx-graph.vala\
	gpx-viewer.c


gpxviewer_VALAFLAGS= --pkg=config --pkg=gtk+-2.0 --pkg=libxml-2.0 --basedir=$(top_builddir)/src --vapidir=$(top_srcdir)/src -H gpx.h

#BUILT_SOURCES=\
#	gpx-parser.vala.stamp\
#    gpx-graph.vala.stamp

LIBS=\
	@modules_LIBS@

AM_CFLAGS=\
	@modules_CFLAGS@\
	-DDATA_DIR="\"$(pkgdatadir)\""\
    -std=c99

#ifdef MAINTAINER_MODE
AM_CFLAGS+=-Wall
#endif
	
EXTRA_DIST=\
    config.vapi

