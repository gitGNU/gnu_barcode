# GNU Barcode
#
# Copyright (C) 2010, 2011, 2013 Free Software Foundation, Inc.
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

SUBDIRS = po doc lib

AM_CFLAGS = -Ilib @CFLAGS@ @DEFS@ @NO_LIBPAPER@ @NO_STRERROR@

AM_LDFLAGS = -L. @LIBPAPER@

EXTRA_DIST = barcode.h cmdline.h

bin_PROGRAMS = barcode sample

noinst_LTLIBRARIES = libbarcode.la

ACLOCAL_AMFLAGS = -I m4

libbarcode_la_SOURCES = library.c ean.c code128.c code39.c code93.c i25.c \
		msi.c plessey.c codabar.c \
		ps.c svg.c pcl.c code11.c

barcode_SOURCES = main.c cmdline.c
barcode_LDADD = libbarcode.la lib/libgnu.la


sample_SOURCES = sample.c
sample_LDADD = libbarcode.la
