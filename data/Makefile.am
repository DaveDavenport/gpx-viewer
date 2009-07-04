SUBDIRS=icons

uidir= $(pkgdatadir)/
ui_DATA=\
	gpx-viewer.ui\
    gpx-viewer-file-chooser.ui

desktopdir = $(datadir)/applications
desktop_in_files = gpxviewer.desktop.in
desktop_DATA=$(desktop_in_files:.desktop.in=.desktop)
@INTLTOOL_DESKTOP_RULE@


EXTRA_DIST=${ui_DATA}\
	${desktop_in_files}

DISTCLEANFILES=\
   ${desktop_DATA} 


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

