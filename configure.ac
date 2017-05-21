AC_INIT([ncmpcpp], [0.8])
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE([subdir-objects])

AC_CONFIG_MACRO_DIR([m4])

AC_PREREQ(2.59)

AC_LANG_CPLUSPLUS
AC_PROG_CXX
AC_PROG_LIBTOOL

AC_ARG_ENABLE(outputs, AS_HELP_STRING([--enable-outputs], [Enable outputs screen @<:@default=no@:>@]), [outputs=$enableval], [outputs=no])
AC_ARG_ENABLE(visualizer, AS_HELP_STRING([--enable-visualizer], [Enable music visualizer screen @<:@default=no@:>@]), [visualizer=$enableval], [visualizer=no])
AC_ARG_ENABLE(clock, AS_HELP_STRING([--enable-clock], [Enable clock screen @<:@default=no@:>@]), [clock=$enableval], [clock=no])

AC_ARG_WITH(fftw, AS_HELP_STRING([--with-fftw], [Enable fftw support (required for frequency spectrum vizualization) @<:@default=auto@:>@]), [fftw=$withval], [fftw=auto])
AC_ARG_WITH(taglib, AS_HELP_STRING([--with-taglib], [Enable tag editor @<:@default=auto@:>@]), [taglib=$withval], [taglib=auto])

if test "$outputs" = "yes"; then
	AC_DEFINE([ENABLE_OUTPUTS], [1], [enables outputs screen])
fi

if test "$clock" = "yes"; then
	AC_DEFINE([ENABLE_CLOCK], [1], [enables clock screen])
fi

# -std=c++14
AC_MSG_CHECKING([whether compiler supports -std=c++14])
old_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="-std=c++14"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]])],
	AC_MSG_RESULT([yes])
		std_cpp14="-std=c++14",
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support C++14, please upgrade (GCC >= 5)]])
)
CXXFLAGS="$old_CXXFLAGS $std_cpp14"

# generic lambda expressions support
AC_MSG_CHECKING([whether compiler supports generic lambda expressions])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[auto f = [](auto n) { return n*n; }; f(7);]])],
	AC_MSG_RESULT([yes]),
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support generic lambda expressions, please upgrade (GCC >= 5)]])
)

# boost
BOOST_REQUIRE([1.60])
AC_SUBST(BOOST_CPPFLAGS)
CPPFLAGS="$CPPFLAGS $BOOST_CPPFLAGS"

BOOST_STRING_ALGO
BOOST_FORMAT
BOOST_HASH
BOOST_CONVERSION
BOOST_MATH
BOOST_OPTIONAL
BOOST_RANGE
BOOST_TOKENIZER
BOOST_TUPLE
BOOST_VARIANT

BOOST_DATE_TIME
AC_SUBST(BOOST_DATE_TIME_LDFLAGS)
AC_SUBST(BOOST_DATE_TIME_LIBS)
LDFLAGS="$LDFLAGS $BOOST_DATE_TIME_LDFLAGS"
LIBS="$LIBS $BOOST_DATE_TIME_LIBS"

BOOST_FILESYSTEM
AC_SUBST(BOOST_FILESYSTEM_LDFLAGS)
AC_SUBST(BOOST_FILESYSTEM_LIBS)
LDFLAGS="$LDFLAGS $BOOST_FILESYSTEM_LDFLAGS"
LIBS="$LIBS $BOOST_FILESYSTEM_LIBS"

BOOST_LOCALE
AC_SUBST(BOOST_LOCALE_LDFLAGS)
AC_SUBST(BOOST_LOCALE_LIBS)
LDFLAGS="$LDFLAGS $BOOST_LOCALE_LDFLAGS"
LIBS="$LIBS $BOOST_LOCALE_LIBS"

BOOST_PROGRAM_OPTIONS
AC_SUBST(BOOST_PROGRAM_OPTIONS_LDFLAGS)
AC_SUBST(BOOST_PROGRAM_OPTIONS_LIBS)
LDFLAGS="$LDFLAGS $BOOST_PROGRAM_OPTIONS_LDFLAGS"
LIBS="$LIBS $BOOST_PROGRAM_OPTIONS_LIBS"

BOOST_REGEX
AC_SUBST(BOOST_REGEX_LDFLAGS)
AC_SUBST(BOOST_REGEX_LIBS)
LDFLAGS="$LDFLAGS $BOOST_REGEX_LDFLAGS"
LIBS="$LIBS $BOOST_REGEX_LIBS"

BOOST_THREAD
AC_SUBST(BOOST_THREAD_LDFLAGS)
AC_SUBST(BOOST_THREAD_LIBS)
LDFLAGS+="$LDFLAGS $BOOST_THREAD_LDFLAGS"
LIBS="$LIBS $BOOST_THREAD_LIBS"

