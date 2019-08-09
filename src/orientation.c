/*
 * Copyright (c) 2011, 2014 Bastien Nocera <hadess@hadess.net>
 *
 * orientation_calc() from the sensorfw package
 * Copyright (C) 2009-2010 Nokia Corporation
 * Authors:
 *   Üstün Ergenoglu <ext-ustun.ergenoglu@nokia.com>
 *   Timo Rongas <ext-timo.2.rongas@nokia.com>
 *   Lihan Guo <lihan.guo@digia.com>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
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

/* First apply scale to get m/s², then
 * convert to 1G ~= 256 as the code expects */
#define SCALE(a) ((int) ((gdouble) a * scale * 256.0 / 9.81))

OrientationUp
orientation_calc (OrientationUp prev,
                  int in_x, int in_y, int in_z,
                  gdouble scale)
{
        OrientationUp ret = prev;
        int x, y, z;
        int portrait_rotation;
        int landscape_rotation;

        /* this code expects 1G ~= 256 */
        x = SCALE(in_x);
        y = SCALE(in_y);
        z = SCALE(in_z);

        portrait_rotation  = round(atan2(x, sqrt(y * y + z * z)) * RADIANS_TO_DEGREES);
        landscape_rotation = round(atan2(y, sqrt(x * x + z * z)) * RADIANS_TO_DEGREES);

        /* Don't change orientation if we are on the common border of two thresholds */
        if (abs(portrait_rotation) > THRESHOLD_PORTRAIT && abs(landscape_rotation) > THRESHOLD_LANDSCAPE)
                return prev;

        /* Portrait check */
        if (abs(portrait_rotation) > THRESHOLD_PORTRAIT) {
                ret = (portrait_rotation > 0) ? ORIENTATION_LEFT_UP : ORIENTATION_RIGHT_UP;

                /* Some threshold to switching between portrait modes */
                if (prev == ORIENTATION_LEFT_UP || prev == ORIENTATION_RIGHT_UP) {
                        if (abs(portrait_rotation) < SAME_AXIS_LIMIT) {
                                ret = prev;
                        }
                }

        } else {
                /* Landscape check */
                if (abs(landscape_rotation) > THRESHOLD_LANDSCAPE) {
                        ret = (landscape_rotation > 0) ? ORIENTATION_BOTTOM_UP : ORIENTATION_NORMAL;

                        /* Some threshold to switching between landscape modes */
                        if (prev == ORIENTATION_BOTTOM_UP || prev == ORIENTATION_NORMAL) {
                                if (abs(landscape_rotation) < SAME_AXIS_LIMIT) {
                                        ret = prev;
                                }
                        }
                }
        }

        return ret;
}
