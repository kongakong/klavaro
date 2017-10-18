## Process this file with automake to produce Makefile.in

bin_PROGRAMS = klavaro

klavaro_SOURCES = \
	main.c main.h\
	auxiliar.c auxiliar.h \
	callbacks.c callbacks.h \
	translation.c translation.h \
	keyboard.c keyboard.h \
	tutor.c tutor.h \
	cursor.c cursor.h \
	plot.c plot.h \
	basic.c basic.h \
	adaptability.c adaptability.h \
	velocity.c velocity.h \
	fluidness.c fluidness.h \
	accuracy.c accuracy.h \
	top10.c top10.h 

AM_CPPFLAGS = @GTK_CFLAGS@ \
	      -DPACKAGE_DATA_DIR=\""$(datadir)"\" \
	      -DPACKAGE_LOCALE_DIR=\""$(prefix)/$(DATADIRNAME)/locale"\"

AM_CFLAGS = -Wextra \
	    -I$(top_srcdir)/gtkdatabox

AM_LDFLAGS = -static

klavaro_LDADD = @GTK_LIBS@ \
		-lgmodule-2.0 \
	       	$(top_srcdir)/gtkdatabox/libgtkdataboks.la

if IS_POSIX
AM_CFLAGS += -export-dynamic
endif

if IS_WIN32
klavaro_LDADD += -lcurldll
AM_CFLAGS += -export-all-symbols
endif

