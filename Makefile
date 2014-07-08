CFLAGS = -g -Wall -Wchar-subscripts -Wnested-externs -Wpointer-arith -Wcast-align -Wsign-compare `pkg-config --cflags glib-2.0 gudev-1.0`
LIBS = `pkg-config --libs glib-2.0 gudev-1.0` -lm

iio-sensor-proxy: iio-sensor-proxy.c uinput.h
	cc $(CFLAGS) iio-sensor-proxy.c -o iio-sensor-proxy $(LIBS)

clean:
	rm -f iio-sensor-proxy

install: iio-sensor-proxy
	install --owner=root --group=root --mode=755 iio-sensor-proxy /usr/sbin
	install --owner=root --group=root --mode=644 40-iio-sensor-proxy.rules /lib/udev/rules.d/
	install --owner=root --group=root --mode=644 iio-sensor-proxy.service /usr/lib/systemd/system/
