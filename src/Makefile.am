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
	iio-buffer-utils.h			\
	iio-buffer-utils.c

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

EXTRA_DIST = test-orientation.c
