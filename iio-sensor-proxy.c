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

#define _GNU_SOURCE

#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <math.h>

#include <gudev/gudev.h>
#include "uinput.h"

typedef enum {
        ORIENTATION_UNDEFINED,
        ORIENTATION_NORMAL,
        ORIENTATION_BOTTOM_UP,
        ORIENTATION_LEFT_UP,
        ORIENTATION_RIGHT_UP
} OrientationUp;

static const char *orientations[] = {
        "undefined",
        "normal",
        "bottom-up",
        "left-up",
        "right-up",
        NULL
};

#define ORIENTATION_UP_UP ORIENTATION_NORMAL

#define RADIANS_TO_DEGREES 180.0/M_PI
#define SAME_AXIS_LIMIT 5

#define THRESHOLD_LANDSCAPE  25
#define THRESHOLD_PORTRAIT  20

static const char *
orientation_to_string (OrientationUp o)
{
        return orientations[o];
}

static OrientationUp
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

static OrientationUp
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

/**
 * iio_channel_info - information about a given channel
 * @name: channel name
 * @scale: scale factor to be applied for conversion to si units
 * @offset: offset to be applied for conversion to si units
 * @index: the channel index in the buffer output
 * @bytes: number of bytes occupied in buffer output
 * @mask: a bit mask for the raw output
 * @is_signed: is the raw value stored signed
 * @enabled: is this channel enabled
 **/
typedef struct {
	char *name;
	char *generic_name;
	float scale;
	float offset;
	unsigned index;
	unsigned bytes;
	unsigned bits_used;
	unsigned shift;
	uint64_t mask;
	unsigned be;
	unsigned is_signed;
	unsigned enabled;
	unsigned location;
} iio_channel_info;

typedef struct {
	GMainLoop *loop;
	char *dev_dir_name;
	char *trigger_name;
	int device_id;

	int channels_count;
	iio_channel_info **channels;

	int uinput;
	int accel_x, accel_y, accel_z;
	GUdevClient *client;
	GUdevDevice *uinput_dev;

	OrientationUp previous_orientation;
} OrientationData;

static char *
iioutils_break_up_name (const char *name)
{
	char **items, *ret;
	guint i;

	items = g_strsplit (name, "_", -1);
	for (i = 0; items[i] != NULL; i++) {
		if (items[i + 1] == NULL) {
			g_clear_pointer (&items[i], g_free);
			break;
		}
	}

	ret = g_strjoinv ("_", items);
	g_strfreev (items);

	return ret;
}

/**
 * iioutils_get_type() - find and process _type attribute data
 * @is_signed: output whether channel is signed
 * @bytes: output how many bytes the channel storage occupies
 * @mask: output a bit mask for the raw data
 * @be: big endian
 * @device_dir: the iio device directory
 * @name: the channel name
 **/
static gboolean
iioutils_get_type (unsigned   *is_signed,
		   unsigned   *bytes,
		   unsigned   *bits_used,
		   unsigned   *shift,
		   uint64_t   *mask,
		   unsigned   *be,
		   const char *device_dir,
		   const char *name,
		   const char *generic_name)
{
	int ret;
	char *builtname;
	char *filename;
	char signchar, endianchar;
	unsigned padint;
	FILE *sysfsfp;

	builtname = g_strdup_printf ("%s_type", name);
	filename = g_build_filename (device_dir, "scan_elements", builtname, NULL);
	g_free (builtname);

	sysfsfp = fopen (filename, "r");
	if (sysfsfp == NULL) {
		builtname = g_strdup_printf ("%s_type", generic_name);
		filename = g_build_filename (device_dir, "scan_elements", builtname, NULL);
		g_free (builtname);

		sysfsfp = fopen (filename, "r");
		if (sysfsfp == NULL) {
			g_free (filename);
			return FALSE;
		}
	}

	ret = fscanf (sysfsfp,
		      "%ce:%c%u/%u>>%u",
		      &endianchar,
		      &signchar,
		      bits_used,
		      &padint, shift);

	if (ret < 0) {
		g_warning ("Failed to pass scan type description for %s", filename);
		fclose (sysfsfp);
		g_free (filename);
		return FALSE;
	}
	fclose (sysfsfp);

	*be = (endianchar == 'b');
	*bytes = padint / 8;
	if (*bits_used == 64)
		*mask = ~0;
	else
		*mask = (1 << *bits_used) - 1;
	if (signchar == 's')
		*is_signed = 1;
	else
		*is_signed = 0;

	g_debug ("Got type for %s: is signed: %d, bytes: %d, bits_used: %d, shift: %d, mask: 0x%lX, be: %d",
		 name, *is_signed, *bytes, *bits_used, *shift, *mask, *be);

	g_free (filename);

	return TRUE;
}

