/*
 * Copyright (c) 2014 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 */

#include <glib.h>
#include <gudev/gudev.h>

typedef enum {
	DRIVER_TYPE_ACCEL_IIO
} DriverType;

typedef struct SensorDriver SensorDriver;

typedef void (*ReadingsUpdateFunc) (SensorDriver *driver,
				    int           x,
				    int           y,
				    int           z,
				    gpointer      user_data);

struct SensorDriver {
	const char *name;
	DriverType  type;

	gboolean (*discover) (GUdevDevice        *device);
	gboolean (*open)     (GUdevDevice        *device,
			      ReadingsUpdateFunc  callback_func,
			      gpointer            user_data);
	void     (*close)    (void);
};

extern SensorDriver iio_buffer_accel;
