
AC_DEFUN([OL_DEBUG], [
  AC_MSG_CHECKING([whether debugging is enabled])
  AC_ARG_ENABLE(
    [debug],
    AS_HELP_STRING(--enable-debug, enable -Werror and assert()),
    [ case "$enableval" in
        yes) ol_debug=yes ;;
         no) ol_debug=no ;;
          *) AC_MSG_RESULT([error])
             AC_MSG_ERROR([bad value "$enableval" for --enable-debug]) ;;
      esac
    ]
  )
  if test "$ol_debug" = yes; then
    echo "ENABLE DEBUG"
    CFLAGS="$CFLAGS -g -O0 -Wall -Werror -Wno-switch -fPIC -rdynamic"
  else
    AC_DEFINE([NDEBUG], [1],
      [Define to 1 if you are building a production release.]
    )
    CFLAGS="$CFLAGS -g -O0 -Wall -Wno-switch -fPIC -rdynamic"
  fi
  AC_MSG_RESULT([${ol_debug=no}])
 ]
)
