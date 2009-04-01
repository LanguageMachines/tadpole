#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# $Id$
# $URL: $

AC_PREREQ(2.59)
AC_INIT([Tadpole], [0.3], [timbl@uvt.nl])
AM_INIT_AUTOMAKE
AC_CONFIG_SRCDIR([configure.ac])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_HEADER([config.h])

# AC_DEFINE_DIR is defined in acinclude.m4
AC_DEFINE_DIR([BIN_PATH], bindir, bindir)
AC_DEFINE_DIR([SYSCONF_PATH], sysconfdir, sysconfdir)
AC_DEFINE_DIR([LOCALSTATE_PATH], localstatedir, localstatedir)

if test x"${CXXFLAGS+set}" = xset; then
  # the user set CXXFLAGS; don't override it.
  cxx_flags_were_set=true
else
  cxx_flags_were_set=false
fi

# Checks for programs.
# when available, we prefer the Intel Compiler
AC_PROG_CXX( [icpc g++] )

if $cxx_flags_were_set; then
  CXXFLAGS=$CXXFLAGS
fi

# use libtool
AC_PROG_LIBTOOL
AC_SUBST([LIBTOOL_DEPS])

# when running tests, use CXX
AC_LANG([C++])

# First check if CXX is an Intel compiler; 
# those lie and claim to be gcc but are not argument compatible
INTELC=

AC_MSG_CHECKING(whether cxx is an Intel compiler)
AC_TRY_COMPILE([
#ifndef __INTEL_COMPILER
       choke me
#endif
], [],
    AC_MSG_RESULT(yes)
    INTELC=1
    ,
    AC_MSG_RESULT(no)
)

AC_SUBST(INTELC)

# Checks for header files.
AC_CHECK_HEADERS([unistd.h sys/time.h])
AC_CHECK_HEADERS([omp.h] )

