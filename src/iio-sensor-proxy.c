/*
 * Modified from industrialio buffer test code, and Lenovo Yoga (2 Pro) orientation helper
 * Copyright (c) 2008 Jonathan Cameron
 * Copyright (c) 2014 Peter F. Patel-Schneider
 * Copyright (c) 2011, 2014 Bastien Nocera <hadess@hadess.net>
 *
 * Every 700 msec, read data from an IIO accelerometer, and
 * from the accelerometer values, as well as the previous
 * orientation, calculate the device's new orientation.
 *
 * Possible values are:
 * * undefined
 * * normal
 * * bottom-up
 * * left-up
 * * right-up
 *
 * The property will be persistent across sessions, and the new
 * orientations can be deducted from the previous one (it allows
 * for a threshold for switching between opposite ends of the
 * orientation).
 *
 * orientation_calc() from the sensorfw package
 * Copyright (C) 2009-2010 Nokia Corporation
 * Authors:
 *   Üstün Ergenoglu <ext-ustun.ergenoglu@nokia.com>
 *   Timo Rongas <ext-timo.2.rongas@nokia.com>
 *   Lihan Guo <lihan.guo@digia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 */

#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <math.h>
#include <stdio.h>

#include <gudev/gudev.h>
#include "uinput.h"
#include "drivers.h"

typedef enum {
        ORIENTATION_UNDEFINED,
        ORIENTATION_NORMAL,
        ORIENTATION_BOTTOM_UP,
        ORIENTATION_LEFT_UP,
        ORIENTATION_RIGHT_UP
} OrientationUp;

static const char *orientations[] = {
        "undefined",
        "normal",
        "bottom-up",
        "left-up",
        "right-up",
        NULL
};

#define ORIENTATION_UP_UP ORIENTATION_NORMAL

#define RADIANS_TO_DEGREES 180.0/M_PI
#define SAME_AXIS_LIMIT 5

#define THRESHOLD_LANDSCAPE  35
#define THRESHOLD_PORTRAIT  35

static const char *
orientation_to_string (OrientationUp o)
{
        return orientations[o];
}

#if 0
static OrientationUp
string_to_orientation (const char *orientation)
{
        int i;

        if (orientation == NULL)
                return ORIENTATION_UNDEFINED;
        for (i = 0; orientations[i] != NULL; i++) {
                if (g_str_equal (orientation, orientations[i]))
                        return i;
        }
        return ORIENTATION_UNDEFINED;
}
#endif

static OrientationUp
orientation_calc (OrientationUp prev,
                  int x, int y, int z)
{
        int rotation;
        OrientationUp ret = prev;

        /* Portrait check */
        rotation = round(atan((double) x / sqrt(y * y + z * z)) * RADIANS_TO_DEGREES);

        if (abs(rotation) > THRESHOLD_PORTRAIT) {
                ret = (rotation < 0) ? ORIENTATION_LEFT_UP : ORIENTATION_RIGHT_UP;

                /* Some threshold to switching between portrait modes */
                if (prev == ORIENTATION_LEFT_UP || prev == ORIENTATION_RIGHT_UP) {
                        if (abs(rotation) < SAME_AXIS_LIMIT) {
                                ret = prev;
                        }
                }

        } else {
                /* Landscape check */
                rotation = round(atan((double) y / sqrt(x * x + z * z)) * RADIANS_TO_DEGREES);

                if (abs(rotation) > THRESHOLD_LANDSCAPE) {
                        ret = (rotation < 0) ? ORIENTATION_BOTTOM_UP : ORIENTATION_NORMAL;

                        /* Some threshold to switching between landscape modes */
                        if (prev == ORIENTATION_BOTTOM_UP || prev == ORIENTATION_NORMAL) {
                                if (abs(rotation) < SAME_AXIS_LIMIT) {
                                        ret = prev;
                                }
                        }
                }
        }

        return ret;
}

