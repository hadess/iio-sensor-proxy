/*
 * Copyright (c) 2015 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include <gio/gio.h>

static GMainLoop *loop;
static guint watch_id;
static GDBusProxy *iio_proxy, *iio_proxy_compass;

static void
properties_changed (GDBusProxy *proxy,
		    GVariant   *changed_properties,
		    GStrv       invalidated_properties,
		    gpointer    user_data)
{
	GVariant *v;
	GVariantDict dict;

	g_variant_dict_init (&dict, changed_properties);

	if (g_variant_dict_contains (&dict, "HasAccelerometer")) {
		v = g_dbus_proxy_get_cached_property (iio_proxy, "HasAccelerometer");
		g_message ("Accelerometer %s", g_variant_get_boolean (v) ? "appeared" : "disappeared");
		g_variant_unref (v);
	}
	if (g_variant_dict_contains (&dict, "AccelerometerOrientation")) {
		v = g_dbus_proxy_get_cached_property (iio_proxy, "AccelerometerOrientation");
		g_message ("Accelerometer orientation changed: %s", g_variant_get_string (v, NULL));
		g_variant_unref (v);
	}
	if (g_variant_dict_contains (&dict, "HasAmbientLight")) {
		v = g_dbus_proxy_get_cached_property (iio_proxy, "HasAmbientLight");
		g_message ("Light sensor %s", g_variant_get_boolean (v) ? "appeared" : "disappeared");
		g_variant_unref (v);
	}
	if (g_variant_dict_contains (&dict, "LightLevel")) {
		GVariant *unit;

		v = g_dbus_proxy_get_cached_property (iio_proxy, "LightLevel");
		unit = g_dbus_proxy_get_cached_property (iio_proxy, "LightLevelUnit");
		g_message ("Light changed: %lf (%s)", g_variant_get_double (v), g_variant_get_string (unit, NULL));
		g_variant_unref (v);
		g_variant_unref (unit);
	}
	if (g_variant_dict_contains (&dict, "HasCompass")) {
		v = g_dbus_proxy_get_cached_property (iio_proxy, "HasCompass");
		g_message ("Compass %s", g_variant_get_boolean (v) ? "appeared" : "disappeared");
		g_variant_unref (v);
	}
	if (g_variant_dict_contains (&dict, "CompassHeading")) {
		v = g_dbus_proxy_get_cached_property (iio_proxy, "CompassHeading");
		g_message ("Compass heading changed: %lf", g_variant_get_double (v));
		g_variant_unref (v);
	}
}

static void
appeared_cb (GDBusConnection *connection,
	     const gchar     *name,
	     const gchar     *name_owner,
	     gpointer         user_data)
{
	GError *error = NULL;

	iio_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
						   G_DBUS_PROXY_FLAGS_NONE,
						   NULL,
						   "net.hadess.SensorProxy",
						   "/net/hadess/SensorProxy",
						   "net.hadess.SensorProxy",
						   NULL, NULL);

	g_signal_connect (G_OBJECT (iio_proxy), "g-properties-changed",
			  G_CALLBACK (properties_changed), NULL);

	if (g_strcmp0 (g_get_user_name (), "geoclue") == 0) {
		iio_proxy_compass = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
								   G_DBUS_PROXY_FLAGS_NONE,
								   NULL,
								   "net.hadess.SensorProxy",
								   "/net/hadess/SensorProxy/Compass",
								   "net.hadess.SensorProxy.Compass",
								   NULL, NULL);

		g_signal_connect (G_OBJECT (iio_proxy_compass), "g-properties-changed",
				  G_CALLBACK (properties_changed), NULL);
	}

	/* Accelerometer */
	g_dbus_proxy_call_sync (iio_proxy,
				"ClaimAccelerometer",
				NULL,
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL, &error);
	g_assert_no_error (error);

	/* ALS */
	g_dbus_proxy_call_sync (iio_proxy,
				"ClaimLight",
				NULL,
				G_DBUS_CALL_FLAGS_NONE,
				-1,
				NULL, &error);
	g_assert_no_error (error);

	/* Compass */
	if (g_strcmp0 (g_get_user_name (), "geoclue") == 0) {
		g_dbus_proxy_call_sync (iio_proxy_compass,
					"ClaimCompass",
					NULL,
					G_DBUS_CALL_FLAGS_NONE,
					-1,
					NULL, &error);
		g_assert_no_error (error);
	}
}

static void
vanished_cb (GDBusConnection *connection,
	     const gchar *name,
	     gpointer user_data)
{
	g_clear_object (&iio_proxy);
	g_clear_object (&iio_proxy_compass);
}

int main (int argc, char **argv)
{
	watch_id = g_bus_watch_name (G_BUS_TYPE_SYSTEM,
				     "net.hadess.SensorProxy",
				     G_BUS_NAME_WATCHER_FLAGS_NONE,
				     appeared_cb,
				     vanished_cb,
				     NULL, NULL);

	loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (loop);

	return 0;
}
