/*
 * Copyright (c) 2014 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 */

#include <glib.h>
#include <gudev/gudev.h>

#include "accel-attributes.h"

typedef enum {
	DRIVER_TYPE_ACCEL,
	DRIVER_TYPE_LIGHT,
	DRIVER_TYPE_COMPASS,
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

typedef enum {
  DRIVER_TYPE_COMPASS_IIO,
  DRIVER_TYPE_COMPASS_FAKE
} DriverTypeCompass;

typedef struct SensorDriver SensorDriver;

typedef struct {
	int accel_x;
	int accel_y;
	int accel_z;
	gdouble scale;
} AccelReadings;

typedef struct {
	gdouble  level;
	gboolean uses_lux;
} LightReadings;

typedef struct {
	gdouble heading;
} CompassReadings;

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

static inline gboolean
driver_discover (SensorDriver *driver,
		 GUdevDevice  *device)
{
	g_return_val_if_fail (driver, FALSE);
	g_return_val_if_fail (driver->discover, FALSE);
	g_return_val_if_fail (device, FALSE);

	if (!driver->discover (device))
		return FALSE;

	if (driver->type != DRIVER_TYPE_ACCEL)
		return TRUE;

	return (setup_accel_location (device) == ACCEL_LOCATION_DISPLAY);
}

static inline gboolean
driver_open (SensorDriver       *driver,
	     GUdevDevice        *device,
	     ReadingsUpdateFunc  callback_func,
	     gpointer            user_data)
{
	g_return_val_if_fail (driver, FALSE);
	g_return_val_if_fail (driver->open, FALSE);
	g_return_val_if_fail (device, FALSE);
	g_return_val_if_fail (callback_func, FALSE);

	return driver->open (device, callback_func, user_data);
}

static inline void
driver_set_polling (SensorDriver *driver,
		    gboolean      state)
{
	g_return_if_fail (driver);

	if (!driver->set_polling)
		return;

	driver->set_polling (state);
}

static inline void
driver_close (SensorDriver *driver)
{
	g_return_if_fail (driver);
	g_return_if_fail (driver->close);

	driver->close ();
}

extern SensorDriver iio_buffer_accel;
extern SensorDriver iio_poll_accel;
extern SensorDriver input_accel;
extern SensorDriver fake_compass;
extern SensorDriver fake_light;
extern SensorDriver iio_poll_light;
extern SensorDriver hwmon_light;
extern SensorDriver iio_buffer_light;
extern SensorDriver iio_buffer_compass;
