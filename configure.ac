# Process this file with autoconf to produce a configure script.
# TODO: Unknown whether 2.64 works; was a bit of a milestone release, though
AC_PREREQ([2.64])
# TODO: required versions for automake, libtool?

AC_INIT([fsrip], [0.0], [BUG-REPORT-ADDRESS])

# we need realpath to expand all relative paths to absolute paths
# in our substitution variables
AC_CHECK_PROG([has_realpath], [realpath], [yes], [no])
if test x"$has_realpath" != x"yes" ; then
  AC_MSG_ERROR([Please install 'realpath' and ensure it is accessible on the path.])
fi

# Set the default C and C++ compilation flags to nothing.
# Do this immediately after AC_INIT, as other macros might invoke
# AC_PROG_CC or AC_PROG_CXX and stomp our defaults.
: ${CFLAGS=''}
: ${CXXFLAGS=''}

## FIXME: Argh, there appears to be no way to distinguish between the
## flags set by mingw64-configure and ones set by the user...
#AC_MSG_NOTICE([Default CPPFLAGS: $CPPFLAGS])
#AC_MSG_NOTICE([Default CFLAGS:   $CFLAGS])
#AC_MSG_NOTICE([Default CXXFLAGS: $CXXFLAGS])
#AC_MSG_NOTICE([Default LDFLAGS:  $LDFLAGS])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src/cmd/main.cpp])
AC_CONFIG_HEADERS([config.h])

AM_INIT_AUTOMAKE([foreign])

#
# common
#
AC_PROG_CC
AC_PROG_CXX

AC_LANG([C])
AX_APPEND_COMPILE_FLAGS([-g -std=c1x -W -Wall -Wextra -pedantic -pipe -O3], [X_CFLAGS])

AC_LANG([C++])
CXXFLAGS_saved="$CXXFLAGS"
AX_CXX_COMPILE_STDCXX_11([noext], [mandatory])
# discover the C++11 flag, which we need later
CXX11_FLAG=`echo $CXXFLAGS | sed -e 's/.* //'`
CXXFLAGS="$CXXFLAGS_saved"
AX_APPEND_COMPILE_FLAGS([-g -W -Wall -Wextra -Wnon-virtual-dtor -Wno-unused-local-typedefs -pedantic -pipe -O3 $CXX11_FLAG], [X_CXXFLAGS])

AC_PROG_INSTALL

# FIXME: mingw defaults to -O2, along with a bunch of other flags which
# we want to keep.
case "$host" in
*-*-mingw*)
  LG_REMOVE_FLAGS([CFLAGS], ['-O2'])
  LG_REMOVE_FLAGS([CXXFLAGS], ['-O2'])
  ;;
esac

# pkg-config doesn't look in /usr/local/lib on some systems
export PKG_CONFIG_PATH=/usr/local/lib/pkgconfig





###########################################################################
#                                                                         #
#                              Dependencies                               #
#                                                                         #
###########################################################################
#
# Boost headers
#
if test "x$with_boost" = "xno"; then
  AC_MSG_ERROR([--without-boost specified, but Boost is mandatory.])
else
  case "$host" in
  *-*-mingw*)
    # AX_BOOST_BASE doesn't find the Boost libs for mingw, we help it out
    if test "$with_boost_libdir" = ""; then
      with_boost_libdir="/usr/${host}/sys-root/mingw/lib"
      AC_MSG_NOTICE([--with-boost-libdir not set. We are guessing ${with_boost_libdir}.])
    fi
    ;;
  esac

  AX_BOOST_BASE([1.49.0],
    [],
    [AC_MSG_ERROR([Failed to find usable Boost headers.])])
fi

#
# Boost libs
#
# AX_BOOST_FILESYSTEM
# if test "x$ax_cv_boost_filesystem" != "xyes"; then
#   AC_MSG_ERROR([Failed to find Boost filesystem library.])
# fi

# AX_BOOST_PROGRAM_OPTIONS
# if test "x$ax_cv_boost_program_options" != "xyes"; then
#   AC_MSG_ERROR([Failed to find Boost program_options library.])
# fi

# AX_BOOST_SYSTEM
# if test "x$ax_cv_boost_system" != "xyes"; then
#   AC_MSG_ERROR([Failed to find Boost system library.])
# fi

# AX_BOOST_ASIO

# case "$host" in
# *-*-mingw*)
#   # Boost ASIO needs ws2_32 and mswsock on Windows
#   BOOST_ASIO_LIB="-lws2_32 -lmswsock"
#   AC_SUBST([BOOST_ASIO_LIB])

#   # FIXME: wrong boost_system lib gets detected!
#   BOOST_SYSTEM_LIB=`echo "$BOOST_SYSTEM_LIB" | sed 's/.dll/-mt/'`
#   # FIXME: wrong boost_filesystem lib gets detected!
#   BOOST_FILESYSTEM_LIB=`echo "$BOOST_FILESYSTEM_LIB" | sed 's/.dll/-mt/'`
#   ;;
# esac

# #
# # Botan
# #
# PKG_CHECK_MODULES([BOTAN], [botan-1.10])

# # pkg-config doesn't distinguish between CPPFLAGS and CFLAGS, argh.
# # What we get for Botan's CFLAGS should all be in CPPFLAGS instead.
# BOTAN_CPPFLAGS="$BOTAN_CFLAGS"
# BOTAN_CFLAGS=""

