/*
 * Copyright (c) 2019 Luís Ferreira <luis@aurorafoss.org>
 * Copyright (c) 2019 Daniel Stuart <daniel.stuart@pucpr.edu.br>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include "accel-attributes.h"

AccelLocation
setup_accel_location (GUdevDevice *device)
{
	AccelLocation ret;
	const char *location;

	location = g_udev_device_get_property (device, "ACCEL_LOCATION");
	if (location) {
		if (parse_accel_location (location, &ret))
			return ret;

		g_warning ("Failed to parse ACCEL_LOCATION ('%s') from udev",
			   location);
	}
	location = g_udev_device_get_sysfs_attr (device, "location");
	if (location) {
		if (parse_accel_location (location, &ret))
			return ret;

		g_warning ("Failed to parse location ('%s') from sysfs",
			   location);
	}
	g_debug ("No auto-detected location, falling back to display location");

	ret = ACCEL_LOCATION_DISPLAY;
	return ret;
}

gboolean
parse_accel_location (const char *location, AccelLocation *value)
{
	/* Empty string means we use the display location */
	if (location == NULL ||
	    *location == '\0' ||
	    g_str_equal (location, "display") ||
	    g_str_equal (location, "lid")) {
		*value = ACCEL_LOCATION_DISPLAY;
		return TRUE;
	} else if (g_str_equal (location, "base")) {
		*value = ACCEL_LOCATION_BASE;
		return TRUE;
	} else {
		g_warning ("Failed to parse '%s' as a location", location);
		return FALSE;
	}
}

gdouble
get_accel_scale (GUdevDevice *device)
{
	gdouble scale;

	scale = g_udev_device_get_sysfs_attr_as_double (device, "in_accel_scale");
	if (scale != 0.0) {
		g_debug ("Attribute in_accel_scale ('%f') found on sysfs", scale);
		return scale;
	}
	scale = g_udev_device_get_sysfs_attr_as_double (device, "scale");
	if (scale != 0.0) {
		g_debug ("Attribute scale ('%f') found on sysfs", scale);
		return scale;
	}

	g_debug ("Failed to auto-detect scale, falling back to 1.0");
	return 1.0;
}
