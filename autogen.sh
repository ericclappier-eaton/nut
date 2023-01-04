#! /bin/sh
#
# Autoreconf wrapper script to ensure that the source tree is
# in a buildable state
# NOTE: uses cumbersome dumbest-possible shell syntax for extra portability

if [ -n "${PYTHON-}" ] ; then
	# May be a name/path of binary, or one with args - check both
	(command -v "$PYTHON") \
	|| $PYTHON -c "import re,glob,codecs" \
	|| {
		echo "----------------------------------------------------------------------"
		echo "WARNING: Caller-specified PYTHON='$PYTHON' is not available."
		echo "----------------------------------------------------------------------"
		# Do not die just here, we may not need the interpreter
	}
else
	PYTHON=""
	for P in python python3 python2 \
		python-3.10 python3.10 \
		python-3.9 python3.9 \
		python-3.7 python3.7 \
		python-3.5 python3.5 \
		python-3.4 python3.4 \
		python-2.7 python2.7 \
	; do
		if (command -v "$P" >/dev/null) && $P -c "import re,glob,codecs" ; then
			PYTHON="$P"
			break
		fi
	done
fi

rm -f *.in.AUTOGEN_WITHOUT || true

# re-generate files needed by configure, and created otherwise at 'dist' time
if [ ! -f scripts/augeas/nutupsconf.aug.in ]
then
	if [ -n "${PYTHON-}" ] && $PYTHON -c "import re,glob,codecs"; then
		echo "Regenerating Augeas ups.conf lens with '$PYTHON'..."
		(   # That script is templated; assume @PYTHON@ is the only
		    # road-bump there
		    cd scripts/augeas \
		    && $PYTHON ./gen-nutupsconf-aug.py.in
		) || exit 1
	else
		echo "----------------------------------------------------------------------"
		echo "Error: Python is not available."
		echo "Unable to regenerate Augeas lens for ups.conf parsing."
		echo "----------------------------------------------------------------------"
		if [ "${WITHOUT_NUT_AUGEAS-}" = true ]; then
			echo "Proceeding without Augeas integration, be sure to not require it in configure script" >&2
			touch scripts/augeas/nutupsconf.aug.in scripts/augeas/nutupsconf.aug.in.AUTOGEN_WITHOUT
		else
			echo "Aborting $0! To avoid this, please   export WITHOUT_NUT_AUGEAS=true   and re-run" >&2
			echo "or better yet,    export PYTHON=python-x.y   and re-run" >&2
			exit 1
		fi
	fi
fi

if [ ! -f scripts/udev/nut-usbups.rules.in -o \
     ! -f scripts/devd/nut-usb.conf.in ]
then
	if perl -e 1; then
		echo "Regenerating the USB helper files..."
		cd tools && {
			./nut-usbinfo.pl || exit 1
			cd ..
		}
	else
		echo "----------------------------------------------------------------------"
		echo "Error: Perl is not available."
		echo "Unable to regenerate USB helper files."
		echo "----------------------------------------------------------------------"
		if [ "${WITHOUT_NUT_USBINFO-}" = true ]; then
			echo "Proceeding without NUT USB Info, be sure to not require it in configure script" >&2
			touch scripts/udev/nut-usbups.rules.in scripts/udev/nut-usbups.rules.in.AUTOGEN_WITHOUT
			touch scripts/devd/nut-usb.conf.in scripts/devd/nut-usb.conf.in.AUTOGEN_WITHOUT
		else
			echo "Aborting $0! To avoid this, please   export WITHOUT_NUT_USBINFO=true   and re-run" >&2
			exit 1
		fi
	fi
fi

if [ ! -f scripts/systemd/nut-common-tmpfiles.conf.in ]; then
	echo '# autoconf requires this file exists before generating configure script; it will be overwritten by configure during a build' > scripts/systemd/nut-common-tmpfiles.conf.in
fi

# now we can safely call autoreconf
if ( command -v dos2unix ) 2>/dev/null >/dev/null ; then
	if ( dos2unix < configure.ac | cmp - configure.ac ) 2>/dev/null >/dev/null ; then
		:
	else
		echo "WARNING: Did not confirm that configure.ac has Unix EOL markers;"
		echo "this may cause issues for m4 parsing with autotools below."
		if [ -f .git ] || [ -d .git ] ; then
			echo "You may want to enforce that Git uses 'lf' line endings and re-checkout:"
			echo "    :; git config core.autocrlf false && git config core.eol lf && rm .git/index && git checkout -f"
		fi
		echo ""
	fi
fi >&2

echo "Calling autoreconf..."
autoreconf -iv && [ -s configure ] && [ -x configure ] \
|| { cat << EOF
FAILED: did not generate an executable configure script!

# Note: on some systems "autoreconf", "automake" et al are dispatcher
# scripts, and need you to explicitly say which version you want, e.g.
#    export AUTOCONF_VERSION=2.65 AUTOMAKE_VERSION=1.10
# If you get issues with AC_DISABLE_STATIC make sure you have libtool.
EOF
	exit 1
} >&2

# Some autoconf versions may leave "/bin/sh" regardless of CONFIG_SHELL
# which originally was made for "recheck" operations
if [ -n "${CONFIG_SHELL-}" ]; then
	case "${CONFIG_SHELL-}" in
		*/*)	;; # use as is, assume full path
		*)
			ENV_PROG="`command -v env`" 2>/dev/null
			if [ -n "$ENV_PROG" -a -x "$ENV_PROG" ] ; then
				echo "Using '$ENV_PROG' to call unqualified CONFIG_SHELL program name '$CONFIG_SHELL'" >&2
				CONFIG_SHELL="$ENV_PROG $CONFIG_SHELL"
			fi
			;;
	esac

	echo "Injecting caller-provided CONFIG_SHELL='$CONFIG_SHELL' into the script" >&2
	echo "#!${CONFIG_SHELL}" > configure.tmp
	cat configure >> configure.tmp
	# keep the original file rights intact
	cat configure.tmp > configure
	rm configure.tmp
else
	CONFIG_SHELL="`head -1 configure | sed 's,^#!,,'`"
fi

# NOTE: Unquoted, may be multi-token
$CONFIG_SHELL -n configure 2>/dev/null >/dev/null \
|| { echo "FAILED: configure script did not pass shell interpreter syntax checks with $CONFIG_SHELL" >&2 ;
	echo "NOTE: If you are using an older OS release, try executing the script with" >&2
	echo "a more functional shell implementation (dtksh, bash, dash...)" >&2
	echo "You can re-run this script with a CONFIG_SHELL in environment" >&2
	exit 1
}

echo "The generated configure script passed shell interpreter syntax checks"
