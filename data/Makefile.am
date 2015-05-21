rulesdir = /lib/udev/rules.d/
rules_DATA = 40-iio-sensor-proxy.rules

servicedir = /usr/lib/systemd/system/
service_DATA = iio-sensor-proxy.service
iio-sensor-proxy.service: iio-sensor-proxy.service.in Makefile
	$(AM_V_GEN) $(SED) -e "s|\@sbindir\@|$(sbindir)|" $< > $@

dbus_confdir = /etc/dbus-1/system.d
dbus_conf_DATA = net.hadess.SensorProxy.conf

CLEANFILES = iio-sensor-proxy.service
EXTRA_DIST = $(rules_DATA) $(dbus_conf_DATA) iio-sensor-proxy.service.in
