# Process this file with autoconf to produce a configure script.

AC_INIT(fishgrid, 0.7.0)
m4_ifdef( [AC_CONFIG_MACRO_DIRS],
	  [ AC_CONFIG_MACRO_DIRS([m4]) ],
	  [ AC_CONFIG_MACRO_DIR([m4]) ] )

AM_INIT_AUTOMAKE([1.10 -Wall gnits subdir-objects])

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])

# no static libraries:
AC_ENABLE_SHARED
AC_DISABLE_STATIC
AC_PROG_LIBTOOL

AC_PROG_LN_S
AC_PROG_CXX
AC_LANG(C++)

# default installation directory:
AC_PREFIX_DEFAULT([/usr])

#################################################
## Check for presence of C++ compiler:
#################################################
RELACS_CHECK_CXX()

CXXFLAGS="-Wall $CXXFLAGS"
CFLAGS="-Wall $CFLAGS"

AM_PROG_CC_C_O


# Backup before any additions
CLEAN_CPPFLAGS=${CPPFLAGS}
CLEAN_LDFLAGS=${LDFLAGS}
CLEAN_LIBS=${LIBS}


#################################################
## GNU make
#################################################
AC_RELACS_GNU_MAKE()


#################################################
## pkg-config
#################################################
RELACS_CHECK_PKG_CONFIG()


#################################################
## libc
#################################################
LIBDL_MISSING="Please install libdl, often shipped with the C Runtime Library.
   On a Debian-based system enter 'sudo apt-get install libc6-dev'."
AC_CHECK_LIB(dl, main,, AC_MSG_ERROR(${LIBDL_MISSING}))


#################################################
## libstdc++
#################################################
LIBSTDCPP_MISSING="Please install the C++ Standard Library (libstdc++).
   On a Debian-based system enter 'sudo apt-get install libstdc++6-4.2-dev'."
AC_CHECK_LIB(stdc++, main,, AC_MSG_ERROR(${LIBSTDCPP_MISSING}))


# Restore
CPPFLAGS=${CLEAN_CPPFLAGS}
LDFLAGS=${CLEAN_LDFLAGS}
LIBS=${CLEAN_LIBS}


#################################################
## math
#################################################
RELACS_LIB_MATH()


#################################################
## GSL
#################################################
RELACS_LIB_GSL()


#################################################
## PortAudio
#################################################
#RELACS_LIB_PORTAUDIO()
RELACS_PORTAUDIO="no"
AM_CONDITIONAL(RELACS_COND_PORTAUDIO,[test "$RELACS_PORTAUDIO" != no])

#################################################
## Qt4, moc
#################################################
RELACS_LIB_QT4()


#################################################
## Relacs libraries
#################################################
RELACS_LIBS([numerics options plot widgets])


#################################################
## Comedi
#################################################
RELACS_LIB_COMEDI()
AM_CONDITIONAL(FISHGRID_COND_COMEDI,[test "$RELACS_COMEDI" != no])


#################################################
## USB and DAQFlex
#################################################
RELACS_LIB_USB()
AM_CONDITIONAL(FISHGRID_COND_USB,[test "$RELACS_USB" != no])
RELACS_DAQFLEX=${RELACS_USB}
AM_CONDITIONAL(FISHGRID_COND_DAQFLEX,[test "$RELACS_DAQFLEX" != no])


#################################################
## NIDAQmxBase
#################################################
FISHGRID_NIDAQMXBASE()
AM_CONDITIONAL(FISHGRID_COND_NIDAQMX,[test "$FISHGRID_NIDAQMX" != no])


#################################################
## Doxygen API documentation
#################################################

DOXYGEN_INPUT="doc include"
DOXYGEN_IMAGEPATHES="doc"

AC_SUBST(DOXYGEN_INPUT)
AC_SUBST(DOXYGEN_IMAGEPATHES)

DX_HTML_FEATURE(ON)
DX_CHM_FEATURE(OFF)
DX_CHI_FEATURE(OFF)
DX_MAN_FEATURE(OFF)
DX_RTF_FEATURE(OFF)
DX_XML_FEATURE(OFF)
DX_PDF_FEATURE(OFF)
DX_PS_FEATURE(OFF)
DX_INIT_DOXYGEN( fishgrid, doc/Doxyfile, doc )


#################################################
## FishGrid general
#################################################

# FishGrid build date:
FISHGRIDDATE=`date '+%m/%d/%y'`
AC_SUBST(FISHGRIDDATE)

# FishGrid version and build date as a preprocessor define:
AC_DEFINE_UNQUOTED([FISHGRIDVERSION],["${VERSION} (${FISHGRIDDATE})"])

#################################################
## File substitutions:
#################################################

AC_CONFIG_FILES([
    Makefile
    src/Makefile

    doc/fishgridmain.doc
])


AC_OUTPUT


#################################################
## Summary:
#################################################

cat <<SUMMARY

    Found COMEDI ................. ${RELACS_COMEDI}
    Found DAQFlex (libusb) ....... ${RELACS_DAQFLEX}
    Found NIDAQmxBase ............ ${FISHGRID_NIDAQMX}
    Use GSL ...................... ${RELACS_GSL}
    Generate API documentation ... ${DX_SUMMARY}

If you can read these lines ./configure finished sucessfully.
The FishGrid programs should build and install smoothly.

Continue with ...... \$ make
Run a demo ......... \$ src/fishgrid -3
Install fishgrid ... \$ sudo make install

SUMMARY

#    Found PortAudio .............. ${RELACS_PORTAUDIO}
