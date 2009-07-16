#Define the version
m4_define([package_major_version], [0])
m4_define([package_minor_version], [0])
m4_define([package_micro_version], [7])
m4_define([package_version], [package_major_version.package_minor_version.package_micro_version])

AC_INIT([gpxviewer], [package_version], [qball@sarine.nl])


AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_LIBTOOL

#intltool, libtool. Check for CC compiler
AC_PROG_INTLTOOL([0.21])
ALL_LINGUAS="ar cs"
AM_GNU_GETTEXT_VERSION(0.11.5)
AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=gpxviewer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [GETTEXT Package name])
AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Name of gettext locale directory])



AM_PROG_VALAC([0.7.0])

PKG_PROG_PKG_CONFIG


PKG_CHECK_MODULES([modules],[
        gtk+-2.0  >= 2.16
        champlain-gtk-0.3  >= 0.3.3
        champlain-0.3 >= 0.3.3
        gmodule-2.0 
        libxml-2.0
        ])

AC_OUTPUT([
	Makefile
	po/Makefile.in
	src/Makefile
	data/Makefile
	data/icons/Makefile
	data/icons/16x16/Makefile
	data/icons/16x16/apps/Makefile
	data/icons/22x22/Makefile
	data/icons/22x22/apps/Makefile
	data/icons/24x24/Makefile
	data/icons/24x24/apps/Makefile
	data/icons/32x32/Makefile
	data/icons/32x32/apps/Makefile
	data/icons/scalable/Makefile
	data/icons/scalable/apps/Makefile
	data/icons/scalable/status/Makefile
])
