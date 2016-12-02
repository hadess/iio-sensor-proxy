/*
 * Copyright (c) 2014-2016 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include "accel-mount-matrix.h"

#define SWAP_Y_Z_MATRIX "1, 0, 0; 0, 0, 1; 0, 1, 0"

static void
print_vecs (AccelVec3 vecs[3])
{
	g_test_message ("%f, %f, %f; %f, %f, %f; %f, %f, %f",
			vecs[0].x, vecs[0].y, vecs[0].z,
			vecs[1].x, vecs[1].y, vecs[1].z,
			vecs[2].x, vecs[2].y, vecs[2].z);
}

static void
test_mount_matrix (void)
{
	AccelVec3 *vecs;
	AccelVec3 test;

	/* Swap Y/Z matrix */
	g_assert_true (parse_mount_matrix (SWAP_Y_Z_MATRIX, &vecs));

	print_vecs (vecs);

	test.x = test.z = 0.0;
	test.y = -256.0;
	g_assert_true (apply_mount_matrix (vecs, &test));

	g_assert_cmpfloat (test.x, ==, 0.0);
	g_assert_cmpfloat (test.z, ==, -256.0);
	g_assert_cmpfloat (test.y, ==, 0.0);
	g_free (vecs);

	/* Identity matrix */
	g_assert_true (parse_mount_matrix ("", &vecs));

	print_vecs (vecs);

	test.x = test.z = 0.0;
	test.y = -256.0;
	g_assert_true (apply_mount_matrix (vecs, &test));

	g_assert_cmpfloat (test.x, ==, 0.0);
	g_assert_cmpfloat (test.z, ==, 0.0);
	g_assert_cmpfloat (test.y, ==, -256.0);
	g_free (vecs);
}

int main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/iio-sensor-proxy/mount-matrix", test_mount_matrix);

	return g_test_run ();
}
