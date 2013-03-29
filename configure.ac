dnl Process this file with autoconf to produce a configure script.

AC_INIT(barcode,
        m4_esyscmd([build-aux/git-version-gen .tarball-version]),
        bug-barcode@gnu.org)
AC_PREREQ([2.65])

AC_CONFIG_AUX_DIR([build-aux])
AM_INIT_AUTOMAKE([gnu])

AC_CANONICAL_HOST
AC_CANONICAL_TARGET
AC_CANONICAL_BUILD

AC_CONFIG_MACRO_DIR([m4])

AC_CONFIG_HEADERS([config.h])

AC_USE_SYSTEM_EXTENSIONS

AC_PROG_CC
gl_EARLY


AC_PROG_AWK

AC_PROG_INSTALL
AC_CHECK_TOOL(AR, ar)
AC_PROG_LIBTOOL
LT_INIT

AM_GNU_GETTEXT([external],[need-ngettext])
AM_GNU_GETTEXT_VERSION([0.18])

gl_INIT

dnl Check for getopt and libpaper. If getopt is missing, use our version
AC_CHECK_HEADER(paper.h, LIBPAPER=-lpaper, NO_LIBPAPER=-DNO_LIBPAPER)
AC_CHECK_HEADERS(unistd.h)

AC_CHECK_FUNC(strerror, , NO_STRERROR=-DNO_STRERROR)
AC_CHECK_FUNCS(strcasecmp)

AC_SUBST(NO_LIBPAPER)
AC_SUBST(LIBPAPER)
AC_SUBST(NO_STRERROR)
AC_SUBST(GMAKEDEPEND0)
AC_SUBST(GMAKEDEPEND1)
AC_SUBST(GMAKEDEPEND2)

AC_SUBST(MAKEINFO)
AC_SUBST(REMOVEHTMLTAGS)
AC_SUBST(INFOTOHTML)

AC_CONFIG_FILES([Makefile \
                 po/Makefile.in \
                 lib/Makefile \
                 doc/Makefile])

AC_OUTPUT
