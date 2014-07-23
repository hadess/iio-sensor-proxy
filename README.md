iio-sensor-proxy
================

IIO accelerometer sensor to input device proxy

Installation
------------
```
make
make install
```
It requires libgudev and systemd.

Usage
-----

With a new enough version of systemd[1], and a GNOME 3 based system,
orientation changes will automatically be applied when rotating the panel.

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
http://cgit.freedesktop.org/systemd/systemd/commit/?id=a545c6e1aa31b4d7e80c9d3609d9fc4fc9921498

Tested on
---------

- Lenovo IdeaPad Yoga 13
- Microsoft Surface Pro 2
