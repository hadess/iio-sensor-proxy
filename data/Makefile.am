udevrules_DATA = 80-iio-sensor-proxy.rules

systemdservicedir = $(systemdsystemunitdir)
systemdservice_in_files = iio-sensor-proxy.service.in
systemdservice_DATA = iio-sensor-proxy.service
iio-sensor-proxy.service: iio-sensor-proxy.service.in Makefile
	$(AM_V_GEN) $(SED) -e "s|\@sbindir\@|$(sbindir)|" $< > $@

dbusconfdir = $(sysconfdir)/dbus-1/system.d
dbusconf_DATA = net.hadess.SensorProxy.conf

net.hadess.SensorProxy.conf: net.hadess.SensorProxy.conf.in Makefile
	@sed -e "s|\@geoclue_user\@|$(geoclue_user)|" $< > $@


CLEANFILES = iio-sensor-proxy.service net.hadess.SensorProxy.conf
EXTRA_DIST = $(udevrules_DATA) $(dbusconf_DATA) $(systemdservice_in_files) net.hadess.SensorProxy.conf.in