static int
iioutils_get_param_float (float      *output,
			  const char *param_name,
			  const char *device_dir,
			  const char *name,
			  const char *generic_name)
{
	FILE *sysfsfp;
	char *builtname, *filename;
	int ret = 0;

	builtname = g_strdup_printf ("%s_%s", name, param_name);
	filename = g_build_filename (device_dir, builtname, NULL);
	g_free (builtname);

	sysfsfp = fopen (filename, "r");
	if (sysfsfp) {
		fscanf (sysfsfp, "%f", output);
		fclose (sysfsfp);
		g_free (filename);
		return 0;
	}

	g_free (filename);

	builtname = g_strdup_printf ("%s_%s", generic_name, param_name);
	filename = g_build_filename (device_dir, builtname, NULL);
	g_free (builtname);

	sysfsfp = fopen (filename, "r");
	if (sysfsfp) {
		fscanf (sysfsfp, "%f", output);
		fclose (sysfsfp);
	} else {
		ret = -errno;
		g_warning ("Failed to read float from %s", filename);
	}

	g_free (filename);

	return ret;
}

static void
channel_info_free (iio_channel_info *ci)
{
	g_free (ci->name);
	g_free (ci->generic_name);
	g_free (ci);
}

/* build_channel_array() - function to figure out what channels are present */
static iio_channel_info **
build_channel_array (const char        *device_dir,
		     int               *counter)
{
	GDir *dp;
	FILE *sysfsfp;
	int ret;
	const char *name;
	char *scan_el_dir;
	GPtrArray *array;
	iio_channel_info **ret_array;
	int i;

	*counter = 0;
	scan_el_dir = g_build_filename (device_dir, "scan_elements", NULL);

	dp = g_dir_open (scan_el_dir, 0, NULL);
	if (dp == NULL) {
		ret = -errno;
		g_free (scan_el_dir);
		return NULL;
	}

	array = g_ptr_array_new_full (0, (GDestroyNotify) channel_info_free);

	while ((name = g_dir_read_name (dp)) != NULL) {
		if (g_str_has_suffix (name, "_en")) {
			char *filename, *index_name;
			iio_channel_info *current;

			filename = g_build_filename (scan_el_dir, name, NULL);
			sysfsfp = fopen (filename, "r");
			if (sysfsfp == NULL) {
				g_free (filename);
				continue;
			}
			fscanf (sysfsfp, "%d", &ret);
			fclose (sysfsfp);
			if (!ret) {
				g_free (filename);
				continue;
			}
			g_free (filename);

			current = g_new0 (iio_channel_info, 1);

			current->scale = 1.0;
			current->offset = 0;
			current->name = g_strndup (name, strlen(name) - strlen("_en"));
			current->generic_name = iioutils_break_up_name (current->name);

			index_name = g_strdup_printf ("%s_index", current->name);
			filename = g_build_filename (scan_el_dir, index_name, NULL);
			g_free (index_name);

			sysfsfp = fopen (filename, "r");
			fscanf (sysfsfp, "%u", &current->index);
			fclose (sysfsfp);
			g_free (filename);

			/* Find the scale */
			ret = iioutils_get_param_float (&current->scale,
							"scale",
							device_dir,
							current->name,
							current->generic_name);
			if (ret < 0)
				goto error;

			ret = iioutils_get_param_float (&current->offset,
							"offset",
							device_dir,
							current->name,
							current->generic_name);
			if (ret < 0)
				goto error;

			ret = iioutils_get_type (&current->is_signed,
						 &current->bytes,
						 &current->bits_used,
						 &current->shift,
						 &current->mask,
						 &current->be,
						 device_dir,
						 current->name,
						 current->generic_name);

			if (!ret) {
				g_warning ("Could not parse name %s, generic name %s",
					   current->name, current->generic_name);
			} else {
				g_ptr_array_add (array, current);
			}
		}
	}
	g_dir_close (dp);
	g_free (scan_el_dir);

	*counter = array->len;
	ret_array = (iio_channel_info **) g_ptr_array_free (array, FALSE);

	for (i = 0; i < *counter; i++) {
		iio_channel_info *ci = ret_array[i];

		g_debug ("Built channel array for %s: is signed: %d, bytes: %d, bits_used: %d, shift: %d, mask: 0x%lX, be: %d",
			 ci->name, ci->is_signed, ci->bytes, ci->bits_used, ci->shift, ci->mask, ci->be);
	}

	return ret_array;

error:
	g_ptr_array_free (array, TRUE);
	g_dir_close (dp);
	g_free (scan_el_dir);
	return NULL;
}

