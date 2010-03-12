AUTOMAKE_OPTIONS = 1.11
SUBDIRS=po

##
# GPX Viewer
##
desktopdir = $(datadir)/applications
desktop_in_files = data/gpx-viewer.desktop.in
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
    data/gpx-viewer-file-chooser.ui\
    data/gpx-viewer-tracklist.ui\
    data/gpx-viewer-preferences.ui
EXTRA_DIST+=${ui_DATA}


##
# GPX Viewer program
##

bin_PROGRAMS=gpx-viewer

gpx_viewer_SOURCES=\
	src/gpx-parser.vala\
    src/gpx-graph.vala\
    src/gpx-playback.vala\
    src/gpx-track-treemodel.vala\
    src/gpx-preferences.vala\
    src/gpx-viewer-mapwidget.vala\
	src/gpx-viewer.c


gpx_viewer_VALAFLAGS= --pkg=config --pkg=clutter-1.0 --pkg=gtk+-2.0 --pkg=libxml-2.0 --pkg=libchamplain-0.5 --pkg=libchamplain-gtk-0.5 --basedir=$(top_builddir)/src --vapidir=$(top_srcdir)/vapi -H gpx.h

LIBS=\
	@modules_LIBS@\
    -lm

AM_CFLAGS=\
	@modules_CFLAGS@\
	-DDATA_DIR="\"$(pkgdatadir)\""\
    -std=c89

if MAINTAINER_MODE
AM_CFLAGS+=-Wall -Wdeclaration-after-statement -Wshadow -fno-common -Wextra -Wno-unused-parameter -Wno-missing-field-initializers -Wold-style-definition -Wmissing-declarations -Wredundant-decls -Wmissing-noreturn -Wpointer-arith -Wcast-align -Wwrite-strings -Winline -Wnested-externs -Wmissing-format-attribute -Waggregate-return -Wundef -Wmissing-include-dirs -Winit-self -Wswitch-default -Wswitch-enum
endif
	
EXTRA_DIST+=\
    src/config.vapi

##
# Icons
##
iconthemedir = $(datadir)/icons/hicolor
appicon16dir = $(iconthemedir)/16x16/apps
dist_appicon16_DATA = data/icons/16x16/apps/gpx-viewer.png

appicon22dir = $(iconthemedir)/22x22/apps
dist_appicon22_DATA = data/icons/22x22/apps/gpx-viewer.png

appicon24dir = $(iconthemedir)/24x24/apps
dist_appicon24_DATA = data/icons/24x24/apps/gpx-viewer.png

appicon32dir = $(iconthemedir)/32x32/apps
dist_appicon32_DATA = data/icons/32x32/apps/gpx-viewer.png

appicon48dir = $(iconthemedir)/48x48/apps
dist_appicon48_DATA = data/icons/48x48/apps/gpx-viewer.png

appicon256dir = $(iconthemedir)/256x256/apps
dist_appicon256_DATA = data/icons/256x256/apps/gpx-viewer.png

appiconscalabledir = $(iconthemedir)/scalable/apps
dist_appiconscalable_DATA = data/icons/scalable/apps/gpx-viewer.svg

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
