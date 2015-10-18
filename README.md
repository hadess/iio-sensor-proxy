iio-sensor-proxy
================

IIO sensors to D-Bus proxy

See https://developer.gnome.org/iio-sensor-proxy/1.0/ for
developer information.

Installation
------------
```
./configure --prefix=/usr --sysconfdir=/etc
make
make install
```
It requires libgudev and systemd.

Usage
-----

With a GNOME 3.18 (or newer) based system, orientation changes will
automatically be applied when rotating the panel, ambient light will be used
to change the screen brightness, and Geoclue will be able to read the compass
data to show the direction in Maps.

Note that a number of kernel bugs will prevent it from working correctly on
some machines so please make sure to use the latest upstream kernel (kernel
crashes on the Surface Pro, sensor failing to work after suspend on the Yoga
Pro, etc.).

You can verify that sensors are detected by running `udevadm info --export-db`
and checking for an output resembling this one:
```
P: /devices/platform/80860F41:04/i2c-12/i2c-BMA250E:00/iio:device0
N: iio:device0
E: DEVNAME=/dev/iio:device0
E: DEVPATH=/devices/platform/80860F41:04/i2c-12/i2c-BMA250E:00/iio:device0
E: DEVTYPE=iio_device
E: MAJOR=249
E: MINOR=0
E: SUBSYSTEM=iio
E: SYSTEMD_WANTS=iio-sensor-proxy.service
E: TAGS=:systemd:
E: USEC_INITIALIZED=7750292
```

You can now check whether a sensor is detected by running:
```
gdbus introspect --system --dest net.hadess.SensorProxy --object-path /net/hadess/SensorProxy
```

After that, use `monitor-sensor` to see changes in the ambient light sensor
or the accelerometer. Note that compass changes are only available to GeoClue
but if you need to ensure that GeoClue is getting correct data you can run:
`su -s /bin/sh geoclue -c monitor-sensor`

If that doesn't work, please file an issue, make sure any running iio-sensor-proxy
has been stopped:
`systemctl stop iio-sensor-proxy.service`
and attach the output of:
`G_MESSAGES_DEBUG=all /usr/sbin/iio-sensor-proxy`

Tested on
---------

- Lenovo IdeaPad Yoga 13
- Microsoft Surface Pro 2
- Lenovo Yoga Pro 2
- Onda v975w
- Dell Venue 8 Pro
- Dell Venue 11 Pro (7140)
- Lenovo ThinkPad Twist
- MacBook Pro (8.2)
- Lenovo X1 Carbon 2014 (rev2)
- MacBook Air (6,2)
