/*
 * Copyright (c) 2014 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 */

#include "drivers.h"
#include "iio-buffer-utils.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

/* 1G (9.81m/s²) corresponds to "256"
 * value x scale is in m/s² */
#define SCALE_TO_FF(scale) (scale * 256 / 9.81)

typedef struct {
	guint              timeout_id;
	ReadingsUpdateFunc callback_func;
	gpointer           user_data;

	GUdevDevice *dev;
	const char *dev_path;
	int device_id;
	BufferDrvData *buffer_data;
} DrvData;

static DrvData *drv_data = NULL;

static int
process_scan (IIOSensorData data, DrvData *or_data)
{
	int i;
	int accel_x, accel_y, accel_z;
	gdouble scale;
	gboolean present_x, present_y, present_z;
	AccelReadings readings;

	if (data.read_size < 0) {
		g_warning ("Couldn't read from device: %s", g_strerror (errno));
		return 0;
	}

	/* Rather than read everything:
	 * for (i = 0; i < data.read_size / or_data->scan_size; i++)...
	 * Just read the last one */
	i = (data.read_size / or_data->buffer_data->scan_size) - 1;
	if (i < 0) {
		g_debug ("Not enough data to read (read_size: %d scan_size: %d)", (int) data.read_size, or_data->buffer_data->scan_size);
		return 0;
	}

	process_scan_1(data.data + or_data->buffer_data->scan_size*i, or_data->buffer_data, "in_accel_x", &accel_x, &scale, &present_x);
	process_scan_1(data.data + or_data->buffer_data->scan_size*i, or_data->buffer_data, "in_accel_y", &accel_y, &scale, &present_y);
	process_scan_1(data.data + or_data->buffer_data->scan_size*i, or_data->buffer_data, "in_accel_z", &accel_z, &scale, &present_z);

	g_debug ("Read from IIO: %d, %d, %d", accel_x, accel_y, accel_z);

	/* To match the Pegatron accelerometer code
	 * (see pega_accel_poll() in asus-laptop.c)
	 * we invert both x, and y values */
	accel_x = -accel_x;
	accel_y = -accel_y;

	//FIXME report errors
	readings.accel_x = accel_x * scale;
	readings.accel_y = accel_y * scale;
	readings.accel_z = accel_z * scale;
	or_data->callback_func (&iio_buffer_accel, (gpointer) &readings, or_data->user_data);

	return 1;
}

static void
prepare_output (DrvData    *or_data,
		const char *dev_dir_name,
		const char *trigger_name)
{
	IIOSensorData data;

	int fp, buf_len = 127;

	data.data = g_malloc(or_data->buffer_data->scan_size * buf_len);

	/* Attempt to open non blocking to access dev */
	fp = open (or_data->dev_path, O_RDONLY | O_NONBLOCK);
	if (fp == -1) { /* If it isn't there make the node */
		g_warning ("Failed to open %s : %s", or_data->dev_path, strerror(errno));
		goto bail;
	}

	/* Actually read the data */
	data.read_size = read (fp, data.data, buf_len * or_data->buffer_data->scan_size);
	if (data.read_size == -1 && errno == EAGAIN) {
		g_debug ("No new data available");
	} else {
		process_scan(data, or_data);
	}

	close(fp);

bail:
	g_free(data.data);
}

static char *
get_trigger_name (GUdevDevice *device)
{
	GList *devices, *l;
	GUdevClient *client;
	gboolean has_trigger = FALSE;
	char *trigger_name;
	const gchar * const subsystems[] = { "iio", NULL };

	client = g_udev_client_new (subsystems);
	devices = g_udev_client_query_by_subsystem (client, "iio");

	/* Find the associated trigger */
	trigger_name = g_strdup_printf ("accel_3d-dev%s", g_udev_device_get_number (device));
	for (l = devices; l != NULL; l = l->next) {
		GUdevDevice *dev = l->data;

		if (g_strcmp0 (trigger_name, g_udev_device_get_sysfs_attr (dev, "name")) == 0) {
			g_debug ("Found associated trigger at %s", g_udev_device_get_sysfs_path (dev));
			has_trigger = TRUE;
			break;
		}
	}

	g_list_free_full (devices, g_object_unref);
	g_clear_object (&client);

	if (has_trigger)
		return trigger_name;

	g_warning ("Could not find trigger name associated with %s",
		   g_udev_device_get_sysfs_path (device));
	g_free (trigger_name);
	return NULL;
}


static gboolean
read_orientation (gpointer user_data)
{
	DrvData *data = user_data;

	prepare_output (data, data->buffer_data->dev_dir_name, data->buffer_data->trigger_name);

	return G_SOURCE_CONTINUE;
}

static gboolean
iio_buffer_accel_discover (GUdevDevice *device)
{
	if (g_strcmp0 (g_udev_device_get_subsystem (device), "iio") != 0)
		return FALSE;

	if (g_strcmp0 ("accel_3d", g_udev_device_get_sysfs_attr (device, "name")) != 0)
		return FALSE;

	g_debug ("Found accel_3d at %s", g_udev_device_get_sysfs_path (device));
	return TRUE;
}

static void
iio_buffer_accel_set_polling (gboolean state)
{
	if (drv_data->timeout_id > 0 && state)
		return;
	if (drv_data->timeout_id == 0 && !state)
		return;

	if (drv_data->timeout_id) {
		g_source_remove (drv_data->timeout_id);
		drv_data->timeout_id = 0;
	}

	if (state) {
		drv_data->timeout_id = g_timeout_add (700, read_orientation, drv_data);
		g_source_set_name_by_id (drv_data->timeout_id, "[iio_buffer_accel_set_polling] read_orientation");
	}
}

static gboolean
iio_buffer_accel_open (GUdevDevice        *device,
		       ReadingsUpdateFunc  callback_func,
		       gpointer            user_data)
{
	char *trigger_name;

	drv_data = g_new0 (DrvData, 1);

	/* Get the trigger name, and build the channels from that */
	trigger_name = get_trigger_name (device);
	if (!trigger_name) {
		g_clear_pointer (&drv_data, g_free);
		return FALSE;
	}
	drv_data->buffer_data = buffer_drv_data_new (device, trigger_name);
	g_free (trigger_name);

	if (!drv_data->buffer_data) {
		g_clear_pointer (&drv_data, g_free);
		return FALSE;
	}

	drv_data->dev = g_object_ref (device);
	drv_data->dev_path = g_udev_device_get_device_file (device);

	drv_data->callback_func = callback_func;
	drv_data->user_data = user_data;

	return TRUE;
}

static void
iio_buffer_accel_close (void)
{
	iio_buffer_accel_set_polling (FALSE);
	g_clear_pointer (&drv_data->buffer_data, buffer_drv_data_free);
	g_clear_object (&drv_data->dev);
	g_clear_pointer (&drv_data, g_free);
}

SensorDriver iio_buffer_accel = {
	.name = "IIO Buffer accelerometer",
	.type = DRIVER_TYPE_ACCEL,
	.specific_type = DRIVER_TYPE_ACCEL_IIO,

	.discover = iio_buffer_accel_discover,
	.open = iio_buffer_accel_open,
	.set_polling = iio_buffer_accel_set_polling,
	.close = iio_buffer_accel_close,
};
