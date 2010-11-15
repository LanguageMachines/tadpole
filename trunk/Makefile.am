# $Id$
# $URL: $

ACLOCAL_AMFLAGS = -I m4 --install

SUBDIRS = src include config scripts m4

EXTRA_DIST = bootstrap AUTHORS TODO NEWS test.txt

docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = ChangeLog

$(top_srcdir)/ChangeLog: $(top_srcdir)/NEWS
	svn update; svn2cl --include-rev
