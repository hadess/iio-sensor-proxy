/*
 * Copyright (c) 2016 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include <glib.h>
#include "orientation.h"

#define ONEG 256

static void
test_orientation (void)
{
	static struct {
		int x;
		int y;
		int z;
		OrientationUp expected;
	} orientations[] = {
		{ 0, -ONEG, 0, ORIENTATION_NORMAL },
		{ -ONEG, 0, 0, ORIENTATION_RIGHT_UP },
		{ ONEG, 0, 0, ORIENTATION_LEFT_UP },
		{ 0, ONEG, 0, ORIENTATION_BOTTOM_UP }
	};
	guint i, num_failures;

	num_failures = 0;

	for (i = 0; i < G_N_ELEMENTS (orientations); i++) {
		OrientationUp o;
		const char *expected, *result;

		o = orientation_calc (ORIENTATION_UNDEFINED,
				      orientations[i].x,
				      orientations[i].y,
				      orientations[i].z,
				      9.81 / ONEG);
		result = orientation_to_string (o);
		expected = orientation_to_string (orientations[i].expected);
		/* Fail straight away when verbose */
		if (g_test_verbose ()) {
			if (g_strcmp0 (result, expected) != 0) {
				g_test_message ("Expected %s, got %s", expected, result);
				num_failures++;
			}
		} else {
			g_assert_cmpstr (result, ==, expected);
		}
	}

	if (num_failures > 0)
		g_test_fail ();
}

int main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/iio-sensor-proxy/orientation", test_orientation);

	return g_test_run ();
}