static int
_write_sysfs_int (const char *filename,
		  const char *basedir,
		  int         val,
		  int         verify,
		  int         type,
		  int         val2)
{
	int ret = 0;
	FILE *sysfsfp;
	int test;
	char *temp;
	temp = g_build_filename (basedir, filename, NULL);
	sysfsfp = fopen(temp, "w");
	if (sysfsfp == NULL) {
		g_warning ("Could not open for write '%s'", temp);
		ret = -errno;
		goto error_free;
	}
	if (type)
		fprintf(sysfsfp, "%d %d", val, val2);
	else
		fprintf(sysfsfp, "%d", val);

	fclose(sysfsfp);
	if (verify) {
		sysfsfp = fopen(temp, "r");
		if (sysfsfp == NULL) {
			g_warning ("Could not open for read '%s'", temp);
			ret = -errno;
			goto error_free;
		}
		fscanf(sysfsfp, "%d", &test);
		if (test != val) {
			g_warning ("Possible failure in int write %d to %s",
				   val, temp);
			ret = -1;
		}
		fclose(sysfsfp);
	}
error_free:
	g_free (temp);
	return ret;
}

static int write_sysfs_int(const char *filename, const char *basedir, int val) {
	return _write_sysfs_int(filename, basedir, val, 0, 0, 0);
}

static int write_sysfs_int_and_verify(char *filename, char *basedir, int val) {
	return _write_sysfs_int(filename, basedir, val, 1, 0, 0);
}

static int
_write_sysfs_string (char *filename,
		     char *basedir,
		     char *val,
		     int   verify)
{
	int ret = 0;
	FILE *sysfsfp;
	char *temp;

	temp = g_build_filename (basedir, filename, NULL);
	sysfsfp = fopen (temp, "w");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}
	fprintf(sysfsfp, "%s", val);
	fclose(sysfsfp);

	/* Verify? */
	if (!verify)
		goto error_free;
	sysfsfp = fopen(temp, "r");
	if (sysfsfp == NULL) {
		ret = -errno;
		goto error_free;
	}
	fscanf(sysfsfp, "%s", temp);
	if (strcmp(temp, val) != 0) {
		g_warning ("Possible failure in string write of %s Should be %s written to %s\\%s\n",
			   temp, val, basedir, filename);
		ret = -1;
	}
	fclose(sysfsfp);

error_free:
	g_free(temp);

	return ret;
}

/**
 * write_sysfs_string_and_verify() - string write, readback and verify
 * @filename: name of file to write to
 * @basedir: the sysfs directory in which the file is to be found
 * @val: the string to write
 **/
static int write_sysfs_string_and_verify(char *filename, char *basedir, char *val) {
	return _write_sysfs_string(filename, basedir, val, 1);
}

