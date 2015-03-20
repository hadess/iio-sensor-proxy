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
#include <stdio.h>

#include <gio/gio.h>
#include <gudev/gudev.h>
#include "drivers.h"
#include "orientation.h"

#define SENSOR_PROXY_DBUS_NAME "net.hadess.SensorProxy"
#define SENSOR_PROXY_DBUS_PATH "/net/hadess/SensorProxy"

static const gchar introspection_xml[] =
"<node>"
"  <interface name='net.hadess.SensorProxy'>"
"    <property name='HasAccelerometer' type='b' access='read'/>"
"    <property name='AccelerometerOrientation' type='s' access='read'/>"
"    <property name='HasAmbientLight' type='b' access='read'/>"
"    <property name='LightLevelUnit' type='s' access='read'/>"
"    <property name='LightLevel' type='d' access='read'/>"
"  </interface>"
"</node>";

#define NUM_SENSOR_TYPES DRIVER_TYPE_LIGHT + 1

typedef struct {
	GMainLoop *loop;
	GDBusNodeInfo *introspection_data;
	GDBusConnection *connection;
	guint name_id;
	gboolean init_done;

	SensorDriver *drivers[NUM_SENSOR_TYPES];
	GUdevDevice  *devices[NUM_SENSOR_TYPES];

	/* Accelerometer */
	int accel_x, accel_y, accel_z;
	OrientationUp previous_orientation;

	/* Light */
	gdouble previous_level;
	gboolean uses_lux;
} SensorData;

static const SensorDriver * const drivers[] = {
	&iio_buffer_accel,
	&iio_poll_accel,
	&input_accel,
	&iio_poll_light,
	&hwmon_light,
	&fake_light
};

static const char *
driver_type_to_str (DriverType type)
{
	switch (type) {
	case DRIVER_TYPE_ACCEL:
		return "accelerometer";
	case DRIVER_TYPE_LIGHT:
		return "ambient light sensor";
	default:
		g_assert_not_reached ();
	}
}

static gboolean
find_sensors (GUdevClient *client,
	      SensorData  *data)
{
	GList *devices, *input, *platform, *l;
	gboolean found = FALSE;

	devices = g_udev_client_query_by_subsystem (client, "iio");
	input = g_udev_client_query_by_subsystem (client, "input");
	platform = g_udev_client_query_by_subsystem (client, "platform");
	devices = g_list_concat (devices, input);
	devices = g_list_concat (devices, platform);

	/* Find the devices */
	for (l = devices; l != NULL; l = l->next) {
		GUdevDevice *dev = l->data;
		guint i;

		for (i = 0; i < G_N_ELEMENTS(drivers); i++) {
			SensorDriver *driver = (SensorDriver *) drivers[i];
			if (data->drivers[driver->type] == NULL &&
			    driver->discover (dev)) {
				g_debug ("Found device %s of type %s at %s",
					 g_udev_device_get_sysfs_path (dev),
					 driver_type_to_str (driver->type),
					 driver->name);
				data->devices[driver->type] = g_object_ref (dev);
				data->drivers[driver->type] = (SensorDriver *) driver;

				found = TRUE;
			}
		}

		if (data->drivers[DRIVER_TYPE_ACCEL] &&
		    data->drivers[DRIVER_TYPE_LIGHT])
			break;
	}

	g_list_free_full (devices, g_object_unref);
	return found;
}

