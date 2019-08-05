dnl Check for LIBJSONC compiler flags. On success, set nut_have_libjsonc="yes"
dnl and set LIBJSONC_CFLAGS and LIBJSONC_LIBS. On failure, set
dnl nut_have_libjsonc="no". This macro can be run multiple times, but will
dnl do the checking only once.

AC_DEFUN([NUT_CHECK_LIBJSONC],
[
if test -z "${nut_have_libjsonc_seen}"; then
	nut_have_libjsonc_seen=yes

	dnl save CFLAGS and LIBS
	CFLAGS_ORIG="${CFLAGS}"
	LIBS_ORIG="${LIBS}"

	AC_MSG_CHECKING(for libjson-c cflags)
	AC_ARG_WITH(json-c-includes,
		AS_HELP_STRING([@<:@--with-json-c-includes=CFLAGS@:>@], [include flags for the libjson-c library]),
	[
		case "${withval}" in
		yes|no)
			AC_MSG_ERROR(invalid option --with(out)-json-c-includes - see docs/configure.txt)
			;;
		*)
			CFLAGS="${withval}"
			;;
		esac
	], [])
	AC_MSG_RESULT([${CFLAGS}])

	AC_MSG_CHECKING(for libjson-c libs)
	AC_ARG_WITH(json-c-libs,
		AS_HELP_STRING([@<:@--with-json-c-libs=LIBS@:>@], [linker flags for the libjson-c library]),
	[
		case "${withval}" in
		yes|no)
			AC_MSG_ERROR(invalid option --with(out)-json-c-libs - see docs/configure.txt)
			;;
		*)
			LIBS="${withval}"
			;;
		esac
	], [LIBS="-ljson-c"])
	AC_MSG_RESULT([${LIBS}])

	dnl check if libjson-c is usable
	AC_CHECK_HEADERS(json-c/json.h, [nut_have_libjsonc=yes], [nut_have_libjsonc=no], [AC_INCLUDES_DEFAULT])
	AC_CHECK_FUNCS(json_type_to_name, [], [nut_have_libjsonc=no])

	if test "${nut_have_libjsonc}" = "yes"; then
		LIBJSONC_CFLAGS="${CFLAGS}"
		LIBJSONC_LIBS="${LIBS}"
	fi

	dnl restore original CFLAGS and LIBS
	CFLAGS="${CFLAGS_ORIG}"
	LIBS="${LIBS_ORIG}"

fi
])
