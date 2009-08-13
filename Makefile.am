AUTOMAKE_OPTIONS = 1.11
SUBDIRS=po

##
# GPX Viewer
##
desktopdir = $(datadir)/applications
desktop_in_files = data/gpxviewer.desktop.in
desktop_DATA=$(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@

DISTCLEANFILES=\
   ${desktop_DATA} 

EXTRA_DIST=${desktop_in_files}

##
# Hack to make make distcheck work
##
BUILT_SOURCES=test_data_dir
test_data_dir:
	@-if test ! -d "$(top_builddir)/data"; then \
	mkdir "$(top_builddir)/data"; \
	fi

##
# Interface files
##
uidir= $(pkgdatadir)/
ui_DATA=\
	data/gpx-viewer.ui\
    data/gpx-viewer-file-chooser.ui
EXTRA_DIST+=${ui_DATA}


##
# GPX Viewer program
##

bin_PROGRAMS=gpxviewer

gpxviewer_SOURCES=\
	src/gpx-parser.vala\
    src/gpx-graph.vala\
	src/gpx-viewer.c


gpxviewer_VALAFLAGS= --pkg=config --pkg=gtk+-2.0 --pkg=libxml-2.0 --basedir=$(top_builddir)/src --vapidir=$(top_srcdir)/src -H gpx.h

LIBS=\
	@modules_LIBS@

AM_CFLAGS=\
	@modules_CFLAGS@\
	-DDATA_DIR="\"$(pkgdatadir)\""\
    -std=c99

if MAINTAINER_MODE
AM_CFLAGS+=-Wall
endif
	
EXTRA_DIST+=\
    src/config.vapi

##
# Icons
##
iconthemedir = $(datadir)/icons/hicolor
appicon16dir = $(iconthemedir)/16x16/apps
dist_appicon16_DATA = data/icons/16x16/apps/gpxviewer.png

appicon22dir = $(iconthemedir)/22x22/apps
dist_appicon22_DATA = data/icons/22x22/apps/gpxviewer.png

appicon24dir = $(iconthemedir)/24x24/apps
dist_appicon24_DATA = data/icons/24x24/apps/gpxviewer.png

appicon32dir = $(iconthemedir)/32x32/apps
dist_appicon32_DATA = data/icons/32x32/apps/gpxviewer.png

appiconscalabledir = $(iconthemedir)/scalable/apps
dist_appiconscalable_DATA = data/icons/scalable/apps/gpxviewer.svg

appiconsprivatescalabledir= $(pkgdatadir)/icons/hicolor/scalable/status/
dist_appiconsprivatescalable_DATA = \
    data/icons/scalable/status/pin-blue.svg\
    data/icons/scalable/status/pin-green.svg\
    data/icons/scalable/status/pin-grey.svg\
    data/icons/scalable/status/pin-orange.svg\
    data/icons/scalable/status/pin-purple.svg\
    data/icons/scalable/status/pin-red.svg\
    data/icons/scalable/status/pin-yellow.svg

# Updating the icon cache
gtk_update_icon_cache = gtk-update-icon-cache -f -t $(DESTDIR)$(datadir)/icons/hicolor
install-data-hook: update-icon-cache
uninstall-hook: update-icon-cache
update-icon-cache:
	@-if test -z "$(DESTDIR)"; then \
		echo "Updating Gtk icon cache."; \
		$(gtk_update_icon_cache); \
		else \
		echo "*** Icon cache not updated.  After (un)install, run this:"; \
		echo "***   $(gtk_update_icon_cache)"; \
		fi
