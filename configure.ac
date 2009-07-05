#Define the version
m4_define([package_major_version], [0])
m4_define([package_minor_version], [0])
m4_define([package_micro_version], [5])
m4_define([package_version], [package_major_version.package_minor_version.package_micro_version])

AC_INIT([gpxviewer], [package_version], [qball@sarine.nl])


AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE

AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_LIBTOOL

#intltool, libtool. Check for CC compiler
AC_PROG_INTLTOOL([0.21])

AC_PATH_PROG(VALAC, valac, valac)
AC_SUBST(VALAC)

PKG_PROG_PKG_CONFIG


PKG_CHECK_MODULES([modules],gtk+-2.0 champlain-gtk-0.3 gmodule-2.0 libxml-2.0)

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
