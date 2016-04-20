# RELACS_LIBS( [lib1 lib2 ...] ) 
# - Provides --with-relacs option and performs header and link checks for each library
# - Adds library o RELACS_LIBS_(LD|CPP)FLAGS and RELACS_LIBS_LIBS and marks them for substitution
# - Leaves ((LD|CPP)FLAGS|LIBS) untouched
# - defines HAVE_LIBRELACSlib1, HAVE_LIBRELACSlib2, ...

AC_DEFUN([RELACS_LIBS], [

# save flags:
SAVE_CPPFLAGS=${CPPFLAGS}
SAVE_LDFLAGS=${LDFLAGS}
SAVE_LIBS=${LIBS}

# add library:
RELACS_LIBS_CPPFLAGS=
RELACS_LIBS_LDFLAGS=
for RLIB in $1; do
	RELACS_LIBS_LIBS="-lrelacs$RLIB ${RELACS_LIBS_LIBS}"
done

AC_ARG_WITH([relacs],
	[AS_HELP_STRING([--with-relacs=DIR],
	[override RELACS installation path ("/lib" and "/include" is appended)])],
	[RELACS_ERROR="No path given for option --with-relacs"
	if test ${withval} != yes -a "x${withval}" != x ; then
		RELACS_LIBS_CPPFLAGS="-I${withval}/include ${RELACS_LIBS_CPPFLAGS}"
		RELACS_LIBS_LDFLAGS="-L${withval}/lib ${RELACS_LIBS_LDFLAGS}"
	else
		AC_MSG_ERROR(${RELACS_ERROR})
	fi],
	[])

for RLIB in $1; do
	RELACS_INVALID="$RLIB is not a RELACS library. Choose from numerics, daq, options, datafile, widgets, or plot."
	CPPFLAGS="${RELACS_LIBS_CPPFLAGS} ${CPPFLAGS}"
	LDFLAGS="${RELACS_LIBS_LDFLAGS} ${LDFLAGS}"
	RHEADER=""
	AS_IF([test "$RLIB" = "numerics"],[RHEADER="array.h"],
	      [test "$RLIB" = "daq"],[RHEADER="device.h"],
	      [test "$RLIB" = "options"],[RHEADER="options.h"],	
	      [test "$RLIB" = "datafile"],[RHEADER="tablekey.h"],
	      [test "$RLIB" = "widgets"],[RHEADER="optwidget.h"],
	      [test "$RLIB" = "plot"],[RHEADER="plot.h"],
	      [AC_MSG_ERROR([$RELACS_INVALID])])
	AS_IF([test "$RLIB" = "widgets" -o "$RLIB" = "plot"],
	      [CPPFLAGS="${QT4_CPPFLAGS} ${CPPFLAGS}"
	       LDFLAGS="${QT4_LDFLAGS} ${LDFLAGS}"
	       LIBS="${QT4_LIBS} ${LIBS}"])
	RELACS_MISSING="Please install the RELACS $RLIB library"
	AC_CHECK_HEADERS(relacs/$RHEADER,, AC_MSG_ERROR([$RELACS_MISSING]))
	AC_CHECK_LIB(relacs$RLIB, main,, AC_MSG_ERROR([$RELACS_MISSING]))
	LDFLAGS=${SAVE_LDFLAGS}
	CPPFLAGS=${SAVE_CPPFLAGS}
	LIBS=${SAVE_LIBS}
done

# publish:
AC_SUBST(RELACS_LIBS_LDFLAGS)
AC_SUBST(RELACS_LIBS_CPPFLAGS)
AC_SUBST(RELACS_LIBS_LIBS)

])

