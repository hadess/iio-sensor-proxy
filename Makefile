CFLAGS = -g -Wall -Wchar-subscripts -Wnested-externs -Wpointer-arith -Wcast-align -Wsign-compare `pkg-config --cflags glib-2.0 gudev-1.0`
LIBS = `pkg-config --libs glib-2.0 gudev-1.0` -lm

FILES = iio-sensor-proxy.c 40-iio-sensor-proxy.rules iio-sensor-proxy.service uinput.h README.md Makefile
NAME = iio-sensor-proxy
VERSION = 0.1

iio-sensor-proxy: iio-sensor-proxy.c uinput.h
	cc $(CFLAGS) iio-sensor-proxy.c -o iio-sensor-proxy $(LIBS)

clean:
	rm -f iio-sensor-proxy $(NAME)-$(VERSION)/

install: iio-sensor-proxy 40-iio-sensor-proxy.rules iio-sensor-proxy.service
	install --owner=root --group=root --mode=755 iio-sensor-proxy /usr/sbin
	install --owner=root --group=root --mode=644 40-iio-sensor-proxy.rules /lib/udev/rules.d/
	install --owner=root --group=root --mode=644 iio-sensor-proxy.service /usr/lib/systemd/system/

dist: $(FILES)
	rm -f $(NAME)-$(VERSION).tar.xz
	mkdir -p $(NAME)-$(VERSION)
	cp -a $(FILES) $(NAME)-$(VERSION)/
	tar cvJf $(NAME)-$(VERSION).tar.xz $(NAME)-$(VERSION)/
	rm -rf $(NAME)-$(VERSION)/