static int write_sysfs_string(char *filename, char *basedir, char *val) {
	return _write_sysfs_string(filename, basedir, val, 0);
}

typedef struct SensorData_s {
	ssize_t read_size;
	int scan_size;
	char* data;
} SensorData;

/**
 * size_from_channelarray() - calculate the storage size of a scan
 * @channels:           the channel info array
 * @num_channels:       number of channels
 *
 * Has the side effect of filling the channels[i].location values used
 * in processing the buffer output.
 **/
static int
size_from_channelarray (iio_channel_info **channels,
			int                num_channels)
{
	int bytes = 0;
	int i = 0;
	while (i < num_channels) {
		if (bytes % channels[i]->bytes == 0)
			channels[i]->location = bytes;
		else
			channels[i]->location = bytes - bytes % channels[i]->bytes
				+ channels[i]->bytes;
		bytes = channels[i]->location + channels[i]->bytes;
		i++;
	}
	return bytes;
}

static int
prepare_output (OrientationData *or_data,
		char            *dev_dir_name,
		char            *trigger_name,
		int   (*callback)(SensorData, OrientationData *)) {
	char * buffer_access;
	int ret;
	SensorData data;

	int fp, buf_len = 127;

	/* Set the device trigger to be the data ready trigger */
	ret = write_sysfs_string_and_verify("trigger/current_trigger",
			dev_dir_name, trigger_name);
	if (ret < 0) {
		printf("Failed to write current_trigger file %s\n", strerror(-ret));
		goto error_ret;
	}

	/* Setup ring buffer parameters */
	ret = write_sysfs_int("buffer/length", dev_dir_name, 128);
	if (ret < 0) goto error_ret;
	/* Enable the buffer */
	ret = write_sysfs_int_and_verify("buffer/enable", dev_dir_name, 1);
	if (ret < 0) {
		printf("Unable to enable the buffer %d\n", ret);
		goto error_ret;
	}
	data.scan_size = size_from_channelarray (or_data->channels, or_data->channels_count);
	data.data = g_malloc(data.scan_size * buf_len);

	buffer_access = g_strdup_printf ("/dev/iio:device%d", or_data->device_id);
	/* Attempt to open non blocking to access dev */
	fp = open (buffer_access, O_RDONLY | O_NONBLOCK);
	if (fp == -1) { /* If it isn't there make the node */
		printf("Failed to open %s : %s\n", buffer_access, strerror(errno));
		ret = -errno;
		goto error_free_buffer_access;
	}

	/* Actually read the data */
	data.read_size = read (fp, data.data, buf_len * data.scan_size);
	if (data.read_size == -EAGAIN) {
		g_debug ("No new data available");
	} else {
		ret = callback(data, or_data);
	}

	/* Stop the buffer */
	int bret = write_sysfs_int ("buffer/enable", dev_dir_name, 0);
	if (bret < 0)
		goto error_close_buffer_access;

	/* Disconnect the trigger - just write a dummy name. */
	write_sysfs_string ("trigger/current_trigger", dev_dir_name, "NULL");

error_close_buffer_access:
	close(fp);
error_free_buffer_access:
	free(buffer_access);
	g_free(data.data);
error_ret:
	return ret;
}

/**
 * enable_sensors: enable all the sensors in a device
 * @device_dir: the IIO device directory in sysfs
 * @
 **/
static gboolean
enable_sensors (GUdevDevice *dev)
{
	GDir *dir;
	char *device_dir;
	const char *name;
	gboolean ret = TRUE;

	device_dir = g_build_filename (g_udev_device_get_sysfs_path (dev), "scan_elements", NULL);
	dir = g_dir_open (device_dir, 0, NULL);
	if (!dir) {
		g_free (device_dir);
		return FALSE;
	}

	while ((name = g_dir_read_name (dir))) {
		char *path;

		if (g_str_has_suffix (name, "_en") == FALSE)
			continue;

		/* Already enabled? */
		path = g_strdup_printf ("scan_elements/%s", name);
		if (g_udev_device_get_sysfs_attr_as_boolean (dev, path)) {
			g_free (path);
			continue;
		}
		g_free (path);

		/* Enable */
		if (write_sysfs_int (name, device_dir, 1) < 0) {
			g_warning ("Could not enable sensor %s/%s", device_dir, name);
			ret = FALSE;
			continue;
		}

		g_debug ("Enabled sensor %s/%s", device_dir, name);
	}
	g_dir_close (dir);
	g_free (device_dir);

	return ret;
}

