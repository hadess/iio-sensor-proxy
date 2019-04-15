/*
 * Copyright (c) 2019 Luís Ferreira <luis@aurorafoss.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include "accel-location.h"

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
	} else {
		g_debug ("No autodetected location, falling back to display location");
	}

	ret = ACCEL_LOCATION_DISPLAY;
	return ret;
}

gboolean
parse_accel_location (const char *location, AccelLocation *value)
{
	/* Empty string means we use the display location */
	if (location == NULL ||
		*location == '\0' ||
		(g_strcmp0 (location, "display") == 0)) {
		*value = ACCEL_LOCATION_DISPLAY;
		return TRUE;
	} else if (g_strcmp0 (location, "base") == 0) {
		*value = ACCEL_LOCATION_BASE;
		return TRUE;
	} else {
		g_warning ("Failed to parse '%s' as a location", location);
		return FALSE;
	}
}