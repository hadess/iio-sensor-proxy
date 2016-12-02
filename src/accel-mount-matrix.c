/*
 * Copyright (c) 2016 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include <glib.h>
#include <stdio.h>

#include "accel-mount-matrix.h"

/* The format is the same used in the iio core to export the values:
 * https://git.kernel.org/cgit/linux/kernel/git/torvalds/linux.git/tree/drivers/iio/industrialio-core.c?id=dfc57732ad38f93ae6232a3b4e64fd077383a0f1#n431
 */

static AccelVec3 id_matrix[3] = {
	{ 1.0, 0.0, 0.0 },
	{ 0.0, 1.0, 0.0 },
	{ 0.0, 0.0, 1.0 }
};

gboolean
parse_mount_matrix (const char *mtx,
		    AccelVec3  *vecs[3])
{
	AccelVec3 *ret;

	g_return_val_if_fail (vecs != NULL, FALSE);


	/* Empty string means we use the identity matrix */
	if (mtx == NULL || *mtx == '\0') {
		*vecs = g_memdup (id_matrix, sizeof(id_matrix));
		return TRUE;
	}

	ret = g_new0 (AccelVec3, 3);
	if (sscanf (mtx, "%f, %f, %f; %f, %f, %f; %f, %f, %f",
		    &ret[0].x, &ret[0].y, &ret[0].z,
		    &ret[1].x, &ret[1].y, &ret[1].z,
		    &ret[2].x, &ret[2].y, &ret[2].z) != 9) {
		g_free (ret);
		g_warning ("Failed to parse '%s' as a mount matrix", mtx);
		return FALSE;
	}

	*vecs = ret;

	return TRUE;
}

gboolean
apply_mount_matrix (const AccelVec3  vecs[3],
		    AccelVec3       *accel)
{
	float _x, _y, _z;

	g_return_val_if_fail (accel != NULL, FALSE);

	_x = accel->x * vecs[0].x + accel->y * vecs[0].y + accel->z * vecs[0].z;
	_y = accel->x * vecs[1].x + accel->y * vecs[1].y + accel->z * vecs[1].z;
	_z = accel->x * vecs[2].x + accel->y * vecs[2].y + accel->z * vecs[2].z;

	accel->x = _x;
	accel->y = _y;
	accel->z = _z;

	return TRUE;
}
