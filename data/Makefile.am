udevrules_DATA = 40-iio-sensor-proxy.rules

systemdservicedir = $(systemdsystemunitdir)
systemdservice_in_files = iio-sensor-proxy.service.in
systemdservice_DATA = iio-sensor-proxy.service
iio-sensor-proxy.service: iio-sensor-proxy.service.in Makefile
	$(AM_V_GEN) $(SED) -e "s|\@sbindir\@|$(sbindir)|" $< > $@

dbus_confdir = /etc/dbus-1/system.d
dbus_conf_DATA = net.hadess.SensorProxy.conf

CLEANFILES = iio-sensor-proxy.service
EXTRA_DIST = $(udevrules_DATA) $(dbus_conf_DATA) $(systemdservice_in_files)
