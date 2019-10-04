/*
 * Copyright (c) 2019 Luís Ferreira <luis@aurorafoss.org>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include "accel-attributes.h"

#define VALID_DISPLAY_LOCATION "display"
#define VALID_BASE_LOCATION "base"
#define INVALID_LOCATION "invalid"

static void
test_accel_location (void)
{
	AccelLocation location;

	/* display location */
	g_assert_true (parse_accel_location (VALID_DISPLAY_LOCATION, &location));
	g_assert_true (location == ACCEL_LOCATION_DISPLAY);

	/* base location */
	g_assert_true (parse_accel_location (VALID_BASE_LOCATION, &location));
	g_assert_true (location == ACCEL_LOCATION_BASE);

	/* default location (display) */
	g_assert_true (parse_accel_location ("", &location));
	g_assert_true (location == ACCEL_LOCATION_DISPLAY);

	/* Invalid matrix */
	g_test_expect_message (NULL, G_LOG_LEVEL_WARNING, "Failed to parse 'invalid' as a location");
	g_assert_false (parse_accel_location (INVALID_LOCATION, &location));
	g_test_assert_expected_messages ();
}

int main (int argc, char **argv)
{
	g_test_init (&argc, &argv, NULL);

	g_test_add_func ("/iio-sensor-proxy/accel-location", test_accel_location);

	return g_test_run ();
}
