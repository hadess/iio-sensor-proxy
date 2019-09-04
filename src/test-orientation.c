/*
 * Copyright (c) 2016 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include <glib.h>
#include <stdlib.h>
#include "orientation.h"
#include "accel-mount-matrix.h"

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
		/* Fail straight away when not verbose */
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

static void
test_mount_matrix_orientation (void)
{
	guint i;
	struct {
		AccelVec3 readings;
		gdouble scale;
		const char *mount_matrix;
		OrientationUp expected;
	} tests[] = {
		/* Onda v975 quirking */
		{ { 523, 13, 5 }, 0.019163, "0, -1, 0; -1, 0, 0; 0, 0, 1", ORIENTATION_NORMAL },
		{ { 8, 521, -67 }, 0.019163, "0, -1, 0; -1, 0, 0; 0, 0, 1", ORIENTATION_RIGHT_UP },
		/* Winbook TW100 quirking */
		{ { 24, 0, -21 }, 0.306457, "0, -1, 0; -1, 0, 0; 0, 0, 1", ORIENTATION_NORMAL },
		{ { 15, -25, -14 }, 0.306457, "0, -1, 0; -1, 0, 0; 0, 0, 1", ORIENTATION_LEFT_UP }
	};

	for (i = 0; i < G_N_ELEMENTS (tests); i++) {
		AccelVec3 *vecs;
		const char *result, *expected;
		OrientationUp o;

		g_assert_true (parse_mount_matrix (tests[i].mount_matrix, &vecs));
		g_assert_true (apply_mount_matrix (vecs, &tests[i].readings));
		o = orientation_calc (ORIENTATION_UNDEFINED,
				      tests[i].readings.x,
				      tests[i].readings.y,
				      tests[i].readings.z,
				      tests[i].scale);
		result = orientation_to_string (o);
		expected = orientation_to_string (tests[i].expected);
		g_assert_cmpstr (result, ==, expected);
		g_free (vecs);
	}
}

static gboolean
print_orientation (const char *x_str,
		   const char *y_str,
		   const char *z_str,
		   const char *scale_str,
		   const char *mount_matrix)
{
	int x, y, z;
	gdouble scale;
	OrientationUp o;

	if (scale_str == NULL)
		scale = 1.0;
	else
		scale = g_strtod (scale_str, NULL);

	x = atoi (x_str);
	y = atoi (y_str);
	z = atoi (z_str);

	if (mount_matrix) {
		AccelVec3 *vecs;
		AccelVec3 readings;

		if (!parse_mount_matrix (mount_matrix, &vecs)) {
			g_printerr ("Could not parse mount matrix '%s'\n",
				    mount_matrix);
			return FALSE;
		}

		readings.x = x;
		readings.y = y;
		readings.z = z;

		if (!apply_mount_matrix (vecs, &readings)) {
			g_printerr ("Could not apply mount matrix '%s'\n",
				    mount_matrix);
			return FALSE;
		}

		x = readings.x;
		y = readings.y;
		z = readings.z;

		g_free (vecs);
	}

	o = orientation_calc (ORIENTATION_UNDEFINED, x, y, z, scale);
	g_print ("Orientation for %d,%d,%d (scale: %lf) is '%s'\n",
		 x, y, z, scale, orientation_to_string (o));

	return TRUE;
}

int main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	if (argc > 1) {
		gboolean ret;

		if (argc == 6) {
			ret = print_orientation (argv[1], argv[2], argv[3], argv[4], argv[5]);
		} else if (argc == 5) {
			ret = print_orientation (argv[1], argv[2], argv[3], argv[4], NULL);
		} else if (argc == 4) {
			ret = print_orientation (argv[1], argv[2], argv[3], "1.0", NULL);
		} else {
			g_printerr ("Usage: %s X Y Z [scale] [mount-matrix]\n", argv[0]);
			ret = FALSE;
		}
		return (ret == FALSE ? 1 : 0);
	}

	g_test_add_func ("/iio-sensor-proxy/orientation", test_orientation);
	g_test_add_func ("/iio-sensor-proxy/quirking", test_mount_matrix_orientation);

	return g_test_run ();
}
