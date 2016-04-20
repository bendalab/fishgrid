# FISHGRID_NIDAQMXBASE() 
# - Provides --with-nidaqmx option and performs header and link checks for each library
# - Adds library to FISHGRID_NIDAQMXBASE_(LD|CPP)FLAGS and FISHGRID_NIDAQMXBASE_LIBS and marks them for substitution
# - Fills FISHGRID_NIDAQMX with (yes|no) for the summary
# - Leaves ((LD|CPP)FLAGS|LIBS) untouched
# - defines HAVE_LIBNIDAQMXBASE_H

AC_DEFUN([FISHGRID_NIDAQMXBASE], [

# save flags:
SAVE_CPPFLAGS=${CPPFLAGS}
SAVE_LDFLAGS=${LDFLAGS}
SAVE_LIBS=${LIBS}

# add library:
FISHGRID_NIDAQMXBASE_CPPFLAGS=
FISHGRID_NIDAQMXBASE_LDFLAGS=
FISHGRID_NIDAQMXBASE_LIBS="-lnidaqmxbase"

AC_ARG_WITH([nidaqmxbase],
	[AS_HELP_STRING([--with-nidaqmxbase=DIR],
		[set NIDAQmxBase installation path ("/lib" and "/include" is appended)])],
	[NIDAQMX_ERROR="No path given for option --with-nidaqmxbase"
	if test ${withval} != yes -a "x${withval}" != x ; then
		FISHGRID_NIDAQMXBASE_CPPFLAGS="-I${withval}/include ${FISHGRID_NIDAQMXBASE_CPPFLAGS}"
		FISHGRID_NIDAQMXBASE_LDFLAGS="-L${withval}/lib ${FISHGRID_NIDAQMXBASE_LDFLAGS}"
	else
		AC_MSG_ERROR(${NIDAQMX_ERROR})
	fi],
	[])

# set flags:
CPPFLAGS="${FISHGRID_NIDAQMXBASE_CPPFLAGS} ${CPPFLAGS}"
LDFLAGS="${FISHGRID_NIDAQMXBASE_LDFLAGS} ${LDFLAGS}"

NIDAQMX_MISSING="NIDAQmxBase not found"
AC_CHECK_HEADERS([NIDAQmxBase.h],
		 [FISHGRID_NIDAQMX=yes],
		 [FISHGRID_NIDAQMX=no
		   AC_MSG_WARN([$NIDAQMX_MISSING])])
AS_IF([test "x$FISHGRID_NIDAQMX" = "xyes" ],
	    [AC_CHECK_LIB([nidaqmxbase], [main],
	    	 	  [FISHGRID_NIDAQMX=yes],
	     		  [FISHGRID_NIDAQMX=no
	      		   AC_MSG_WARN([$NIDAQMX_MISSING])])])

# clear on error:
AS_IF([test "$FISHGRID_NIDAQMX" = "no"],
	[FISHGRID_NIDAQMXBASE_CPPFLAGS=""
	 FISHGRID_NIDAQMXBASE_LDFLAGS=""
	 FISHGRID_NIDAQMXBASE_LIBS=""])

# restore:
LDFLAGS=${SAVE_LDFLAGS}
CPPFLAGS=${SAVE_CPPFLAGS}
LIBS=${SAVE_LIBS}

# publish:
AC_SUBST(FISHGRID_NIDAQMXBASE_LDFLAGS)
AC_SUBST(FISHGRID_NIDAQMXBASE_CPPFLAGS)
AC_SUBST(FISHGRID_NIDAQMXBASE_LIBS)

])

