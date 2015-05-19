AC_PREREQ(2.59)

AC_INIT([iio-sensor-proxy], [0.1], [hadess@hadess.net])
AM_INIT_AUTOMAKE(1.9 dist-xz no-dist-gzip check-news)

# Enable silent build when available (Automake 1.11)
m4_ifdef([AM_SILENT_RULES],[AM_SILENT_RULES([yes])])

AC_PROG_CC
AC_PROG_MAKE_SET
AC_PROG_INSTALL
AC_PROG_SED
AM_PROG_CC_C_O

GNOME_COMPILE_WARNINGS([maximum])

GTK_DOC_CHECK(1.0)

PKG_CHECK_MODULES(IIO_SENSOR_PROXY, gio-2.0 gudev-1.0)

AC_PATH_PROG([GDBUS_CODEGEN],[gdbus-codegen])

AC_CONFIG_FILES([
	Makefile
	data/Makefile
	src/Makefile
	docs/Makefile
	docs/version.xml
])
AC_OUTPUT
