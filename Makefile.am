# $Id$
# $URL: $

ACLOCAL_AMFLAGS = -I m4

SUBDIRS = src include config scripts

EXTRA_DIST = bootstrap AUTHORS TODO NEWS test.txt

docdir = $(datadir)/doc/$(PACKAGE)
doc_DATA = ChangeLog

$(top_srcdir)/ChangeLog: $(top_srcdir)/NEWS
	svn update; svn2cl --include-rev
