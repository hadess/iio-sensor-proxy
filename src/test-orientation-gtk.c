/*
 * Copyright (c) 2014-2016 Bastien Nocera <hadess@hadess.net>
 *
 * This program is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License version 3 as published by
 * the Free Software Foundation.
 *
 */

#include <gtk/gtk.h>
#include "orientation.h"

static GtkWidget *scale_x, *scale_y, *scale_z;
static GtkWidget *label;

#define ONEG 256

static void
value_changed (GtkSpinButton *spin_button,
	       gpointer       user_data)
{
	int x, y, z;
	OrientationUp o;

	x = gtk_spin_button_get_value (GTK_SPIN_BUTTON (scale_x));
	y = gtk_spin_button_get_value (GTK_SPIN_BUTTON (scale_y));
	z = gtk_spin_button_get_value (GTK_SPIN_BUTTON (scale_z));

	o = orientation_calc (ORIENTATION_UNDEFINED, x, y, z, 9.81 / ONEG);
	gtk_label_set_text (GTK_LABEL (label), orientation_to_string (o));
}

int main (int argc, char **argv)
{
	GtkWidget *window;
	GtkWidget *grid;

	gtk_init (&argc, &argv);

	window = gtk_window_new (GTK_WINDOW_TOPLEVEL);
	scale_x = gtk_spin_button_new_with_range (-ONEG, ONEG, 1);
	scale_y = gtk_spin_button_new_with_range (-ONEG, ONEG, 1);
	scale_z = gtk_spin_button_new_with_range (-ONEG, ONEG, 1);

	/* Set default values to "up" orientation */
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (scale_x), 0.0);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (scale_y), ONEG);
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (scale_z), 0.0);

	grid = gtk_grid_new ();
	g_object_set (G_OBJECT (grid),
		      "column-spacing", 12,
		      "row-spacing", 12,
		      NULL);
	gtk_container_add (GTK_CONTAINER (window), grid);

	gtk_grid_attach (GTK_GRID (grid), gtk_label_new ("X:"),
			 0, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), gtk_label_new ("Y:"),
			 0, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), gtk_label_new ("Z:"),
			 0, 2, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), scale_x,
			 1, 0, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), scale_y,
			 1, 1, 1, 1);
	gtk_grid_attach (GTK_GRID (grid), scale_z,
			 1, 2, 1, 1);

	g_signal_connect (G_OBJECT (scale_x), "value-changed",
			  G_CALLBACK (value_changed), NULL);
	g_signal_connect (G_OBJECT (scale_y), "value-changed",
			  G_CALLBACK (value_changed), NULL);
	g_signal_connect (G_OBJECT (scale_z), "value-changed",
			  G_CALLBACK (value_changed), NULL);

	label = gtk_label_new ("");
	gtk_grid_attach (GTK_GRID (grid), label, 0, 3, 2, 1);

	value_changed (NULL, NULL);

	gtk_widget_show_all (window);
	gtk_main ();

	return 0;
}
