AC_INIT(configure.ac)

AC_CONFIG_HEADERS(config.h)
AM_INIT_AUTOMAKE(ncmpcpp, 0.6_beta5)

AC_PREREQ(2.59)

AC_LANG_CPLUSPLUS
AC_PROG_CXX
AM_PROG_LIBTOOL

AC_ARG_ENABLE(outputs, AS_HELP_STRING([--enable-outputs], [Enable outputs screen @<:@default=no@:>@]), [outputs=$enableval], [outputs=no])
AC_ARG_ENABLE(visualizer, AS_HELP_STRING([--enable-visualizer], [Enable music visualizer screen @<:@default=no@:>@]), [visualizer=$enableval], [visualizer=no])
AC_ARG_ENABLE(clock, AS_HELP_STRING([--enable-clock], [Enable clock screen @<:@default=no@:>@]), [clock=$enableval], [clock=no])
AC_ARG_ENABLE(unicode, AS_HELP_STRING([--enable-unicode], [Enable utf8 support @<:@default=yes@:>@]), [unicode=$enableval], [unicode=yes])
AC_ARG_WITH(curl, AS_HELP_STRING([--with-curl], [Enable fetching lyrics from the Internet @<:@default=auto@:>@]), [curl=$withval], [curl=auto])
AC_ARG_WITH(fftw, AS_HELP_STRING([--with-fftw], [Enable fftw support (required for frequency spectrum vizualization) @<:@default=auto@:>@]), [fftw=$withval], [fftw=auto])
AC_ARG_WITH(pdcurses, AS_HELP_STRING([--with-pdcurses[=LIBNAME]], [Link against pdcurses instead of ncurses @<:@default=XCurses@:>@]), [pdcurses=$withval], [pdcurses=no])
AC_ARG_WITH(taglib, AS_HELP_STRING([--with-taglib], [Enable tag editor @<:@default=auto@:>@]), [taglib=$withval], [taglib=auto])

if test "$outputs" = "yes"; then
	AC_DEFINE([ENABLE_OUTPUTS], [1], [enables outputs screen])
fi

if test "$clock" = "yes"; then
	AC_DEFINE([ENABLE_CLOCK], [1], [enables clock screen])
fi

dnl ================================
dnl = checking for -std=c++0x flag =
dnl ================================
AC_MSG_CHECKING([whether compiler supports -std=c++0x])
old_CXXFLAGS="$CXXFLAGS"
CXXFLAGS="-std=c++0x"
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[ ]])],
	AC_MSG_RESULT([yes])
		std_cpp0x="-std=c++0x",
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support C++0x, please upgrade (GCC >= 4.6)]])
)
CXXFLAGS="$old_CXXFLAGS $std_cpp0x"

dnl ==========================================
dnl = checking for initializer lists support =
dnl ==========================================
AC_MSG_CHECKING([whether compiler supports initializer lists])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <vector>], [[std::vector<int> test = { 1, 2, 3 }; test.push_back(4);]])],
	AC_MSG_RESULT([yes]),
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support initializer lists, please upgrade (GCC >= 4.6)]])
)

dnl =====================================
dnl = checking for auto keyword support =
dnl =====================================
AC_MSG_CHECKING([whether compiler supports auto keyword])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([], [[auto test = new int; *test = 1;]])],
	AC_MSG_RESULT([yes]),
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support auto keyword, please upgrade (GCC >= 4.6)]])
)

dnl =========================================
dnl = checking for lambda functions support =
dnl =========================================
AC_MSG_CHECKING([whether compiler supports lambda functions])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <functional>], [[int a = 5; std::function<int(int)> f = [&a](int b) { return a + b; }; f(8);]])],
	AC_MSG_RESULT([yes]),
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support lambda functions, please upgrade (GCC >= 4.6)]])
)

dnl ================================================================
dnl = checking whether calling derived member function of object   =
dnl = passed to lambda without explicit 'this' usage works         =
dnl ================================================================
AC_MSG_CHECKING([whether calling derived member function passed to lambda without explicit this usage works ])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([#include <functional>], [[struct A { void foo() { } };  struct B : public A { void bar() { std::function<void()> f = [this]() { foo(); }; f();  } } foo; foo.bar();]])],
	AC_MSG_RESULT([yes]),
	AC_MSG_RESULT([no])
		AC_MSG_ERROR([[Your compiler doesn't seem to support calling derived member function of an object passed to lambda as 'this' without explicit 'this' usage, please upgrade (GCC >= 4.6)]])
)

dnl =========================================
dnl = checking for override keyword support =
dnl =========================================
AH_TEMPLATE([OVERRIDE], [override keyword used in C++11])
AC_MSG_CHECKING([whether compiler supports override keyword])
AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[struct A { virtual void foo() { } }; struct B : public A { virtual void foo() override { } };]], [])],
	AC_MSG_RESULT([yes])
		AC_DEFINE([OVERRIDE], [override]),
	AC_MSG_RESULT([no])
		AC_DEFINE([OVERRIDE], []),
)