doOmp=0
if test "$ac_cv_header_omp_h" = "yes"; then
   if test "$ac_cv_prog_ac_ct_CXX" == "g++"; then
     AC_MSG_NOTICE( [Test for gcc's gomp] )
     oldCXXFLAGS=$CXXFLAGS
     oldLIBS=$LIBS
     CXXFLAGS="$CXXFLAGS -fopenmp"
     LIBS="$LIBS -lgomp"
     AC_SEARCH_LIBS( omp_get_num_threads, [gomp], [doOmp=1] )
     if test "$doOmp" != "1"; then
       CXXFLAGS=$oldCXXFLAGS
       LIBS=$oldLIBS
     fi
   else
     if test "$ac_cv_prog_ac_ct_CXX" == "xlc++"; then
       oldCXXFLAGS=$CXXFLAGS
       oldLDFLAGS=$LDFLAGS
       CXXFLAGS="$CXXFLAGS -qsmp -qsuppress=1500-029"
       LDFLAGS="$LDFLAGS -qsmp"
       AC_SEARCH_LIBS( omp_get_num_threads, [], [doOmp=1] )
       if test "$doOmp" != "1"; then
         CXXFLAGS=$oldCXXFLAGS
         LDFLAGS=$oldLDFLAGS
       fi
     else
       if test "$ac_cv_prog_ac_ct_CXX" == "icpc"; then
         oldCXXFLAGS=$CXXFLAGS
         oldLDFLAGS=$LDFLAGS
         CXXFLAGS="$CXXFLAGS -openmp"
         LDFLAGS="$LDFLAGS -openmp"
         AC_SEARCH_LIBS( omp_get_num_threads, [], [doOmp=1] )
         if test "$doOmp" != "1"; then
           CXXFLAGS=$oldCXXFLAGS
           LDFLAGS=$oldLDFLAGS
         fi
       fi
     fi
   fi
fi

if test "$doOmp" == "1"; then
   AC_MSG_NOTICE( we have Open MP )
else
   AC_MSG_NOTICE( We don't seem top have Open MP )
   LIBS="$LIBS -lpthread"
   CXXFLAGS="$CXXFLAGS -DPTHREADS"
fi

# Checks for typedefs, structures, and compiler characteristics.
AC_HEADER_STDBOOL
AC_C_CONST
AC_TYPE_SIZE_T

# Checks for libraries.

# inspired by feh-1.3.4/configure.ac.  Tnx Tom Gilbert and feh hackers.
AC_ARG_WITH(python,
       [  --with-python=DIR       use python executable installed in <DIR>],
       [PYTHON="$withval/python"],
       [PYTHON="python"])

#we need Python 2.5.x
AM_PATH_PYTHON([2.5],
	       [CXXFLAGS="$CXXFLAGS -I/usr/include/python2.5"
	        LIBS="-L/usr/lib/python2.5 -lpython2.5 $LIBS"
		AC_DEFINE_DIR([PYTHONDIR],pythondir, PYTHONDIR )],
	       [:])

AC_ARG_WITH(timbl,
       [  --with-timbl=DIR       use timbl installed in <DIR>],
       [CXXFLAGS="$CXXFLAGS -I$withval/include"
       LIBS="-L$withval/lib $LIBS"],
       [CXXFLAGS="$CXXFLAGS -I$prefix/include"
       LIBS="-L$prefix/lib $LIBS"] )

AC_ARG_WITH(mbt,
       [  --with-mbt=DIR       use Mbt installed in <DIR>],
       [CXXFLAGS="$CXXFLAGS -I$withval/include"
       LIBS="-L$withval/lib $LIBS"],
       [CXXFLAGS="$CXXFLAGS -I$prefix/include"
       LIBS="-L$prefix/lib $LIBS"] )

AC_CHECK_HEADERS( [Python.h], , AC_MSG_FAILURE("can't find Python C API") )

BOOST_REQUIRE([1.33.1])
BOOST_FILESYSTEM

LIBS="$BOOST_LDFLAGS $BOOST_FILESYSTEM_LIB $LIBS"
CXXFLAGS="$CXXFLAGS $BOOST_CPPFLAGS"

AC_CHECK_ICU([3.6],
	[CXXFLAGS="$CXXFLAGS $ICU_CXXFLAGS"
	LIBS="$ICU_LIBS $LIBS"],
	AC_MSG_FAILURE( "No ICU development environment found. Please check if libicu-dev or the like is installed" ) )

dnl
dnl ----- Sockets
checkBoth=0
AC_CHECK_FUNC(connect)
if test "$ac_cv_func_connect" = "no"; then
    AC_CHECK_LIB(socket, main, LIBS="$LIBS -lsocket", checkBoth=1)
fi
if test "$checkBoth" = "1"; then
    oldLibs=$LIBS
    LIBS="$LIBS -lsocket -lnsl"
    AC_CHECK_FUNC(accept, , [LIBS=$oldLibs])
fi
AC_CHECK_FUNC(gethostbyname, ,[AC_CHECK_LIB(nsl, main, [LIBS="$LIBS -lnsl"])])

dnl ------ socklen_t
dnl We check for socklen_t by making prototypes with the
dnl various types. First socklen_t, then size_t, finally int.
dnl If the prototype succeeds, we are probably safe.
dnl That works if accept is not preprocessor defined (such sa AIX)
AC_MSG_CHECKING([for socklen_t])
AC_CACHE_VAL(ac_cv_check_socklen_t,[ac_cv_check_socklen_t=''
        AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#ifdef __cplusplus
extern "C" {
#endif
#define try 1
#ifdef AIX
#if AIX >= 51
#define try 0
#endif
#endif
#if try
extern int accept(int, struct sockaddr *, socklen_t *);
#endif
#ifdef __cplusplus
}
#endif
        ],,[ac_cv_check_socklen_t=socklen_t],[
                AC_TRY_COMPILE([
#include <sys/types.h>
#include <sys/socket.h>
#ifdef __cplusplus
extern "C" {
#endif
#define try 1
#ifdef AIX
#if AIX >= 42
#define try 0
#endif
#endif
#if try
extern int accept(int, struct sockaddr *, size_t t *);
#endif
#ifdef __cplusplus
}
#endif
],,[ac_cv_check_socklen_t=size_t],[ac_cv_check_socklen_t=int])
                ])
        ])
AC_MSG_RESULT($ac_cv_check_socklen_t)
AC_DEFINE_UNQUOTED(TIMBL_SOCKLEN_T,$ac_cv_check_socklen_t, "sock_len" )

if test "$ac_cv_prog_ac_ct_CXX" == "g++"; then
  have_abi64=0
  oldLibs=$LIBS
  oldCXXFLAGS=$CXXFLAGS
  LIBS="$LIBS -mabi=64 -mips4"
  CXXFLAGS="$CXXFLAGS -mabi=64 -mips4"
  AC_MSG_CHECKING([-mabi=64 compiler flag] )
  AC_COMPILE_IFELSE([AC_LANG_PROGRAM([[const char hw[] = "Hello World\n";]])],
                 [have_abi64=1])
  if test "$have_abi64" == "0"; then
      LIBS=$oldLibs
      CXXFLAGS=$oldCXXFLAGS
      AC_MSG_RESULT( [No] )
  else
      AC_MSG_RESULT( [Ok] )
  fi
  CXXFLAGS="$CXXFLAGS -W -Wall"
else
  CXXFLAGS="$CXXFLAGS -W"
fi

# Checks for library functions.
AC_CHECK_FUNCS([getaddrinfo])

AC_PATH_PROG(pkgconfigpath, pkg-config, NONE)
#check for Timbl and Mbt

if test "$pkgconfigpath" != "NONE"; then
# ugly hack when PKG_CONFIG_PATH isn't defined.
# couldn't get it to work otherwise
  if test "x$PKG_CONFIG_PATH" == x; then
     export PKG_CONFIG_PATH=""
  fi
  AC_ARG_WITH(timbl,
       [  --with-timbl=DIR       use timbl installed in <DIR>],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"])
  PKG_CHECK_MODULES([timbl], [timbl >= 6.1.3] )
  CXXFLAGS="$CXXFLAGS $timbl_CFLAGS"
  LIBS="$LIBS $timbl_LIBS"
  AC_ARG_WITH(mbt,
       [  --with-mbt=DIR       use mbt installed in <DIR>],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$withval/lib/pkgconfig"],
       [PKG_CONFIG_PATH="$PKG_CONFIG_PATH:$prefix/lib/pkgconfig"])
  PKG_CHECK_MODULES([mbt], [mbt >= 3.1.1] )
  CXXFLAGS="$CXXFLAGS $mbt_CFLAGS"
  LIBS="$LIBS $mbt_LIBS"
else
  AC_ARG_WITH(timbl,
       [  --with-timbl=DIR       use timbl installed in <DIR>],
       [CXXFLAGS="$CXXFLAGS -I$withval/include"
        LIBS="-L$withval/lib $LIBS"],
        [CXXFLAGS="$CXXFLAGS -I$prefix/include"
        LIBS="-L$prefix/lib $LIBS"])

  noTimbl=0
  AC_CHECK_LIB([Timbl],timbl_present,,noTimbl=1)
  if test "$noTimbl" =  "1"; then
     AC_MSG_FAILURE( [libTimbl not found, please see usage of --with-timbl] )
  fi
  AC_ARG_WITH(mbt,
       [  --with-mbt=DIR       use mbt installed in <DIR>],
       [CXXFLAGS="$CXXFLAGS -I$withval/include"
        LIBS="-L$withval/lib $LIBS"],
        [CXXFLAGS="$CXXFLAGS -I$prefix/include"
        LIBS="-L$prefix/lib $LIBS"])

  noMbt=0
  AC_CHECK_LIB([Mbt],mbt_present,,noMbt=1)
  if test "$noMbt" =  "1"; then
     AC_MSG_FAILURE( [libMbt not found, please see usage of --with-mbt] )
  fi

fi

AC_OUTPUT([
  Makefile
  src/Makefile
  src/TPtokenize
  config/Makefile
  scripts/Makefile
  scripts/pylet/Makefile
  scripts/pylet/data/Makefile
  scripts/pylet/util/Makefile
  scripts/finalizeParser.sh
  include/Makefile
  include/tadpole/Makefile
])
