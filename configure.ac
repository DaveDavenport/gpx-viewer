#Define the version
m4_define([package_major_version], [0])
m4_define([package_minor_version], [3])
m4_define([package_micro_version], [0])
m4_define([package_version], [package_major_version.package_minor_version.package_micro_version])

AC_INIT([GPX Viewer],[package_version],[qball@sarine.nl],[gpx-viewer],[http://blog.sarine.nl/gpx-viewer/])


AC_CONFIG_HEADER([config.h])
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

AC_PROG_CC
AC_PROG_CC_C_O
LT_INIT
# Remove the check for c++ and fortran compiler
m4_defun([_LT_AC_LANG_CXX_CONFIG], [:])
m4_defun([_LT_AC_LANG_F77_CONFIG], [:])

dnl Checks for libm
AC_CHECK_LIB([m],[floor])


dnl for fixing stuff.
AC_ARG_ENABLE([database-updates],
        AS_HELP_STRING([--disable-database-updates],
			     [disable database updates (useful for distcheck and distribution packagers]))
AM_CONDITIONAL(DATABASE_UPDATES,[test "$enable_database_updates" != "no"])
DISTCHECK_CONFIGURE_FLAGS="${DISTCHECK_CONFIGURE_FLAGS} --disable-database-updates"

AC_SUBST(DISTCHECK_CONFIGURE_FLAGS)


#intltool, libtool. Check for CC compiler
IT_PROG_INTLTOOL([0.21])
ALL_LINGUAS="ar cs de es hu it ru uk zh_CN"
AM_GNU_GETTEXT_VERSION(0.11.5)
AM_GLIB_GNU_GETTEXT
GETTEXT_PACKAGE=gpx-viewer
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE", [GETTEXT Package name])
AC_DEFINE_UNQUOTED(LOCALEDIR, "${prefix}/${DATADIRNAME}/locale", [Name of gettext locale directory])



AM_PROG_VALAC([0.7.0])

PKG_PROG_PKG_CONFIG


PKG_CHECK_MODULES([gtk],[
        gtk+-3.0  >= 3.0
        gmodule-2.0
        ])
PKG_CHECK_MODULES([libxml], [
        libxml-2.0
])
PKG_CHECK_MODULES([libgdl], [
        gdl-3.0
])
PKG_CHECK_MODULES([champlain], [
        champlain-gtk-0.12 >= 0.12.3
        champlain-0.12 >= 0.12.3
])

PKG_CHECK_MODULES([clutter], [
        clutter-gtk-1.0 >= 1.4.0
])



# mime
UPDATE_MIME_DATABASE=update-mime-database
AC_SUBST(UPDATE_MIME_DATABASE)

AC_CONFIG_FILES([
	Makefile
	po/Makefile.in
])
AC_OUTPUT
