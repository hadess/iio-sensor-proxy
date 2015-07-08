iio-sensor-proxy
================

IIO sensors to D-Bus proxy

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

With a new enough version of systemd[1], and a GNOME 3.18 (or newer) based
system, orientation changes will automatically be applied when rotating
the panel.

Note that a number of kernel bugs will prevent it from working correctly on
some machines with the 3.16 kernel (kernel crashes on the Surface Pro, sensor
failing to work after suspend on the Yoga Pro, etc.).

You can verify this by running `udevadm info --export-db` and checking for
an output resembling this one:
```
P: /devices/virtual/input/input15
E: ABS=7
E: DEVPATH=/devices/virtual/input/input15
E: EV=9
E: ID_INPUT=1
E: ID_INPUT_ACCELEROMETER=1
E: ID_INPUT_ACCELEROMETER_ORIENTATION=normal
E: MODALIAS=input:b0006v0001p0002e0000-e0,3,kra0,1,2,mlsfw
E: NAME="IIO Accelerometer Proxy"
E: PRODUCT=6/1/2/0
E: PROP=0
E: SUBSYSTEM=input
E: TAGS=:seat:
E: USEC_INITIALIZED=74243
```

If that doesn't work, please file an issue, make sure any running iio-sensor-proxy has been stopped:
`systemctl stop iio-sensor-proxy.service`
and attach the output of:
`G_MESSAGES_DEBUG=all /usr/sbin/iio-sensor-proxy`

[1]: One including this patch:
http://thread.gmane.org/gmane.comp.sysutils.systemd.devel/32047

Tested on
---------

- Lenovo IdeaPad Yoga 13
- Microsoft Surface Pro 2
- Lenovo Yoga Pro 2
- Onda v975w
- Dell Venue 8 Pro
- Lenovo ThinkPad Twist
- MacBook Pro (8.2)
- Lenovo X1 Carbon 2014 (rev2)