static void
send_dbus_event (SensorData *data)
{
	GVariantBuilder props_builder;
	GVariant *props_changed = NULL;

	g_assert (data->connection);

	g_variant_builder_init (&props_builder, G_VARIANT_TYPE ("a{sv}"));

	g_variant_builder_add (&props_builder, "{sv}", "HasAccelerometer",
			       g_variant_new_boolean (data->drivers[DRIVER_TYPE_ACCEL] != NULL));
	g_variant_builder_add (&props_builder, "{sv}", "AccelerometerOrientation",
			       g_variant_new_string (orientation_to_string (data->previous_orientation)));
	g_variant_builder_add (&props_builder, "{sv}", "HasAmbientLight",
			       g_variant_new_boolean (data->drivers[DRIVER_TYPE_LIGHT] != NULL));
	g_variant_builder_add (&props_builder, "{sv}", "LightLevelUnit",
			       g_variant_new_string (data->uses_lux ? "lux" : "vendor"));
	g_variant_builder_add (&props_builder, "{sv}", "LightLevel",
			       g_variant_new_double (data->previous_level));

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
	SensorData *data = user_data;

	g_assert (data->connection);

	if (g_strcmp0 (property_name, "HasAccelerometer") == 0)
		return g_variant_new_boolean (data->drivers[DRIVER_TYPE_ACCEL] != NULL);
	if (g_strcmp0 (property_name, "AccelerometerOrientation") == 0)
		return g_variant_new_string (orientation_to_string (data->previous_orientation));
	if (g_strcmp0 (property_name, "HasAmbientLight") == 0)
		return g_variant_new_boolean (data->drivers[DRIVER_TYPE_LIGHT] != NULL);
	if (g_strcmp0 (property_name, "LightLevelUnit") == 0)
		return g_variant_new_string (data->uses_lux ? "lux" : "vendor");
	if (g_strcmp0 (property_name, "LightLevel") == 0)
		return g_variant_new_double (data->previous_level);

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
		   const gchar     *name,
		   gpointer         user_data)
{
	g_debug ("iio-sensor-proxy is already running");
	exit (0);
}

static void
bus_acquired_handler (GDBusConnection *connection,
		      const gchar     *name,
		      gpointer         user_data)
{
	SensorData *data = user_data;

	g_dbus_connection_register_object (connection,
					   SENSOR_PROXY_DBUS_PATH,
					   data->introspection_data->interfaces[0],
					   &interface_vtable,
					   data,
					   NULL,
					   NULL);

	data->connection = g_object_ref (connection);
}

static void
name_acquired_handler (GDBusConnection *connection,
		       const gchar     *name,
		       gpointer         user_data)
{
	SensorData *data = user_data;

	if (data->init_done)
		send_dbus_event (data);
}

static gboolean
setup_dbus (SensorData *data)
{
	data->introspection_data = g_dbus_node_info_new_for_xml (introspection_xml, NULL);
	g_assert (data->introspection_data != NULL);

	data->name_id = g_bus_own_name (G_BUS_TYPE_SYSTEM,
					SENSOR_PROXY_DBUS_NAME,
					G_BUS_NAME_OWNER_FLAGS_NONE,
					bus_acquired_handler,
					name_acquired_handler,
					name_lost_handler,
					data,
					NULL);

	return TRUE;
}

