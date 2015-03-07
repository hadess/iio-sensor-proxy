/*
 * Modified from industrialio buffer test code, and Lenovo Yoga (2 Pro) orientation helper
 * Copyright (c) 2008 Jonathan Cameron
 * Copyright (c) 2014 Peter F. Patel-Schneider
 * Copyright (c) 2011, 2014 Bastien Nocera <hadess@hadess.net>
 *
 * Every 700 msec, read data from an IIO accelerometer, and
 * from the accelerometer values, as well as the previous
 * orientation, calculate the device's new orientation.
 *
 * Possible values are:
 * * undefined
 * * normal
 * * bottom-up
 * * left-up
 * * right-up
 *
 * The property will be persistent across sessions, and the new
 * orientations can be deducted from the previous one (it allows
 * for a threshold for switching between opposite ends of the
 * orientation).
 *
 * orientation_calc() from the sensorfw package
 * Copyright (C) 2009-2010 Nokia Corporation
 * Authors:
 *   Üstün Ergenoglu <ext-ustun.ergenoglu@nokia.com>
 *   Timo Rongas <ext-timo.2.rongas@nokia.com>
 *   Lihan Guo <lihan.guo@digia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 2 as published by
 * the Free Software Foundation.
 *
 */

#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <glib.h>

#include "orientation.h"

static const char *orientations[] = {
        "undefined",
        "normal",
        "bottom-up",
        "left-up",
        "right-up",
        NULL
};

const char *
orientation_to_string (OrientationUp o)
{
        return orientations[o];
}

OrientationUp
string_to_orientation (const char *orientation)
{
        int i;

        if (orientation == NULL)
                return ORIENTATION_UNDEFINED;
        for (i = 0; orientations[i] != NULL; i++) {
                if (g_str_equal (orientation, orientations[i]))
                        return i;
        }
        return ORIENTATION_UNDEFINED;
}

#define RADIANS_TO_DEGREES 180.0/M_PI
#define SAME_AXIS_LIMIT 5

#define THRESHOLD_LANDSCAPE  35
#define THRESHOLD_PORTRAIT  35

OrientationUp
orientation_calc (OrientationUp prev,
                  int x, int y, int z)
{
        int rotation;
        OrientationUp ret = prev;

        /* Portrait check */
        rotation = round(atan((double) x / sqrt(y * y + z * z)) * RADIANS_TO_DEGREES);

        if (abs(rotation) > THRESHOLD_PORTRAIT) {
                ret = (rotation < 0) ? ORIENTATION_LEFT_UP : ORIENTATION_RIGHT_UP;

                /* Some threshold to switching between portrait modes */
                if (prev == ORIENTATION_LEFT_UP || prev == ORIENTATION_RIGHT_UP) {
                        if (abs(rotation) < SAME_AXIS_LIMIT) {
                                ret = prev;
                        }
                }

        } else {
                /* Landscape check */
                rotation = round(atan((double) y / sqrt(x * x + z * z)) * RADIANS_TO_DEGREES);

                if (abs(rotation) > THRESHOLD_LANDSCAPE) {
                        ret = (rotation < 0) ? ORIENTATION_BOTTOM_UP : ORIENTATION_NORMAL;

                        /* Some threshold to switching between landscape modes */
                        if (prev == ORIENTATION_BOTTOM_UP || prev == ORIENTATION_NORMAL) {
                                if (abs(rotation) < SAME_AXIS_LIMIT) {
                                        ret = prev;
                                }
                        }
                }
        }

        return ret;
}
