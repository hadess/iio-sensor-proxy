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
	DRIVER_TYPE_ACCEL,
	DRIVER_TYPE_LIGHT
} DriverType;

/* Driver types */
typedef guint DriverSpecificType;

typedef enum {
	DRIVER_TYPE_ACCEL_IIO,
	DRIVER_TYPE_ACCEL_INPUT
} DriverAccelType;

typedef enum {
	DRIVER_TYPE_LIGHT_IIO,
	DRIVER_TYPE_LIGHT_FAKE,
	DRIVER_TYPE_LIGHT_HWMON
} DriverLightType;

typedef struct SensorDriver SensorDriver;

typedef struct {
	int accel_x;
	int accel_y;
	int accel_z;
} AccelReadings;

typedef struct {
	gdouble  level;
	gboolean uses_lux;
} LightReadings;

typedef void (*ReadingsUpdateFunc) (SensorDriver *driver,
				    gpointer      readings,
				    gpointer      user_data);

struct SensorDriver {
	const char             *name;
	DriverType              type;
	DriverSpecificType      specific_type;

	gboolean (*discover)    (GUdevDevice        *device);
	gboolean (*open)        (GUdevDevice        *device,
			         ReadingsUpdateFunc  callback_func,
			         gpointer            user_data);
	void     (*set_polling) (gboolean            state);
	void     (*close)       (void);
};

extern SensorDriver iio_buffer_accel;
extern SensorDriver iio_poll_accel;
extern SensorDriver input_accel;
extern SensorDriver fake_light;
extern SensorDriver iio_poll_light;
extern SensorDriver hwmon_light;
extern SensorDriver iio_buffer_light;
