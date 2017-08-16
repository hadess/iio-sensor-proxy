/*
 * Copyright (c) 2011, 2014 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
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
#include <termios.h>

#include <gudev/gudev.h>
#include "uinput.h"

#define ONEG 256

typedef struct {
	GMainLoop *loop;

	int uinput;
	int accel_x, accel_y, accel_z;
	GUdevClient *client;
	GUdevDevice *uinput_dev;
	struct termios old_tio;
} OrientationData;

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
	(void) write (data->uinput, &ev, sizeof(ev));

	ev.code = ABS_Y;
	ev.value = data->accel_y;
	(void) write (data->uinput, &ev, sizeof(ev));

	ev.code = ABS_Z;
	ev.value = data->accel_z;
	(void) write (data->uinput, &ev, sizeof(ev));

	memset(&ev, 0, sizeof(ev));
	gettimeofday(&ev.time, NULL);
	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	(void) write (data->uinput, &ev, sizeof(ev));

	if (!data->uinput_dev)
		data->uinput_dev = setup_uinput_udev (data->client);
	if (!data->uinput_dev)
		return FALSE;

	if (write_sysfs_string ((char *) "uevent", (char *) g_udev_device_get_sysfs_path (data->uinput_dev), (char *) "change") < 0) {
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
	snprintf (dev.name, sizeof (dev.name), "%s", "iio-sensor-proxy test application");
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
keyboard_usage (void)
{
	g_print ("Valid keys are: u (up), d (down), l (left), r (right), q/x (quit)\n");
}

static gboolean
check_keyboard (GIOChannel      *source,
		GIOCondition     condition,
		OrientationData *data)
{
	GIOStatus status;
	char buf[1];

	status = g_io_channel_read_chars (source, buf, 1, NULL, NULL);
	if (status == G_IO_STATUS_ERROR ||
	    status == G_IO_STATUS_EOF) {
		g_main_loop_quit (data->loop);
		return FALSE;
	}

	if (status == G_IO_STATUS_AGAIN)
		return TRUE;

	switch (buf[0]) {
	case 'u':
		data->accel_x = 0;
		data->accel_y = -ONEG;
		data->accel_z = 0;
		break;
	case 'd':
		data->accel_x = 0;
		data->accel_y = ONEG;
		data->accel_z = 0;
		break;
	case 'l':
		data->accel_x = ONEG;
		data->accel_y = 0;
		data->accel_z = 0;
		break;
	case 'r':
		data->accel_x = -ONEG;
		data->accel_y = 0;
		data->accel_z = 0;
		break;
	case 'q':
	case 'x':
		g_main_loop_quit (data->loop);
		return FALSE;
	default:
		keyboard_usage ();
		return TRUE;
	}

	send_uinput_event (data);

	return TRUE;
}

static gboolean
setup_keyboard (OrientationData *data)
{
	GIOChannel *channel;
	struct termios new_tio;

	tcgetattr(STDIN_FILENO, &data->old_tio);
	new_tio = data->old_tio;
	new_tio.c_lflag &=(~ICANON & ~ECHO);
	tcsetattr(STDIN_FILENO, TCSANOW, &new_tio);

	channel = g_io_channel_unix_new (STDIN_FILENO);
	if (!channel) {
		g_warning ("Failed to open stdin");
		return FALSE;
	}

	if (g_io_channel_set_encoding (channel, NULL, NULL) != G_IO_STATUS_NORMAL) {
		g_warning ("Failed to set stdin encoding to NULL");
		return FALSE;
	}

	g_io_add_watch (channel, G_IO_IN, (GIOFunc) check_keyboard, data);
	return TRUE;
}

static void
free_orientation_data (OrientationData *data)
{
	if (data == NULL)
		return;

	tcsetattr(STDIN_FILENO, TCSANOW, &data->old_tio);

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
	const gchar * const subsystems[] = { "input", NULL };
	int ret = 0;

	data = g_new0 (OrientationData, 1);
	data->client = g_udev_client_new (subsystems);

	if (!setup_keyboard (data)) {
		g_warning ("Failed to setup keyboard capture");
		ret = 1;
		goto out;
	}

	/* Set up uinput */
	if (!setup_uinput (data)) {
		ret = 1;
		goto out;
	}

	/* Start with the 'normal' orientation */
	data->accel_x = 0;
	data->accel_y = ONEG;
	data->accel_z = 0;

	send_uinput_event (data);
	keyboard_usage ();

	data->loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (data->loop);

out:
	free_orientation_data (data);

	return ret;
}
