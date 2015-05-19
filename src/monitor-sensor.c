/*
 * Copyright (c) 2015 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 */

#include <gio/gio.h>

static GMainLoop *loop;
static GDBusProxy *iio_proxy;
static gboolean accel_claimed, als_claimed;

static void
properties_changed (GDBusProxy *proxy,
		    GVariant   *changed_properties,
		    GStrv       invalidated_properties,
		    gpointer    user_data)
{
	GError *error = NULL;
	gboolean has_accel, has_als;
	GVariant *v;

	has_accel = has_als = FALSE;

	v = g_dbus_proxy_get_cached_property (iio_proxy, "HasAccelerometer");
	if (v) {
		has_accel = g_variant_get_boolean (v);
		g_variant_unref (v);
	}
	if (has_accel && !accel_claimed) {
		g_dbus_proxy_call_sync (iio_proxy,
					"Claim",
					g_variant_new ("(&s)", "accel"),
					G_DBUS_CALL_FLAGS_NONE,
					-1,
					NULL, &error);
		g_assert_no_error (error);
		accel_claimed = TRUE;
	}

	v = g_dbus_proxy_get_cached_property (iio_proxy, "HasAmbientLight");
	if (v) {
		has_als = g_variant_get_boolean (v);
		g_variant_unref (v);
	}
	if (has_als && !als_claimed) {
		g_dbus_proxy_call_sync (iio_proxy,
					"Claim",
					g_variant_new ("(&s)", "light"),
					G_DBUS_CALL_FLAGS_NONE,
					-1,
					NULL, &error);
		g_assert_no_error (error);
		als_claimed = TRUE;
	}
}

int main (int argc, char **argv)
{
	iio_proxy = g_dbus_proxy_new_for_bus_sync (G_BUS_TYPE_SYSTEM,
						   G_DBUS_PROXY_FLAGS_NONE,
						   NULL,
						   "net.hadess.SensorProxy",
						   "/net/hadess/SensorProxy",
						   "net.hadess.SensorProxy",
						   NULL, NULL);

	if (!iio_proxy) {
		g_message ("iio-sensor-proxy not running");
		return 0;
	}

	g_signal_connect (G_OBJECT (iio_proxy), "g-properties-changed",
			  G_CALLBACK (properties_changed), NULL);

	accel_claimed = als_claimed = FALSE;

	properties_changed (iio_proxy, NULL, NULL, NULL);

	loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (loop);

	return 0;
}
