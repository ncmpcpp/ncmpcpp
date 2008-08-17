bin_PROGRAMS = ncmpcpp
ncmpcpp_SOURCES = color_parser.cpp helpers.cpp libmpdclient.c lyrics.cpp \
	menu.cpp misc.cpp mpdpp.cpp ncmpcpp.cpp scrollpad.cpp settings.cpp song.cpp \
	status_checker.cpp window.cpp

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
ncmpcpp_LDFLAGS = $(all_libraries)
noinst_HEADERS = helpers.h lyrics.h menu.h mpdpp.h scrollpad.h settings.h \
	song.h status_checker.h window.h