# icu
AH_TEMPLATE([BOOST_REGEX_ICU], [boost.regex was compiled with ICU support])
PKG_CHECK_MODULES([ICU], [icu-i18n icu-uc], [
	old_CPPFLAGS="$CPPFLAGS"
	old_LIBS="$LIBS"
	AC_SUBST(ICU_CFLAGS)
	AC_SUBST(ICU_LIBS)
	CPPFLAGS="$CPPFLAGS $ICU_CFLAGS"
	LIBS="$LIBS $ICU_LIBS"
	AC_MSG_CHECKING([whether boost.regex was compiled with ICU support])
	AC_LINK_IFELSE([AC_LANG_PROGRAM([
    #include <boost/regex/icu.hpp>
    ], [boost::u32regex rx = boost::make_u32regex("foo"); if (boost::u32regex_search("foobar", rx)) { }])],
	AC_MSG_RESULT([yes])
		AC_DEFINE([BOOST_REGEX_ICU], [1]),
	AC_MSG_RESULT([no])
		CPPFLAGS="$old_CPPFLAGS"
		LIBS="$old_LIBS",
	)
], [[]])

# various headers
AC_CHECK_HEADERS([netinet/tcp.h netinet/in.h], , AC_MSG_ERROR(vital headers missing))
AC_CHECK_HEADERS([langinfo.h], , AC_MSG_WARN(locale detection disabled))

# libmpdclient2
PKG_CHECK_MODULES([libmpdclient], [libmpdclient >= 2.8], [
	AC_SUBST(libmpdclient_CFLAGS)
	AC_SUBST(libmpdclient_LIBS)
	CPPFLAGS="$CPPFLAGS $libmpdclient_CFLAGS"
	AC_CHECK_HEADERS([mpd/client.h],
		LIBS="$LIBS $libmpdclient_LIBS"
	,
		AC_MSG_ERROR([missing mpd/client.h header])
	)
],
	AC_MSG_ERROR([libmpdclient >= 2.8 is required!])
)

# readline
AX_LIB_READLINE
if test "$ax_cv_lib_readline" = "no"; then
  AC_MSG_ERROR([no readline compatible library found])
fi
if test "$ax_cv_lib_readline_history" = "no"; then
  AC_MSG_WARN([readline library has no history functionality])
fi

# pthread
AC_CHECK_LIB(pthread, pthread_create, LIBS="$LIBS -lpthread",
	AC_MSG_ERROR([pthread library is required])
)

# ncursesw
PKG_CHECK_MODULES([ncursesw], [ncursesw], [
	AC_SUBST(ncursesw_CFLAGS)
	AC_SUBST(ncursesw_LIBS)
	CPPFLAGS="$CPPFLAGS $ncursesw_CFLAGS"
	AC_CHECK_HEADERS([curses.h],
		LIBS="$LIBS $ncursesw_LIBS"
	,
		AC_MSG_ERROR([missing curses.h header])
	)
  AC_CHECK_LIB(ncursesw, initscr, , AC_MSG_ERROR([ncursesw doesn't provide initscr]))
  AC_MSG_CHECKING([whether curses.h exposes wide-char functions by default])
  AC_LINK_IFELSE([AC_LANG_PROGRAM([
    #include <curses.h>
    ], [waddnwstr(stdscr, L"123", 3)])],
	AC_MSG_RESULT([yes]),
	AC_MSG_RESULT([no])
		CPPFLAGS="$CPPFLAGS -D_XOPEN_SOURCE_EXTENDED",
	)
],
	AC_MSG_ERROR([ncursesw is required!])
)

# fftw3
if test "$visualizer" = "yes" ; then
	if test "$fftw" != "no" ; then
		PKG_CHECK_MODULES([fftw3], [fftw3 >= 3], [
			AC_SUBST(fftw3_LIBS)
			AC_SUBST(fftw3_CFLAGS)
			CPPFLAGS="$CPPFLAGS $fftw3_CFLAGS"
			AC_CHECK_HEADERS([fftw3.h],
				LIBS="$LIBS $fftw3_LIBS"
			,
				if test "$fftw" = "yes" ; then
					AC_MSG_ERROR([missing fftw3.h header])
				fi
			)
		],
			if test "$fftw" = "yes" ; then
				AC_MSG_ERROR([fftw3 library is required!])
			fi
		)
	fi
	AC_DEFINE([ENABLE_VISUALIZER], [1], [enables music visualizer screen])
fi

# libcurl
PKG_CHECK_MODULES([libcurl], [libcurl], [
	AC_SUBST(libcurl_CFLAGS)
	AC_SUBST(libcurl_LIBS)
	CPPFLAGS="$CPPFLAGS $libcurl_CFLAGS"
	AC_CHECK_HEADERS([curl/curl.h],
		LIBS="$LIBS $libcurl_LIBS"
	,
		AC_MSG_ERROR([missing curl/curl.h header])
	)
],
	AC_MSG_ERROR([libcurl is required!])
)

# taglib
if test "$taglib" != "no" ; then
	AC_PATH_PROG(TAGLIB_CONFIG, taglib-config)
	if test "$TAGLIB_CONFIG" != "" ; then
		CPPFLAGS="$CPPFLAGS `$TAGLIB_CONFIG --cflags`"
		LIBS="$LIBS `$TAGLIB_CONFIG --libs`"
		AC_CHECK_HEADERS([taglib.h], ,
			if test "$taglib" = "yes" ; then
				AC_MSG_ERROR([missing taglib.h header])
			fi
		)
	else
		if test "$taglib" = "yes" ; then
			AC_MSG_ERROR([taglib-config executable is missing])
		fi
	fi
fi

AC_CONFIG_FILES([Makefile src/Makefile doc/Makefile])
AC_OUTPUT