# AC_SUBST([BOTAN_CPPFLAGS])

#
# liblightgrep
#

# PKG_CHECK_MODULES([LG], [lightgrep])

# LG_CPPFLAGS="$LG_CFLAGS"
# LG_CFLAGS=""

# AC_SUBST([LG_CPPFLAGS])

#
# tests
#

# Scope test framework
AC_ARG_WITH([scope],
  [AS_HELP_STRING([--with-scope=ARG],
    [use Scope headers from the specified location])],
  [SCOPE_PATH="$withval"],
  [SCOPE_PATH="vendors/scope"])

if test "x$with_scope" != "xno"; then
  # test Scope without adding its path to CPPFLAGS generally
  SCOPE_CPPFLAGS="-isystem `realpath $SCOPE_PATH`"
  CPPFLAGS_saved="$CPPFLAGS"
  CPPFLAGS="$SCOPE_CPPFLAGS"
  export CPPFLAGS

  AC_CHECK_HEADERS([scope/test.h],[scope_ok="yes"])

  CPPFLAGS="$CPPFLAGS_saved"

  if test "x$scope_ok" = "xyes"; then
    AC_DEFINE(HAVE_SCOPE,1,[Define to 1 if Scope test framework is available.])
    AC_SUBST([SCOPE_CPPFLAGS])
  fi
fi

# Tell the user why not if he won't be able to compile the tests
if test "x$scope_ok" != "xyes"; then
  AC_MSG_WARN([])
  AC_MSG_WARN([You will be unable to compile and run the tests because:])
  AC_MSG_WARN([])
  if test "x$scope_ok" != "xyes"; then
    AC_MSG_WARN([  * Scope test framework is unavailable])
  fi
  AC_MSG_WARN([])
fi

###########################################################################





#
# Threading
# 
case "$host" in
*-*-mingw*)
  AX_APPEND_FLAG([-mthreads], [X_CPPLAGS])
  AX_APPEND_LINK_FLAGS([-mthreads], [X_LDFLAGS])
  ;;
*-apple-darwin*)
  ;;
*)
  AX_APPEND_FLAG([-pthread], [X_CPPFLAGS])
  AX_APPEND_LINK_FLAGS([-pthread], [X_LDFLAGS])
esac



#AX_APPEND_FLAG([-Iinclude], [X_CPPFLAGS])


# Now that these vars have been appropriately munged, export them to automake
AC_SUBST([X_CPPFLAGS])
AC_SUBST([X_CXXFLAGS])
AC_SUBST([X_LDFLAGS])
 

# C++ library
#
case "$host" in
*-apple-darwin*)
  echo " ** MacOS X builds are problematic as compilers are in transition."
  echo " ** We presume you're using a recent clang and libc++."
  echo " ** If you build the unit tests, you are entering a world of pain."
  AX_APPEND_COMPILE_FLAGS([-stdlib=libc++], [STDCXX_CXXFLAGS])
  # TODO: Actually linking against libc++ will require everything else down
  # the chain to have been linked with libc++, including Boost, ICU, etc.
  # So, don't do this just yet.
  # STDCXX_LIB='-lc++'
  STDCXX_LIB='-lstdc++'
  ;;
*)
  STDCXX_LIB='-lstdc++'
  ;;
esac

AC_SUBST([STDCXX_LIB])
AC_SUBST([STDCXX_CXXFLAGS])

# FIXME: need to add -install_name to LDFLAGS when building a dynamic lib
# for MacOS X.

# FIXME: Is this really the right way to do this?!
# Build dynamically-linked executables if we build dynamic libs
if test "x$enable_shared" != "xyes"; then
  CXXLD="$CXX -all-static"
  CCLD="$CC -all-static"
else
  CXXLD="$CXX"
  CCLD="$CC"
fi

AC_SUBST([CXXLD])
AC_SUBST([CCLD])

#
# Report on flags
#

# TODO: libtre has a nice display we can steal
AC_MSG_NOTICE([BOOST_CPPFLAGS: $BOOST_CPPFLAGS])
AC_MSG_NOTICE([BOOST_CXXFLAGS: $BOOST_CXXFLAGS])
# AC_MSG_NOTICE([BOTAN_CFLAGS:   $BOTAN_CFLAGS])

AC_MSG_NOTICE([CC:       $CC])
AC_MSG_NOTICE([CXX:      $CXX])
AC_MSG_NOTICE([CPPFLAGS: $CPPFLAGS])
AC_MSG_NOTICE([CFLAGS:   $CFLAGS])
AC_MSG_NOTICE([CXXFLAGS: $CXXFLAGS])
AC_MSG_NOTICE([LIBS:     $LIBS])
AC_MSG_NOTICE([LDFLAGS:  $LDFLAGS])

#
# Dump recursively expanded output variables to tup.config.in
#
(
  # prefix and exec_prefix must be defaulted when equal to NONE
  test "x$prefix" = xNONE && prefix="$ac_default_prefix"
  test "x$exec_prefix" = xNONE && exec_prefix="${prefix}"

  for var in $ac_subst_vars; do
    eval val=\$$var
    echo "${var}='${val}'"
  done | ./expand.py > build/tup.config.in
)

AC_CONFIG_FILES([Makefile build/tup.config])
AC_OUTPUT
