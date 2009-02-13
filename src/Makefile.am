bin_PROGRAMS = ncmpcpp
ncmpcpp_SOURCES = browser.cpp charset.cpp clock.cpp display.cpp help.cpp \
	helpers.cpp info.cpp libmpdclient.c lyrics.cpp media_library.cpp misc.cpp mpdpp.cpp \
	ncmpcpp.cpp playlist.cpp playlist_editor.cpp scrollpad.cpp search_engine.cpp \
	settings.cpp song.cpp status_checker.cpp str_pool.c tag_editor.cpp window.cpp

# set the include path found by configure
INCLUDES= $(all_includes)

# the library search path.
ncmpcpp_LDFLAGS = $(all_libraries)
noinst_HEADERS = browser.h charset.h clock.h display.h global.h help.h \
	helpers.h info.h lyrics.h media_library.h menu.h mpdpp.h playlist_editor.h \
	scrollpad.h search_engine.h settings.h song.h status_checker.h tag_editor.h \
	window.h