/**
 * process_scan_1() - get an integer value for a particular channel
 * @data:               pointer to the start of the scan
 * @channels:           information about the channels. Note
 *  size_from_channelarray must have been called first to fill the
 *  location offsets.
 * @num_channels:       number of channels
 * ch_name:		name of channel to get
 * ch_val:		value for the channel
 * ch_present:		whether the channel is present
 **/
static void
process_scan_1 (char              *data,
		iio_channel_info **channels,
		int                num_channels,
		char              *ch_name,
		int               *ch_val,
		gboolean          *ch_present)
{
	int k;
	for (k = 0; k < num_channels; k++) {
		if (strcmp (channels[k]->name, ch_name) != 0)
			continue;

		switch (channels[k]->bytes) {
			/* only a few cases implemented so far */
		case 4:
			if (!channels[k]->is_signed) {
				uint32_t val = *(uint32_t *) (data + channels[k]->location);
				val = val >> channels[k]->shift;
				if (channels[k]->bits_used < 32) val &= ((uint32_t) 1 << channels[k]->bits_used) - 1;
				*ch_val = (int) val;
				*ch_present = TRUE;
			} else {
				int32_t val = *(int32_t *) (data + channels[k]->location);
				val = val >> channels[k]->shift;
				if (channels[k]->bits_used < 32) val &= ((uint32_t) 1 << channels[k]->bits_used) - 1;
				val = (int32_t) (val << (32 - channels[k]->bits_used)) >> (32 - channels[k]->bits_used);
				*ch_val = (int) val;
				*ch_present = TRUE;
			}
			break;
		case 2:
		case 8:
			g_error ("Process %d bytes channels not supported yet", channels[k]->bytes);
		default:
			g_assert_not_reached ();
			break;
		}
	}
}

static int
process_scan (SensorData data, OrientationData *or_data)
{
	OrientationUp orientation = or_data->previous_orientation;
	int i;
	int accel_x, accel_y, accel_z;
	gboolean present_x, present_y, present_z;

	/* Rather than read everything:
	 * for (i = 0; i < data.read_size / data.scan_size; i++)...
	 * Just read the last one */
	i = (data.read_size / data.scan_size) - 1;
	if (i < 0)
		return or_data->previous_orientation;

	process_scan_1(data.data + data.scan_size*i, or_data->channels, or_data->channels_count, "in_accel_x", &accel_x, &present_x);
	process_scan_1(data.data + data.scan_size*i, or_data->channels, or_data->channels_count, "in_accel_y", &accel_y, &present_y);
	process_scan_1(data.data + data.scan_size*i, or_data->channels, or_data->channels_count, "in_accel_z", &accel_z, &present_z);

	/* To match the Pegatron accelerometer code
	 * (see pega_accel_poll() in asus-laptop.c)
	 * we invert both x, and y values */
	accel_x = -accel_x;
	accel_y = -accel_y;

	g_debug ("Read from IIO: %d, %d, %d", accel_x, accel_y, accel_z);

	orientation = orientation_calc (or_data->previous_orientation, accel_x, accel_y, accel_z);

	or_data->accel_x = accel_x;
	or_data->accel_y = accel_y;
	or_data->accel_z = accel_z;

	return orientation;
}

