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
	drv-fake-light.c			\
	drv-iio-poll-light.c			\
	drv-hwmon-light.c			\
	drv-iio-buffer-light.c			\
	drv-iio-buffer-compass.c		\
	iio-buffer-utils.h			\
	iio-buffer-utils.c			\
	$(BUILT_SOURCES)

iio_sensor_proxy_CPPFLAGS =			\
	$(IIO_SENSOR_PROXY_CFLAGS)		\
	$(WARN_CFLAGS)
iio_sensor_proxy_LDADD = $(IIO_SENSOR_PROXY_LIBS) -lm

noinst_PROGRAMS = fake-input-accelerometer

fake_input_accelerometer_SOURCES =		\
	fake-input-accelerometer.c		\
	uinput.h

fake_input_accelerometer_CPPFLAGS =		\
	$(IIO_SENSOR_PROXY_CFLAGS)		\
	$(WARN_CFLAGS)
fake_input_accelerometer_LDADD = $(IIO_SENSOR_PROXY_LIBS)

bin_PROGRAMS = monitor-sensor

monitor_sensor_SOURCES =			\
	monitor-sensor.c

monitor_sensor_CPPFLAGS =			\
	$(IIO_SENSOR_PROXY_CFLAGS)		\
	$(WARN_CFLAGS)
monitor_sensor_LDADD = $(IIO_SENSOR_PROXY_LIBS)

EXTRA_DIST =					\
	test-orientation.c			\
	net.hadess.SensorProxy.xml		\
	iio-sensor-proxy.gresource.xml