typedef struct {
	GMainLoop *loop;

	SensorDriver *driver;

	int uinput;
	int accel_x, accel_y, accel_z;
	GUdevClient *client;
	GUdevDevice *uinput_dev;

	OrientationUp previous_orientation;
} OrientationData;

static const SensorDriver * const drivers[] = {
	&iio_buffer_accel,
	&iio_poll_accel
};

static GUdevDevice *
find_accel (GUdevClient   *client,
	    SensorDriver **driver)
{
	GList *devices, *l;
	GUdevDevice *ret = NULL;

	*driver = NULL;
	devices = g_udev_client_query_by_subsystem (client, "iio");

	/* Find the accelerometer */
	for (l = devices; l != NULL; l = l->next) {
		GUdevDevice *dev = l->data;
		guint i;

		for (i = 0; i < G_N_ELEMENTS(drivers); i++) {
			if (drivers[i]->discover (dev)) {
				g_debug ("Found device %s at %s",
					 g_udev_device_get_sysfs_path (dev),
					 drivers[i]->name);
				ret = g_object_ref (dev);
				*driver = (SensorDriver *) drivers[i];
				break;
			}
		}
	}

	g_list_free_full (devices, g_object_unref);
	return ret;
}

static GUdevDevice *
setup_uinput_udev (GUdevClient *client)
{
	GList *devices, *l;
	GUdevDevice *ret = NULL;

	devices = g_udev_client_query_by_subsystem (client, "input");
	for (l = devices; l != NULL; l = l->next) {
		GUdevDevice *dev = l->data;

		if (g_udev_device_get_property_as_boolean (dev, "ID_INPUT_ACCELEROMETER")) {
			ret = g_object_ref (dev);
			break;
		}
	}
	g_list_free_full (devices, g_object_unref);

	return ret;
}

static int
write_sysfs_string (char *filename,
		     char *basedir,
		     char *val)
{
	int ret = 0;
	FILE *sysfsfp;
	char *temp;

	temp = g_build_filename (basedir, filename, NULL);
	sysfsfp = fopen (temp, "w");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}
	fprintf(sysfsfp, "%s", val);
	fclose(sysfsfp);

error_free:
	g_free(temp);

	return ret;
}

static gboolean
send_uinput_event (OrientationData *data)
{
	struct uinput_event ev;

	memset(&ev, 0, sizeof(ev));

	ev.type = EV_ABS;
	ev.code = ABS_X;
	ev.value = data->accel_x;
	write (data->uinput, &ev, sizeof(ev));

	ev.code = ABS_Y;
	ev.value = data->accel_y;
	write (data->uinput, &ev, sizeof(ev));

	ev.code = ABS_Z;
	ev.value = data->accel_z;
	write (data->uinput, &ev, sizeof(ev));

	memset(&ev, 0, sizeof(ev));
	gettimeofday(&ev.time, NULL);
	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	write (data->uinput, &ev, sizeof(ev));

	if (!data->uinput_dev)
		data->uinput_dev = setup_uinput_udev (data->client);
	if (!data->uinput_dev)
		return FALSE;

	if (write_sysfs_string ("uevent", (char *) g_udev_device_get_sysfs_path (data->uinput_dev), "change") < 0) {
		g_warning ("Failed to write uevent");
		return FALSE;
	}

	return TRUE;
}

