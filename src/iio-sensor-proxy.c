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

#include <gio/gio.h>
#include <gudev/gudev.h>
#include "drivers.h"

#define SENSOR_PROXY_DBUS_NAME "net.hadess.SensorProxy"
#define SENSOR_PROXY_DBUS_PATH "/net/hadess/SensorProxy"

static const gchar introspection_xml[] =
"<node>"
"  <interface name='net.hadess.SensorProxy'>"
"    <property name='HasAccelerometer' type='b' access='read'/>"
"    <property name='AccelerometerOrientation' type='s' access='read'/>"
"  </interface>"
"</node>";

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
	GDBusNodeInfo *introspection_data;
	GDBusConnection *connection;
	guint name_id;

	/* Accelerometer */
	SensorDriver *driver;
	int accel_x, accel_y, accel_z;
	OrientationUp previous_orientation;
} OrientationData;

static const SensorDriver * const drivers[] = {
	&iio_buffer_accel,
	&iio_poll_accel,
	&input_accel
};

static GUdevDevice *
find_accel (GUdevClient   *client,
	    SensorDriver **driver)
{
	GList *devices, *input, *l;
	GUdevDevice *ret = NULL;

	*driver = NULL;
	devices = g_udev_client_query_by_subsystem (client, "iio");
	input = g_udev_client_query_by_subsystem (client, "input");
	devices = g_list_concat (devices, input);

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

static void
send_dbus_event (OrientationData *data)
{
	GVariantBuilder props_builder;
	GVariant *props_changed = NULL;

	g_assert (data->connection);

	g_variant_builder_init (&props_builder, G_VARIANT_TYPE ("a{sv}"));

	g_variant_builder_add (&props_builder, "{sv}", "HasAccelerometer",
			       g_variant_new_boolean (data->driver != NULL));
	g_variant_builder_add (&props_builder, "{sv}", "AccelerometerOrientation",
			       g_variant_new_string (orientation_to_string (data->previous_orientation)));

	props_changed = g_variant_new ("(s@a{sv}@as)", SENSOR_PROXY_DBUS_NAME,
				       g_variant_builder_end (&props_builder),
				       g_variant_new_strv (NULL, 0));

	g_dbus_connection_emit_signal (data->connection,
				       NULL,
				       SENSOR_PROXY_DBUS_PATH,
				       "org.freedesktop.DBus.Properties",
				       "PropertiesChanged",
				       props_changed, NULL);
}

static GVariant *
handle_get_property (GDBusConnection *connection,
		     const gchar     *sender,
		     const gchar     *object_path,
		     const gchar     *interface_name,
		     const gchar     *property_name,
		     GError         **error,
		     gpointer         user_data)
{
	OrientationData *data = user_data;

	g_assert (data->connection);

	if (g_strcmp0 (property_name, "HasAccelerometer") == 0)
		return g_variant_new_boolean (data->driver != NULL);
	if (g_strcmp0 (property_name, "AccelerometerOrientation") == 0)
		return g_variant_new_string (orientation_to_string (data->previous_orientation));

	return NULL;
}

static const GDBusInterfaceVTable interface_vtable =
{
	NULL,
	handle_get_property,
	NULL
};

static void
name_lost_handler (GDBusConnection *connection,
		   const gchar *name,
		   gpointer user_data)
{
	g_warning ("Failed to setup D-Bus");
	exit (1);
}

static gboolean
setup_dbus (OrientationData *data)
{
	GError *error = NULL;

	data->introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
	g_assert (data->introspection_data != NULL);

	data->connection = g_bus_get_sync (G_BUS_TYPE_SYSTEM,
					   NULL,
					   &error);
	if (!data->connection) {
		g_warning ("Failed to setup D-Bus: %s", error->message);
		g_clear_error (&error);
		return FALSE;
	}

	g_dbus_connection_register_object (data->connection,
					   SENSOR_PROXY_DBUS_PATH,
					   data->introspection_data->interfaces[0],
					   &interface_vtable,
					   data,
					   NULL,
					   NULL);

	data->name_id = g_bus_own_name_on_connection (data->connection,
						      SENSOR_PROXY_DBUS_NAME,
						      G_BUS_NAME_OWNER_FLAGS_NONE,
						      NULL,
						      name_lost_handler,
						      NULL,
						      NULL);

	return TRUE;
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
	g_debug ("Sent by driver (quirk applied): %d, %d, %d", accel_x, accel_y, accel_z);

	orientation = orientation_calc (data->previous_orientation, accel_x, accel_y, accel_z);

	data->accel_x = accel_x;
	data->accel_y = accel_y;
	data->accel_z = accel_z;

	if (data->previous_orientation != orientation) {
		OrientationUp tmp;

		tmp = data->previous_orientation;
		data->previous_orientation = orientation;
		send_dbus_event (data);
		g_debug ("Emitted orientation changed: from %s to %s",
			 orientation_to_string (tmp),
			 orientation_to_string (data->previous_orientation));
	}
}

static void
free_orientation_data (OrientationData *data)
{
	if (data == NULL)
		return;

	if (data->name_id != 0) {
		g_bus_unown_name (data->name_id);
		data->name_id = 0;
	}
	g_clear_pointer (&data->introspection_data, g_dbus_node_info_unref);
	g_clear_object (&data->connection);
	g_clear_pointer (&data->loop, g_main_loop_unref);
	g_free (data);
}

int main (int argc, char **argv)
{
	OrientationData *data;
	GUdevClient *client;
	GUdevDevice *dev;
	SensorDriver *driver;
	int ret = 0;

	/* g_setenv ("G_MESSAGES_DEBUG", "all", TRUE); */

	client = g_udev_client_new (NULL);
	dev = find_accel (client, &driver);
	if (!dev) {
		g_debug ("Could not find IIO accelerometer");
		return 0;
	}

	data = g_new0 (OrientationData, 1);
	data->previous_orientation = ORIENTATION_UNDEFINED;
	data->driver = driver;

	/* Open up the accelerometer */
	if (!data->driver->open (dev,
				 accel_changed_func,
				 data)) {
		ret = 1;
		goto out;
	}

	/* Set up D-Bus */
	if (!setup_dbus (data)) {
		ret = 1;
		goto out;
	}
	send_dbus_event (data);

	data->loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (data->loop);

out:
	data->driver->close ();

	g_object_unref (dev);
	free_orientation_data (data);

	return ret;
}
