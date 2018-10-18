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
#define INVALID_MATRIX "0, 1, 0; 1, 0, 0; 0, 0, 0"

static void
print_vecs (AccelVec3 vecs[3])
{
	g_test_message ("%f, %f, %f; %f, %f, %f; %f, %f, %f",
			(double) vecs[0].x, (double) vecs[0].y, (double) vecs[0].z,
			(double) vecs[1].x, (double) vecs[1].y, (double) vecs[1].z,
			(double) vecs[2].x, (double) vecs[2].y, (double) vecs[2].z);
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

	/* Invalid matrix */
	g_test_expect_message (NULL, G_LOG_LEVEL_WARNING, "In mount matrix '0, 1, 0; 1, 0, 0; 0, 0, 0', axis z is all zeroes, which is invalid");
	g_assert_false (parse_mount_matrix (INVALID_MATRIX, &vecs));
	g_test_assert_expected_messages ();
}

int main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/iio-sensor-proxy/mount-matrix", test_mount_matrix);

	return g_test_run ();
}
