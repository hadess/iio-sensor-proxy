
bin_PROGRAMS = iio-sensor-proxy

iio_sensor_proxy_SOURCES = iio-sensor-proxy.c uinput.h
iio_sensor_proxy_CPPFLAGS =			\
	$(IIO_SENSOR_PROXY_CFLAGS)		\
	$(WARN_CFLAGS)
iio_sensor_proxy_LDADD = $(IIO_SENSOR_PROXY_LIBS) -lm
