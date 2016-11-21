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
} IioAccelVec3;

gboolean parse_mount_matrix (const char   *mtx,
                             IioAccelVec3 *vecs[3]);

gboolean apply_mount_matrix (const IioAccelVec3 vecs[3],
                             IioAccelVec3       *accel);
