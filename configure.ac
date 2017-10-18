dnl Process this file with autoconf to produce a configure script.

AC_PREREQ([2.69])
AC_INIT([klavaro],[3.03],[fefcas@gmail.com])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AM_INIT_AUTOMAKE([-Wall])

AC_CONFIG_MACRO_DIR([m4])
AM_PROG_AR
AM_PROG_LIBTOOL

AC_SEARCH_LIBS([strerror],[cposix])
AC_PROG_CC
AC_PROG_INSTALL
AC_PROG_MAKE_SET
AC_PROG_CXX
AC_PROG_RANLIB
dnl For Windows, comment out this one (then do an "autoreconf"):
IT_PROG_INTLTOOL

AC_HEADER_STDC
AC_CHECK_HEADERS([locale.h stdlib.h string.h unistd.h curl/curl.h])
dnl When GtkDataBox-GTK3 is available, check for it also:
dnl AC_CHECK_HEADERS([locale.h stdlib.h string.h unistd.h curl/curl.h gtkdatabox.h])

m4_define([gtk_required_version], [3.8.0])
m4_define([pango_required_version], [1.16.0])
m4_define([cairo_required_version], [1.4.0])

PKG_CHECK_MODULES(GTK, [gtk+-3.0 >= [gtk_required_version]])
PKG_CHECK_MODULES(GTK, [gdk-3.0 >= [gtk_required_version]])
PKG_CHECK_MODULES(PANGO, [pango >= [pango_required_version]])
PKG_CHECK_MODULES(CAIRO, [cairo >= [cairo_required_version]])
AC_SUBST(GTK_CFLAGS)
AC_SUBST(GTK_LIBS)
dnl For Windows:
dnl PKG_CHECK_MODULES(GTKDATABOX, [gtkdatabox >= 0.9.1.1])

AC_CHECK_LIB([curl], [curl_global_init])
AC_CHECK_LIB([m], [sqrt])
dnl When GtkDataBox-GTK3 is available, check for it:
dnl AC_CHECK_LIB([gtkdatabox], [gtk_databox_new])

AC_FUNC_STRTOD
AC_CHECK_FUNCS([floor pow memmove memset rint setlocale sqrt strcasecmp strchr strdup strerror strrchr strstr strtoul])

GETTEXT_PACKAGE=klavaro
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE,"$GETTEXT_PACKAGE", [Gettext package.])

AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.18.3])

AM_CONDITIONAL(IS_WIN32, test `uname` = "MINGW32_NT-6.1")
AM_CONDITIONAL(IS_POSIX, test  `uname` != "MINGW32_NT-6.1" -a `uname` != "Darwin")

AC_CONFIG_FILES([Makefile
		gtkdatabox/Makefile
		src/Makefile
		po/Makefile.in
		doc/Makefile
		data/Makefile
		data/icons/Makefile
		data/icons/hicolor/Makefile
		data/icons/hicolor/16x16/Makefile
		data/icons/hicolor/16x16/apps/Makefile
		data/icons/hicolor/22x22/Makefile
		data/icons/hicolor/22x22/apps/Makefile
		data/icons/hicolor/24x24/Makefile
		data/icons/hicolor/24x24/apps/Makefile
		data/icons/hicolor/32x32/Makefile
		data/icons/hicolor/32x32/apps/Makefile
		data/icons/hicolor/48x48/Makefile
		data/icons/hicolor/48x48/apps/Makefile
		data/icons/hicolor/64x64/Makefile
		data/icons/hicolor/64x64/apps/Makefile
		data/icons/hicolor/96x96/Makefile
		data/icons/hicolor/96x96/apps/Makefile
])
AC_OUTPUT

