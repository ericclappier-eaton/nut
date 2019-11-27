dnl Check for LIBCURL compiler flags. On success, set nut_have_curl="yes"
dnl and set LIBCURL_CFLAGS and LIBCURL_LIBS. On failure, set
dnl nut_have_curl="no". This macro can be run multiple times, but will
dnl do the checking only once.

AC_DEFUN([NUT_CHECK_LIBCURL],
[
if test -z "${nut_have_curl_seen}"; then
	nut_have_curl_seen=yes

	dnl save CFLAGS and LIBS
	CFLAGS_ORIG="${CFLAGS}"
	LIBS_ORIG="${LIBS}"

	nut_defaulted_curl_version=no
	nut_defaulted_curl_cflags=no
	nut_defaulted_curl_libs=no

	dnl See which version of the curl library (if any) is installed
	dnl FIXME : Support detection of cflags/ldflags below by legacy discovery if pkgconfig is not there
	AC_MSG_CHECKING(for libcurl version via pkg-config (0.25.0 minimum required))
	CURL_VERSION="`pkg-config --silence-errors --modversion libcurl 2>/dev/null`"
	if test "$?" != "0" -o -z "${CURL_VERSION}"; then
		nut_defaulted_curl_version=yes
		CURL_VERSION="none"
	fi
	AC_MSG_RESULT(${CURL_VERSION} found)

	if test "${nut_defaulted_curl_version}" = "yes" ; then
		AC_MSG_WARN([could not get pkg-config information for libcurl version, using fallback defaults])
	fi

	AC_MSG_CHECKING(for libcurl cflags)
	AC_ARG_WITH(curl-includes,
		AS_HELP_STRING([@<:@--with-curl-includes=CFLAGS@:>@], [include flags for the curl library]),
	[
		case "${withval}" in
		yes|no)
			AC_MSG_ERROR(invalid option --with(out)-curl-includes - see docs/configure.txt)
			;;
		*)
			CFLAGS="${withval}"
			;;
		esac
	], [CFLAGS="`pkg-config --silence-errors --cflags libcurl 2>/dev/null`"
		if test "$?" != 0 ; then
			nut_defaulted_curl_cflags=yes
			CFLAGS="-I/usr/include"
		fi])
	AC_MSG_RESULT([${CFLAGS}])

	if test "${nut_defaulted_curl_cflags}" = "yes" ; then
		AC_MSG_WARN([could not get pkg-config information for libcurl cflags, using fallback defaults])
	fi

	AC_MSG_CHECKING(for libcurl ldflags)
	AC_ARG_WITH(curl-libs,
		AS_HELP_STRING([@<:@--with-curl-libs=LIBS@:>@], [linker flags for the curl library]),
	[
		case "${withval}" in
		yes|no)
			AC_MSG_ERROR(invalid option --with(out)-curl-libs - see docs/configure.txt)
			;;
		*)
			LIBS="${withval}"
			;;
		esac
	], [LIBS="`pkg-config --silence-errors --libs libcurl 2>/dev/null`"
		if test "$?" != 0 ; then
			nut_defaulted_curl_libs=yes
			LIBS="-lcurl"
		fi])
	AC_MSG_RESULT([${LIBS}])

	if test "${nut_defaulted_curl_libs}" = "yes" ; then
		AC_MSG_WARN([could not get pkg-config information for libcurl libs, using fallback defaults])
	fi

	dnl check if curl is usable
	AC_CHECK_HEADERS(curl/curl.h, [nut_have_curl=yes], [nut_have_curl=no], [AC_INCLUDES_DEFAULT])
	AC_CHECK_FUNCS(curl_global_init, [], [nut_have_curl=no])

	if test "${nut_have_curl}" = "yes"; then
		LIBCURL_CFLAGS="${CFLAGS}"
		LIBCURL_LIBS="${LIBS}"
	fi

    AC_MSG_RESULT(LIBCURL_CFLAGS=${LIBCURL_CFLAGS} LIBCURL_LIBS=${LIBCURL_LIBS})

	dnl restore original CFLAGS and LIBS
	CFLAGS="${CFLAGS_ORIG}"
	LIBS="${LIBS_ORIG}"
fi
])
