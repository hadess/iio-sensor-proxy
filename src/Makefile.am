BUILT_SOURCES =					\
	iio-sensor-proxy-resources.c		\
	iio-sensor-proxy-resources.h

resource_files = $(shell glib-compile-resources --sourcedir=$(srcdir) --generate-dependencies $(srcdir)/iio-sensor-proxy.gresource.xml)
iio-sensor-proxy-resources.c: iio-sensor-proxy.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-source --c-name iio_sensor_proxy $<
iio-sensor-proxy-resources.h: iio-sensor-proxy.gresource.xml $(resource_files)
	$(AM_V_GEN) glib-compile-resources --target=$@ --sourcedir=$(srcdir) --generate-header --c-name iio_sensor_proxy $<

sbin_PROGRAMS = iio-sensor-proxy

iio_sensor_proxy_SOURCES =			\
	iio-sensor-proxy.c			\
	drivers.h				\
	orientation.c				\
	orientation.h				\
	drv-iio-buffer-accel.c			\
	drv-iio-poll-accel.c			\
	drv-input-accel.c			\
	drv-fake-compass.c			\
	drv-fake-light.c			\
	drv-iio-poll-light.c			\
	drv-hwmon-light.c			\
	drv-iio-buffer-light.c			\
	drv-iio-buffer-compass.c		\
	iio-buffer-utils.h			\
	iio-buffer-utils.c			\
	accel-mount-matrix.h			\
	accel-mount-matrix.c			\
	accel-attributes.h				\
	accel-attributes.c				\
	$(BUILT_SOURCES)

iio_sensor_proxy_CPPFLAGS =			\
	$(IIO_SENSOR_PROXY_CFLAGS)		\
	$(WARN_CFLAGS)
iio_sensor_proxy_LDADD = $(IIO_SENSOR_PROXY_LIBS) $(LIBM)

noinst_PROGRAMS = fake-input-accelerometer test-mount-matrix test-accel-location test-orientation

fake_input_accelerometer_SOURCES =		\
	fake-input-accelerometer.c		\
	uinput.h

fake_input_accelerometer_CPPFLAGS =		\
	$(IIO_SENSOR_PROXY_CFLAGS)		\
	$(WARN_CFLAGS)
fake_input_accelerometer_LDADD = $(IIO_SENSOR_PROXY_LIBS)

include $(top_srcdir)/Makefile.decl
TEST_PROGS += test-mount-matrix test-accel-location test-orientation

test_mount_matrix_SOURCES =			\
	test-mount-matrix.c			\
	accel-mount-matrix.h			\
	accel-mount-matrix.c

test_mount_matrix_CPPFLAGS =			\
	$(IIO_SENSOR_PROXY_CFLAGS)		\
	$(WARN_CFLAGS)
test_mount_matrix_LDADD = $(IIO_SENSOR_PROXY_LIBS)

test_accel_location_SOURCES =			\
	test-accel-location.c			\
	accel-attributes.h			\
	accel-attributes.c

test_accel_location_CPPFLAGS =			\
	$(IIO_SENSOR_PROXY_CFLAGS)		\
	$(WARN_CFLAGS)
test_accel_location_LDADD = $(IIO_SENSOR_PROXY_LIBS)

test_orientation_SOURCES =			\
	test-orientation.c			\
	orientation.c				\
	orientation.h				\
	accel-mount-matrix.c			\
	accel-mount-matrix.h

test_orientation_CPPFLAGS =			\
	$(IIO_SENSOR_PROXY_CFLAGS)		\
	$(WARN_CFLAGS)
test_orientation_LDADD = $(IIO_SENSOR_PROXY_LIBS) $(LIBM)

if HAVE_GTK_TESTS
noinst_PROGRAMS += test-orientation-gtk
test_orientation_gtk_SOURCES =			\
	test-orientation-gtk.c			\
	orientation.c				\
	orientation.h
test_orientation_gtk_CPPFLAGS =			\
	$(GTK_TESTS_CFLAGS)			\
	$(WARN_CFLAGS)
test_orientation_gtk_LDADD = $(GTK_TESTS_LIBS) $(LIBM)
endif

bin_PROGRAMS = monitor-sensor

monitor_sensor_SOURCES =			\
	monitor-sensor.c

monitor_sensor_CPPFLAGS =			\
	$(IIO_SENSOR_PROXY_CFLAGS)		\
	$(WARN_CFLAGS)
monitor_sensor_LDADD = $(IIO_SENSOR_PROXY_LIBS)

EXTRA_DIST =					\
	test-orientation-gtk.c			\
	net.hadess.SensorProxy.xml		\
	iio-sensor-proxy.gresource.xml
