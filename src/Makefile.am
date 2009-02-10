bin_PROGRAMS = ncmpcpp
ncmpcpp_SOURCES = browser.cpp charset.cpp clock.cpp display.cpp help.cpp \
	helpers.cpp libmpdclient.c lyrics.cpp misc.cpp mpdpp.cpp ncmpcpp.cpp scrollpad.cpp \
	search_engine.cpp settings.cpp song.cpp status_checker.cpp str_pool.c tag_editor.cpp \
	window.cpp

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
ncmpcpp_LDFLAGS = $(all_libraries)
noinst_HEADERS = browser.h charset.h clock.h display.h help.h helpers.h \
	lyrics.h menu.h mpdpp.h scrollpad.h search_engine.h settings.h song.h \
	status_checker.h tag_editor.h window.h