static void
accel_changed_func (SensorDriver *driver,
		    gpointer      readings_data,
		    gpointer      user_data)
{
	SensorData *data = user_data;
	AccelReadings *readings = (AccelReadings *) readings_data;
	OrientationUp orientation = data->previous_orientation;

	//FIXME handle errors
	g_debug ("Accel sent by driver (quirk applied): %d, %d, %d", readings->accel_x, readings->accel_y, readings->accel_z);

	orientation = orientation_calc (data->previous_orientation, readings->accel_x, readings->accel_y, readings->accel_z);

	data->accel_x = readings->accel_x;
	data->accel_y = readings->accel_y;
	data->accel_z = readings->accel_z;

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
light_changed_func (SensorDriver *driver,
		    gpointer      readings_data,
		    gpointer      user_data)
{
	SensorData *data = user_data;
	LightReadings *readings = (LightReadings *) readings_data;

	//FIXME handle errors
	g_debug ("Light level sent by driver (quirk applied): %lf (unit: %s)",
		 readings->level, data->uses_lux ? "lux" : "vendor");

	if (data->previous_level != readings->level ||
	    data->uses_lux != readings->uses_lux) {
		gdouble tmp;

		tmp = data->previous_level;
		data->previous_level = readings->level;

		data->uses_lux = readings->uses_lux;

		send_dbus_event (data);
		g_debug ("Emitted light changed: from %lf to %lf",
			 tmp, data->previous_level);
	}
}

static ReadingsUpdateFunc
driver_type_to_callback_func (DriverType type)
{
	switch (type) {
	case DRIVER_TYPE_ACCEL:
		return accel_changed_func;
	case DRIVER_TYPE_LIGHT:
		return light_changed_func;
	default:
		g_assert_not_reached ();
	}
}

static void
free_orientation_data (SensorData *data)
{
	guint i;

	if (data == NULL)
		return;

	if (data->name_id != 0) {
		g_bus_unown_name (data->name_id);
		data->name_id = 0;
	}

	for (i = 0; i < NUM_SENSOR_TYPES; i++) {
		if (data->drivers[i] != NULL)
			data->drivers[i]->close ();
		g_clear_object (&data->devices[i]);
	}

	g_clear_pointer (&data->introspection_data, g_dbus_node_info_unref);
	g_clear_object (&data->connection);
	g_clear_pointer (&data->loop, g_main_loop_unref);
	g_free (data);
}

static gboolean
any_sensors_left (SensorData *data)
{
	guint i;
	gboolean exists = FALSE;

	for (i = 0; i < NUM_SENSOR_TYPES; i++) {
		if (data->drivers[i] != NULL) {
			exists = TRUE;
			break;
		}
	}

	return exists;
}

static void
sensor_changes (GUdevClient *client,
		gchar       *action,
		GUdevDevice *device,
		SensorData  *data)
{
	guint i;

	if (g_strcmp0 (action, "remove") == 0) {
		for (i = 0; i < NUM_SENSOR_TYPES; i++) {
			GUdevDevice *dev = data->devices[i];

			if (!dev)
				continue;

			if (g_strcmp0 (g_udev_device_get_sysfs_path (device), g_udev_device_get_sysfs_path (dev)) == 0) {
				g_debug ("Sensor type %s got removed (%s)",
					 driver_type_to_str (i),
					 g_udev_device_get_sysfs_path (dev));
				g_clear_object (&data->devices[i]);
				data->drivers[i] = NULL;
			}
		}

		if (!any_sensors_left (data))
			g_main_loop_quit (data->loop);
	} else if (g_strcmp0 (action, "add") == 0) {
		guint i;

		for (i = 0; i < G_N_ELEMENTS(drivers); i++) {
			SensorDriver *driver = (SensorDriver *) drivers[i];
			if (data->drivers[driver->type] == NULL &&
			    driver->discover (device)) {
				g_debug ("Found hotplugged device %s of type %s at %s",
					 g_udev_device_get_sysfs_path (device),
					 driver_type_to_str (driver->type),
					 driver->name);

				if (driver->open (device,
						  driver_type_to_callback_func (driver->type),
						  data)) {
					data->devices[driver->type] = g_object_ref (device);
					data->drivers[driver->type] = (SensorDriver *) driver;
				}
				break;
			}
		}
	}
}

int main (int argc, char **argv)
{
	SensorData *data;
	GUdevClient *client;
	int ret = 0;
	const gchar * const subsystems[] = { "iio", "input", "platform", NULL };
	guint i;

	/* g_setenv ("G_MESSAGES_DEBUG", "all", TRUE); */

	data = g_new0 (SensorData, 1);
	data->previous_orientation = ORIENTATION_UNDEFINED;
	data->uses_lux = TRUE;

	/* Set up D-Bus */
	setup_dbus (data);

	client = g_udev_client_new (subsystems);
	if (!find_sensors (client, data)) {
		g_debug ("Could not find any supported sensors");
		return 0;
	}
	g_signal_connect (G_OBJECT (client), "uevent",
			  G_CALLBACK (sensor_changes), data);

	for (i = 0; i < NUM_SENSOR_TYPES; i++) {
		if (data->drivers[i] == NULL)
			continue;
		if (!data->drivers[i]->open (data->devices[i],
					     driver_type_to_callback_func (data->drivers[i]->type),
					     data)) {
			data->drivers[i] = NULL;
			g_clear_object (&data->devices[i]);
		}
	}

	if (!any_sensors_left (data))
		goto out;

	data->init_done = TRUE;
	if (data->connection)
		send_dbus_event (data);

	data->loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (data->loop);

out:
	free_orientation_data (data);

	return ret;
}