dnl =============================
dnl = setting boost environment =
dnl =============================
AS_IF([test -z "${BOOST_LIB_SUFFIX+x}"], [BOOST_LIB_SUFFIX=-mt])
AC_ARG_VAR([BOOST_LIB_SUFFIX], [Boost library name suffix [default=-mt]])

dnl ======================================
dnl = checking for various boost headers =
dnl ======================================
AC_CHECK_HEADERS([boost/bind.hpp], ,
	AC_MSG_ERROR(boost/bind.hpp is missing))
AC_CHECK_HEADERS([boost/lexical_cast.hpp], ,
	AC_MSG_ERROR(boost/lexical_cast.hpp is missing))
AC_CHECK_HEADERS([boost/algorithm/string.hpp], ,
	AC_MSG_ERROR(boost/algorithm/string.hpp is missing))

dnl =================================
dnl = checking for boost.filesystem =
dnl =================================
AC_CHECK_HEADERS([boost/filesystem.hpp], ,
	AC_MSG_ERROR(boost/filesystem.hpp is missing)
)
AC_CHECK_LIB(boost_filesystem$BOOST_LIB_SUFFIX, main, LDFLAGS="$LDFLAGS -lboost_filesystem$BOOST_LIB_SUFFIX",
	AC_MSG_ERROR([no boost.filesystem library found])
)
AC_CHECK_LIB(boost_system$BOOST_LIB_SUFFIX, main, LDFLAGS="$LDFLAGS -lboost_system$BOOST_LIB_SUFFIX",
	AC_MSG_ERROR([no boost.system library found])
)

dnl =============================
dnl = checking for boost.format =
dnl =============================
AC_CHECK_HEADERS([boost/format.hpp], ,
	AC_MSG_ERROR(boost/format.hpp is missing)
)

dnl =============================
dnl = checking for boost.locale =
dnl =============================
AC_CHECK_HEADERS([boost/locale.hpp], ,
	AC_MSG_ERROR(boost/locale.hpp is missing)
)
AC_CHECK_LIB(boost_locale$BOOST_LIB_SUFFIX, main, LDFLAGS="$LDFLAGS -lboost_locale$BOOST_LIB_SUFFIX",
	AC_MSG_ERROR([no boost.locale library found])
)

dnl =================================
dnl = checking for boost.posix_time =
dnl =================================
AC_CHECK_HEADERS([boost/date_time/posix_time/posix_time.hpp], ,
	AC_MSG_ERROR(boost/date_time/posix_time/posix_time.hpp is missing)
)

dnl ======================================
dnl = checking for boost.program_options =
dnl ======================================
AC_CHECK_HEADERS([boost/program_options.hpp], ,
	AC_MSG_ERROR(boost/program_options.hpp is missing)
)
AC_CHECK_LIB(boost_program_options$BOOST_LIB_SUFFIX, main, LDFLAGS="$LDFLAGS -lboost_program_options$BOOST_LIB_SUFFIX",
	AC_MSG_ERROR([no boost.program_options library found])
)

dnl ============================
dnl = checking for boost.regex =
dnl ============================
AC_CHECK_HEADERS([boost/regex.hpp], ,
	AC_MSG_ERROR(boost/regex.hpp is missing)
)
AC_CHECK_LIB(boost_regex$BOOST_LIB_SUFFIX, main, LDFLAGS="$LDFLAGS -lboost_regex$BOOST_LIB_SUFFIX",
	AC_MSG_ERROR([no boost.regex library found])
)

dnl =============================
dnl = checking for boost.thread =
dnl =============================
AC_DEFINE([BOOST_THREAD_VERSION], [3], [require boost.thread v3])
AC_CHECK_HEADERS([boost/thread.hpp], ,
	AC_MSG_ERROR([boost/thread.hpp is missing or your boost version is too old (boost.thread v3 is required)])
)
AC_CHECK_LIB(boost_thread$BOOST_LIB_SUFFIX, main, LDFLAGS="$LDFLAGS -lboost_thread$BOOST_LIB_SUFFIX",
	AC_MSG_ERROR([no boost.thread library found])
)

dnl ==============================
dnl = checking for regex (win32) =
dnl ==============================
AC_CHECK_LIB(regex, regcomp, LDFLAGS="$LDFLAGS -lregex", )

dnl ================================
dnl = checking for various headers =
dnl ================================
AC_CHECK_HEADERS([netinet/tcp.h netinet/in.h], , AC_MSG_ERROR(vital headers missing))
AC_CHECK_HEADERS([langinfo.h], , AC_MSG_WARN(locale detection disabled))

