/*
 * Copyright (c) 2016 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include <glib.h>

typedef struct {
	float x;
	float y;
	float z;
} AccelVec3;

gboolean parse_mount_matrix (const char *mtx,
                             AccelVec3  *vecs[3]);

gboolean apply_mount_matrix (const AccelVec3  vecs[3],
                             AccelVec3       *accel);
