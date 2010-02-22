#Define the version
m4_define([package_major_version], [0])
m4_define([package_minor_version], [1])
m4_define([package_micro_version], [2])
m4_define([package_version], [package_major_version.package_minor_version.package_micro_version])

AC_INIT([gpx-viewer], [package_version], [qball@sarine.nl])


AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CC_C_O
AC_PROG_LIBTOOL

#intltool, libtool. Check for CC compiler
AC_PROG_INTLTOOL([0.21])
ALL_LINGUAS="ar cs de es hu it ru uk zh_CN"
AM_GNU_GETTEXT_VERSION(0.11.5)
AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=gpx-viewer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [GETTEXT Package name])
AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Name of gettext locale directory])



AM_PROG_VALAC([0.7.0])

PKG_PROG_PKG_CONFIG


PKG_CHECK_MODULES([modules],[
        gtk+-2.0  >= 2.16
        champlain-gtk-0.5
        champlain-0.5 >= 0.5.0
        gmodule-2.0 
        libxml-2.0
        ])

AC_OUTPUT([
	Makefile
	po/Makefile.in
])
