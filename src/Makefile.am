bin_PROGRAMS=gpxviewer

gpxviewer_SOURCES=\
	gpx-viewer.c\
    speed-plot.c\
	gpx-parser.c

BUILT_SOURCES=\
	gpx-parser.c\
    speed-plot.c

LIBS=\
	@modules_LIBS@

AM_CFLAGS=\
	@modules_CFLAGS@\
	-DDATA_DIR="\"$(pkgdatadir)\""
	
EXTRA_DIST=\
	gpx-parser.vala\
    speed-plot.vala

gpx-parser.c: gpx-parser.vala
	$(VALAC) -C -H gpx-parser.h gpx-parser.vala --pkg=gtk+-2.0 --pkg=libxml-2.0 --pkg=glib-2.0 --library=gpx-parser
# Update time stamps, even if vala didn't find need to rebuild them.
	touch gpx-parser.c gpx-parser.h

speed-plot.c: speed-plot.vala
	$(VALAC) -C -H speed-plot.h speed-plot.vala --pkg=gtk+-2.0 --pkg=libxml-2.0 --pkg=glib-2.0  --pkg=gpx-parser --vapidir=.
# Update time stamps, even if vala didn't find need to rebuild them.
	touch speed-plot.c speed-plot.h