static GUdevDevice *
find_accel (GUdevClient *client)
{
	GList *devices, *l;
	GUdevDevice *ret = NULL;
	gboolean has_trigger = FALSE;
	char *trigger_name;

	devices = g_udev_client_query_by_subsystem (client, "iio");

	/* Find the accelerometer */
	for (l = devices; l != NULL; l = l->next) {
		GUdevDevice *dev = l->data;

		if (g_strcmp0 ("accel_3d", g_udev_device_get_sysfs_attr (dev, "name")) == 0) {
			g_debug ("Found accel_3d at %s", g_udev_device_get_sysfs_path (dev));
			ret = g_object_ref (dev);
			break;
		}
	}
	if (ret == NULL)
		goto out;

	/* Find the associated trigger */
	trigger_name = g_strdup_printf ("accel_3d-dev%s", g_udev_device_get_number (ret));
	for (l = devices; l != NULL; l = l->next) {
		GUdevDevice *dev = l->data;

		if (g_strcmp0 (trigger_name, g_udev_device_get_sysfs_attr (dev, "name")) == 0) {
			g_debug ("Found associated trigger at %s", g_udev_device_get_sysfs_path (dev));
			has_trigger = TRUE;
			break;
		}
	}
	g_free (trigger_name);

	if (!has_trigger)
		g_clear_object (&ret);

out:
	g_list_free_full (devices, g_object_unref);
	return ret;
}

static GUdevDevice *
setup_uinput_udev (GUdevClient *client)
{
	GList *devices, *l;
	GUdevDevice *ret = NULL;

	devices = g_udev_client_query_by_subsystem (client, "input");
	for (l = devices; l != NULL; l = l->next) {
		GUdevDevice *dev = l->data;
//		guint i;
#if 0
		g_message ("path: %s", g_udev_device_get_sysfs_path (dev));
		const gchar * const * keys = g_udev_device_get_property_keys (dev);
		for (i = 0; keys[i]; i++)
			g_message ("key %s", keys[i]);
#endif
		if (g_udev_device_get_property_as_boolean (dev, "ID_INPUT_ACCELEROMETER")) {
			ret = g_object_ref (dev);
			break;
		}
	}
	g_list_free_full (devices, g_object_unref);

	return ret;
}

static gboolean
send_uinput_event (OrientationData *data)
{
	struct uinput_event ev;

	memset(&ev, 0, sizeof(ev));

	ev.type = EV_ABS;
	ev.code = ABS_X;
	ev.value = data->accel_x;
	write (data->uinput, &ev, sizeof(ev));

	ev.code = ABS_Y;
	ev.value = data->accel_y;
	write (data->uinput, &ev, sizeof(ev));

	ev.code = ABS_Z;
	ev.value = data->accel_z;
	write (data->uinput, &ev, sizeof(ev));

	memset(&ev, 0, sizeof(ev));
	gettimeofday(&ev.time, NULL);
	ev.type = EV_SYN;
	ev.code = SYN_REPORT;
	write (data->uinput, &ev, sizeof(ev));

	if (!data->uinput_dev)
		data->uinput_dev = setup_uinput_udev (data->client);
	if (!data->uinput_dev)
		return FALSE;

	if (write_sysfs_string ("uevent", (char *) g_udev_device_get_sysfs_path (data->uinput_dev), "change") < 0) {
		g_warning ("Failed to write uevent");
		return FALSE;
	}

	return TRUE;
}

static gboolean
read_orientation (gpointer user_data)
{
	OrientationData *data = user_data;
	OrientationUp orientation;

	if ((orientation = prepare_output (data, data->dev_dir_name, data->trigger_name, &process_scan)) < 0)
		return G_SOURCE_REMOVE;

	g_debug ("Found orientation: %s, prev:%s",
		 orientation_to_string (orientation),
		 orientation_to_string (data->previous_orientation));

	if (data->previous_orientation != orientation) {
		/* If we failed to send the uevent,
		 * we'll try again a bit later */
		if (send_uinput_event (data)) {
			g_debug ("Emitted orientation changed: from %s to %s",
				 orientation_to_string (data->previous_orientation),
				 orientation_to_string (orientation));
			data->previous_orientation = orientation;
		}
	}

	return G_SOURCE_CONTINUE;
}

