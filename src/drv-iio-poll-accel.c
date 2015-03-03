/*
 * Copyright (c) 2014 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include "drivers.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>

/* 1G (9.81m/s²) corresponds to "256"
 * value x scale is in m/s² */
#define SCALE_TO_FF(scale) (scale * 256 / 9.81)

typedef struct DrvData {
	guint               timeout_id;
	ReadingsUpdateFunc  callback_func;
	gpointer            user_data;
	GUdevDevice        *dev;

	double              scale;
} DrvData;

static DrvData *drv_data = NULL;

static int
sysfs_get_int (GUdevDevice *dev,
	       const char  *attribute)
{
	int result;
	char *contents;
	char *filename;

	result = 0;
	filename = g_build_filename (g_udev_device_get_sysfs_path (dev), attribute, NULL);
	if (g_file_get_contents (filename, &contents, NULL, NULL)) {
		result = atoi (contents);
		g_free (contents);
	}
	g_free (filename);

	return result;
}

static gboolean
poll_orientation (gpointer user_data)
{
	DrvData *data = user_data;
	int accel_x, accel_y, accel_z;

	accel_x = sysfs_get_int (data->dev, "in_accel_x_raw") * data->scale;
	accel_y = sysfs_get_int (data->dev, "in_accel_y_raw") * data->scale;
	accel_z = sysfs_get_int (data->dev, "in_accel_z_raw") * data->scale;

	//FIXME report errors
	if (g_strcmp0 ("i2c-SMO8500:00", g_udev_device_get_sysfs_attr (data->dev, "name")) == 0) {
		/* Quirk for i2c-SMO8500:00 device,
		 * swap x and y */
		data->callback_func (&iio_poll_accel, accel_y, accel_x, accel_z, data->user_data);
	} else {
		data->callback_func (&iio_poll_accel, accel_x, accel_y, accel_z, data->user_data);
	}

	return G_SOURCE_CONTINUE;
}

static gboolean
iio_poll_accel_discover (GUdevDevice *device)
{
	if (g_strcmp0 ("i2c-SMO8500:00", g_udev_device_get_sysfs_attr (device, "name")) != 0)
		return FALSE;

	g_debug ("Found polling accelerometer at %s", g_udev_device_get_sysfs_path (device));
	return TRUE;
}

static gboolean
iio_poll_accel_open (GUdevDevice        *device,
		       ReadingsUpdateFunc  callback_func,
		       gpointer            user_data)
{
	drv_data = g_new0 (DrvData, 1);
	drv_data->dev = g_object_ref (device);

	drv_data->callback_func = callback_func;
	drv_data->user_data = user_data;
	drv_data->scale = SCALE_TO_FF(g_udev_device_get_sysfs_attr_as_double (device, "in_accel_scale"));
	if (drv_data->scale == 0.0)
		drv_data->scale = 1.0;

	drv_data->timeout_id = g_timeout_add (700, poll_orientation, drv_data);
	g_source_set_name_by_id (drv_data->timeout_id, "poll_orientation");

	return TRUE;
}

static void
iio_poll_accel_close (void)
{
	g_source_remove (drv_data->timeout_id);
	g_clear_object (&drv_data->dev);
	g_clear_pointer (&drv_data, g_free);
}

SensorDriver iio_poll_accel = {
	.name = "IIO Poll accelerometer",
	.type = DRIVER_TYPE_ACCEL,
	.specific_type = DRIVER_TYPE_ACCEL_IIO,

	.discover = iio_poll_accel_discover,
	.open = iio_poll_accel_open,
	.close = iio_poll_accel_close,
};
