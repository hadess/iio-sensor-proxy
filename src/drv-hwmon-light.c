/*
 * Copyright (c) 2014 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 */

#include "drivers.h"

#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <stdio.h>

#define DEFAULT_POLL_TIME 8000
#define MAX_LIGHT_LEVEL   255

typedef struct DrvData {
	ReadingsUpdateFunc  callback_func;
	gpointer            user_data;

	char               *light_path;
	guint               timeout_id;
} DrvData;

static DrvData *drv_data = NULL;

static gboolean
hwmon_light_discover (GUdevDevice *device)
{
	if (g_strcmp0 (g_udev_device_get_property (device, "IIO_SENSOR_PROXY_TYPE"), "hwmon-als") != 0)
		return FALSE;

	g_debug ("Found HWMon light at %s", g_udev_device_get_sysfs_path (device));
	return TRUE;
}

static gboolean
light_changed (void)
{
	LightReadings readings;
	gdouble level;
	char *contents;
	GError *error = NULL;

	if (g_file_get_contents (drv_data->light_path, &contents, NULL, &error)) {
		int light1, light2;
		if (sscanf (contents, "(%d,%d)", &light1, &light2) != 2) {
			g_warning ("Failed to parse light level: %s", contents);
			g_free (contents);
			return G_SOURCE_CONTINUE;
		}
		level = ((float) MAX(light1, light2)) / (float) MAX_LIGHT_LEVEL * 100.0;
		g_free (contents);
	} else {
		g_warning ("Failed to read input level at %s: %s",
			   drv_data->light_path, error->message);
		g_error_free (error);
		return G_SOURCE_CONTINUE;
	}

	readings.level = level;
	readings.uses_lux = FALSE;
	drv_data->callback_func (&hwmon_light, (gpointer) &readings, drv_data->user_data);

	return G_SOURCE_CONTINUE;
}

static gboolean
hwmon_light_open (GUdevDevice        *device,
		 ReadingsUpdateFunc  callback_func,
		 gpointer            user_data)
{
	drv_data = g_new0 (DrvData, 1);
	drv_data->callback_func = callback_func;
	drv_data->user_data = user_data;

	drv_data->light_path = g_build_filename (g_udev_device_get_sysfs_path (device),
						 "light", NULL);

	return TRUE;
}

static void
hwmon_light_set_polling (gboolean state)
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
		drv_data->timeout_id = g_timeout_add (DEFAULT_POLL_TIME, (GSourceFunc) light_changed, NULL);
		g_source_set_name_by_id (drv_data->timeout_id, "[hwmon_light_set_polling] light_changed");

		/* And send a reading straight away */
		light_changed ();
	}
}

static void
hwmon_light_close (void)
{
	hwmon_light_set_polling (FALSE);
	g_clear_pointer (&drv_data->light_path, g_free);
	g_clear_pointer (&drv_data, g_free);
}

SensorDriver hwmon_light = {
	.name = "Platform HWMon Light",
	.type = DRIVER_TYPE_LIGHT,
	.specific_type = DRIVER_TYPE_LIGHT_HWMON,

	.discover = hwmon_light_discover,
	.open = hwmon_light_open,
	.set_polling = hwmon_light_set_polling,
	.close = hwmon_light_close,
};