static gboolean
setup_uinput (OrientationData *data)
{
	struct uinput_dev dev;
	int fd;

	fd = open("/dev/uinput", O_RDWR);
	if (fd < 0) {
		g_warning ("Could not open uinput");
		return FALSE;
	}

	memset (&dev, 0, sizeof(dev));
	snprintf (dev.name, sizeof (dev.name), "%s", "IIO Accelerometer Proxy");
	dev.id.bustype = BUS_VIRTUAL;
	dev.id.vendor = 0x01; //FIXME
	dev.id.product = 0x02;

	/* 1G accel is reported as ~256, so clamp to 2G */
	dev.absmin[ABS_X] = dev.absmin[ABS_Y] = dev.absmin[ABS_Z] = -512;
	dev.absmax[ABS_X] = dev.absmax[ABS_Y] = dev.absmax[ABS_Z] = 512;

	if (write (fd, &dev, sizeof(dev)) != sizeof(dev)) {
		g_warning ("Error creating uinput device");
		goto bail;
	}

	/* enabling key events */
	if (ioctl (fd, UI_SET_EVBIT, EV_ABS) < 0) {
		g_warning ("Error enabling uinput absolute events");
		goto bail;
	}

	/* enabling keys */
	if (ioctl (fd, UI_SET_ABSBIT, ABS_X) < 0 ||
	    ioctl (fd, UI_SET_ABSBIT, ABS_Y) < 0 ||
	    ioctl (fd, UI_SET_ABSBIT, ABS_Z) < 0) {
		g_warning ("Couldn't enable uinput axis");
		goto bail;
	}

	/* creating the device */
	if (ioctl (fd, UI_DEV_CREATE) < 0) {
		g_warning ("Error creating uinput device");
		goto bail;
	}

	data->uinput = fd;

	return TRUE;

bail:
	close (fd);
	return FALSE;
}

static void
free_orientation_data (OrientationData *data)
{
	int i;
	if (data == NULL)
		return;
	g_free (data->dev_dir_name);
	g_free (data->trigger_name);
	for (i = 0; i < data->channels_count; i++)
		channel_info_free (data->channels[i]);
	g_free (data->channels);
	if (data->uinput > 0)
		close (data->uinput);
	g_clear_object (&data->uinput_dev);
	g_clear_object (&data->client);
	g_main_loop_unref (data->loop);
	g_free (data);
}

int main (int argc, char **argv)
{
	OrientationData *data;
	GUdevClient *client;
	GUdevDevice *dev;
	const gchar * const subsystems[] = { "iio", NULL };
	guint id;

	/* g_setenv ("G_MESSAGES_DEBUG", "all", TRUE); */

	client = g_udev_client_new (subsystems);
	dev = find_accel (client);
	if (!dev) {
		g_debug ("Could not find IIO accelerometer");
		return 0;
	}

	data = g_new0 (OrientationData, 1);
	data->previous_orientation = ORIENTATION_UNDEFINED;
	data->dev_dir_name = g_strdup (g_udev_device_get_sysfs_path (dev));
	data->device_id = atoi (g_udev_device_get_number (dev));
	data->trigger_name = g_strdup_printf ("accel_3d-dev%d", data->device_id);
	data->client = client;

	if (!enable_sensors (dev)) {
		free_orientation_data (data);
		return 1;
	}
	g_object_unref (dev);

	/* Parse the files in scan_elements to identify what channels are present */
	data->channels = build_channel_array (data->dev_dir_name, &(data->channels_count));
	if (data->channels == NULL) {
		g_warning ("Problem reading scan element information: %s", data->dev_dir_name);
		return 1;
	}

	/* Set up uinput */
	if (!setup_uinput (data)) {
		free_orientation_data (data);
		return 1;
	}
	send_uinput_event (data);

	id = g_timeout_add (700, read_orientation, data);
	g_source_set_name_by_id (id, "read_orientation");

	data->loop = g_main_loop_new (NULL, TRUE);
	g_main_loop_run (data->loop);

	free_orientation_data (data);

	return 0;
}