dnl ==============================
dnl = checking for libmpdclient2 =
dnl ==============================
PKG_CHECK_MODULES([libmpdclient], [libmpdclient >= 2.8], [
	AC_SUBST(libmpdclient_LIBS)
	AC_SUBST(libmpdclient_CFLAGS)
	CPPFLAGS="$CPPFLAGS $libmpdclient_CFLAGS"
	AC_CHECK_HEADERS([mpd/client.h],
		LDFLAGS="$LDFLAGS $libmpdclient_LIBS"
	,
		AC_MSG_ERROR([missing mpd/client.h header])
	)
],
	AC_MSG_ERROR([libmpdclient >= 2.8 is required!])
)

dnl =========================
dnl = checking for readline =
dnl =========================
AC_CHECK_HEADERS([readline/readline.h readline/history.h],
	AC_CHECK_LIB(readline, rl_initialize, LDFLAGS="$LDFLAGS -lreadline",
		AC_MSG_ERROR([readline headers found but there is no readline library to make use of])
	),
)


dnl ========================
dnl = checking for pthread =
dnl ========================
AC_CHECK_HEADERS([pthread.h],
	AC_CHECK_LIB(pthread, pthread_create, LDFLAGS="$LDFLAGS -lpthread",
		AC_MSG_ERROR([pthread.h found but there is no pthread library to make use of])
	),
)

dnl ========================
dnl = checking for ncurses =
dnl ========================
if test "$pdcurses" = "no" ; then
	if test "$unicode" = "yes" ; then
		curses_config_bin="ncursesw6-config ncursesw5-config"
		AC_DEFINE([NCMPCPP_UNICODE], [1], [enables unicode support])
	else
		curses_config_bin="ncurses6-config ncurses5-config"
	fi
else
	if test "$pdcurses" = "yes" ; then
		pdcurses_lib=XCurses
		curses_config_bin=xcurses-config
	else
		pdcurses_lib=$pdcurses
	fi
	AC_DEFINE([USE_PDCURSES], [1], [enables pdcurses support])
fi
AC_PATH_PROGS(CURSES_CONFIG, $curses_config_bin)
if test "$CURSES_CONFIG" != "" ; then
	CPPFLAGS="$CPPFLAGS `$CURSES_CONFIG --cflags`"
	LDFLAGS="$LDFLAGS `$CURSES_CONFIG --libs`"
fi
if test "$pdcurses" = "no" ; then
	AC_CHECK_LIB(ncursesw, initscr,
		curses_lib=ncursesw,
		curses_lib=ncurses
	)
else
	curses_lib=$pdcurses_lib
fi
AC_CHECK_LIB($curses_lib, initscr,
	if test "$CURSES_CONFIG" = "" ; then
		LDFLAGS="$LDFLAGS -l$curses_lib"
	fi
	,
	AC_MSG_ERROR([$curses_lib library is required])
	)
if test "$pdcurses" != "no" ; then
	AC_CHECK_LIB($curses_lib, Xinitscr, AC_DEFINE([XCURSES], [1], [x11 pdcurses available]), )
fi
AC_CHECK_HEADERS([curses.h], , AC_MSG_ERROR([missing curses.h header]))

dnl ======================
dnl = checking for fftw3 =
dnl ======================
if test "$visualizer" = "yes" ; then
	if test "$fftw" != "no" ; then
		PKG_CHECK_MODULES([fftw3], [fftw3 >= 3], [
			AC_SUBST(fftw3_LIBS)
			AC_SUBST(fftw3_CFLAGS)
			CPPFLAGS="$CPPFLAGS $fftw3_CFLAGS"
			AC_CHECK_HEADERS([fftw3.h],
				LDFLAGS="$LDFLAGS $fftw3_LIBS"
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

dnl =====================
dnl = checking for curl =
dnl =====================
if test "$curl" != "no" ; then
	AC_PATH_PROG(CURL_CONFIG, curl-config)
	if test "$CURL_CONFIG" != "" ; then
		CPPFLAGS="$CPPFLAGS `$CURL_CONFIG --cflags`"
		AC_CHECK_LIB(curl, curl_easy_init, LDFLAGS="$LDFLAGS `$CURL_CONFIG --libs`",
			if test "$curl" = "yes" ; then
				AC_MSG_ERROR([curl library is required])
			fi
		)
		AC_CHECK_HEADERS([curl/curl.h], ,
			if test "$curl" = "yes" ; then
				AC_MSG_ERROR([missing curl.h header])
			fi
		)
	else
		AC_CHECK_LIB(curl, curl_easy_init, LDFLAGS="$LDFLAGS -lcurl",
			if test "$curl" = "yes" ; then
				AC_MSG_ERROR([curl library is required])
			fi
		)
	fi
fi

dnl =======================
dnl = checking for taglib =
dnl =======================
if test "$taglib" != "no" ; then
	AC_PATH_PROG(TAGLIB_CONFIG, taglib-config)
	if test "$TAGLIB_CONFIG" != "" ; then
		CPPFLAGS="$CPPFLAGS `$TAGLIB_CONFIG --cflags`"
		LDFLAGS="$LDFLAGS `$TAGLIB_CONFIG --libs`"
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
