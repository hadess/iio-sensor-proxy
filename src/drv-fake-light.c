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

#include <linux/input.h>

typedef struct DrvData {
	ReadingsUpdateFunc callback_func;
	gpointer           user_data;

	guint              timeout_id;
} DrvData;

static DrvData *drv_data = NULL;

static gboolean
fake_light_discover (GUdevDevice *device)
{
	if (g_getenv ("FAKE_LIGHT_SENSOR") == NULL)
		return FALSE;

	if (g_strcmp0 (g_udev_device_get_subsystem (device), "input") != 0)
		return FALSE;

	/* "Lid switch" is a random input device to latch onto */
	if (g_strcmp0 (g_udev_device_get_property (device, "NAME"), "\"Lid Switch\"") == 0)
		return FALSE;

	g_debug ("Found fake light at %s", g_udev_device_get_sysfs_path (device));
	return TRUE;
}

static gboolean
light_changed (void)
{
	static gdouble level = -1.0;
	LightReadings readings;

	/* XXX:
	 * Might need to do something better here, like
	 * replicate real readings from a device */
	level += 1.0;
	readings.level = level;
	readings.uses_lux = TRUE;
	drv_data->callback_func (&fake_light, (gpointer) &readings, drv_data->user_data);

	return G_SOURCE_CONTINUE;
}

static gboolean
first_values (gpointer user_data)
{
	light_changed ();
	drv_data->timeout_id = g_timeout_add_seconds (1, (GSourceFunc) light_changed, NULL);
	g_source_set_name_by_id (drv_data->timeout_id, "[fake_light_set_polling] light_changed");
	return G_SOURCE_REMOVE;
}

static gboolean
fake_light_open (GUdevDevice        *device,
		 ReadingsUpdateFunc  callback_func,
		 gpointer            user_data)
{
	drv_data = g_new0 (DrvData, 1);
	drv_data->callback_func = callback_func;
	drv_data->user_data = user_data;

	return TRUE;
}

static void
fake_light_set_polling (gboolean state)
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
		drv_data->timeout_id = g_idle_add (first_values, NULL);
		g_source_set_name_by_id (drv_data->timeout_id, "[fake_light_set_polling] first_values");
	}
}

static void
fake_light_close (void)
{
	fake_light_set_polling (FALSE);
	g_clear_pointer (&drv_data, g_free);
}

SensorDriver fake_light = {
	.name = "Fake light",
	.type = DRIVER_TYPE_LIGHT,
	.specific_type = DRIVER_TYPE_LIGHT_FAKE,

	.discover = fake_light_discover,
	.open = fake_light_open,
	.set_polling = fake_light_set_polling,
	.close = fake_light_close,
};