static gboolean
setup_uinput (OrientationData *data)
{
	struct uinput_dev dev;
	int fd;

	fd = open("/dev/uinput", O_RDWR);
	if (fd < 0) {
		g_warning ("Could not open uinput");
		return FALSE;
	}

	memset (&dev, 0, sizeof(dev));
	snprintf (dev.name, sizeof (dev.name), "%s", "IIO Accelerometer Proxy");
	dev.id.bustype = BUS_VIRTUAL;
	dev.id.vendor = 0x01; //FIXME
	dev.id.product = 0x02;

	/* 1G accel is reported as ~256, so clamp to 2G */
	dev.absmin[ABS_X] = dev.absmin[ABS_Y] = dev.absmin[ABS_Z] = -512;
	dev.absmax[ABS_X] = dev.absmax[ABS_Y] = dev.absmax[ABS_Z] = 512;

	if (write (fd, &dev, sizeof(dev)) != sizeof(dev)) {
		g_warning ("Error creating uinput device");
		goto bail;
	}

	/* enabling key events */
	if (ioctl (fd, UI_SET_EVBIT, EV_ABS) < 0) {
		g_warning ("Error enabling uinput absolute events");
		goto bail;
	}

	/* enabling keys */
	if (ioctl (fd, UI_SET_ABSBIT, ABS_X) < 0 ||
	    ioctl (fd, UI_SET_ABSBIT, ABS_Y) < 0 ||
	    ioctl (fd, UI_SET_ABSBIT, ABS_Z) < 0) {
		g_warning ("Couldn't enable uinput axis");
		goto bail;
	}

	/* creating the device */
	if (ioctl (fd, UI_DEV_CREATE) < 0) {
		g_warning ("Error creating uinput device");
		goto bail;
	}

	data->uinput = fd;

	return TRUE;

bail:
	close (fd);
	return FALSE;
}

static void
accel_changed_func (SensorDriver *driver,
		    int           accel_x,
		    int           accel_y,
		    int           accel_z,
		    gpointer      user_data)
{
	OrientationData *data = user_data;
	OrientationUp orientation = data->previous_orientation;

	//FIXME handle errors

	/* To match the Pegatron accelerometer code
	 * (see pega_accel_poll() in asus-laptop.c)
	 * we invert both x, and y values */
	accel_x = -accel_x;
	accel_y = -accel_y;

	g_debug ("Read from IIO: %d, %d, %d", accel_x, accel_y, accel_z);

	orientation = orientation_calc (data->previous_orientation, accel_x, accel_y, accel_z);

	data->accel_x = accel_x;
	data->accel_y = accel_y;
	data->accel_z = accel_z;

	if (data->previous_orientation != orientation) {
		/* If we failed to send the uevent,
		 * we'll try again a bit later */
		if (send_uinput_event (data)) {
			g_debug ("Emitted orientation changed: from %s to %s",
				 orientation_to_string (data->previous_orientation),
				 orientation_to_string (orientation));
			data->previous_orientation = orientation;
		}
	}
}

static void
free_orientation_data (OrientationData *data)
{
	if (data == NULL)
		return;
	if (data->uinput > 0)
		close (data->uinput);
	g_clear_object (&data->uinput_dev);
	g_clear_object (&data->client);
	g_clear_pointer (&data->loop, g_main_loop_unref);
	g_free (data);
}

int main (int argc, char **argv)
{
	OrientationData *data;
	GUdevClient *client;
	GUdevDevice *dev;
	SensorDriver *driver;
	const gchar * const subsystems[] = { "iio", NULL };
	int ret = 0;

	/* g_setenv ("G_MESSAGES_DEBUG", "all", TRUE); */

	client = g_udev_client_new (subsystems);
	dev = find_accel (client, &driver);
	if (!dev) {
		g_debug ("Could not find IIO accelerometer");
		return 0;
	}

	data = g_new0 (OrientationData, 1);
	data->previous_orientation = ORIENTATION_UNDEFINED;
	data->client = client;
	data->driver = driver;

	/* Open up the accelerometer */
	if (!data->driver->open (dev,
				 accel_changed_func,
				 data)) {
		ret = 1;
		goto out;
	}

	/* Set up uinput */
	if (!setup_uinput (data)) {
		ret = 1;
		goto out;
	}
	send_uinput_event (data);

	data->loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (data->loop);

out:
	data->driver->close ();

	g_object_unref (dev);
	free_orientation_data (data);

	return ret;
}
