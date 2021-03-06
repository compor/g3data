/*

g3data : A program for grabbing data from scanned graphs
Copyright (C) 2000 Jonas Frantz

    This file is part of g3data.

    g3data is free software; you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation; either version 2 of the License, or
    (at your option) any later version.

    g3data is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program; if not, write to the Free Software
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA


Authors email : jonas.frantz@welho.com

*/

#include <gtk/gtk.h>									/* Include gtk library */
#include <stdio.h>									/* Include stdio library */
#include <stdlib.h>									/* Include stdlib library */
#include "main.h"
#include "g3data-application.h"
#include "drawing.h"
#include "points.h"

gboolean use_error = FALSE;


/****************************************************************/
/* This is the main function, this function gets called when	*/
/* the program is executed. It allocates the necessary work-	*/
/* spaces and initialized the main window and its widgets.	*/
/****************************************************************/
int main (int argc, char **argv)
{
    const gchar **filenames = NULL;
    gboolean x_is_log = FALSE;
    gboolean y_is_log = FALSE;
    gint height = -1;
    gint width = -1;
    gdouble scale = G_MAXDOUBLE;
    gdouble coords[4] = {G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE, G_MAXDOUBLE};
    struct g3data_options *options;
    GError *error = NULL;
    GOptionContext *context;

    const GOptionEntry goption_options[] =
    {
        { "height", 'h', 0, G_OPTION_ARG_INT, &height, "The maximum height of image. Larger images will be scaled to this height.", "H"},
        { "width", 'w', 0, G_OPTION_ARG_INT, &width, "The maximum width of image. Larger images will be scaled to this width.", "W"},
        { "scale", 's', 0, G_OPTION_ARG_DOUBLE, &scale, "Scale image by scale factor.", "S"},
        { "error", 'e', 0, G_OPTION_ARG_NONE, &use_error, "Output estimates of error", NULL },
        { "lnx", 0, 0, G_OPTION_ARG_NONE, &x_is_log, "Use logarithmic scale for x coordinates", NULL },
        { "lny", 0, 0, G_OPTION_ARG_NONE, &y_is_log, "Use logarithmic scale for y coordinates", NULL},
        { "x0", 0, 0, G_OPTION_ARG_DOUBLE, &coords[0], "Preset the x-coordinate for the lower left corner", "x0" },
        { "x1", 0, 0, G_OPTION_ARG_DOUBLE, &coords[1], "Preset the x-coordinate for the upper right corner", "x1" },
        { "y0", 0, 0, G_OPTION_ARG_DOUBLE, &coords[2], "Preset the y-coordinate for the lower left corner", "y0" },
        { "y1", 0, 0, G_OPTION_ARG_DOUBLE, &coords[3], "Preset the y-coordinate for the upper right corner", "y1" },
        { G_OPTION_REMAINING, 0, 0, G_OPTION_ARG_FILENAME_ARRAY, &filenames, NULL, "[FILE...]" },
        { NULL }
    };

    gtk_init (&argc, &argv);

    context = g_option_context_new ("- grab graph data");
    g_option_context_add_main_entries (context, goption_options, NULL);
    g_option_context_add_group (context, gtk_get_option_group (TRUE));
    if (!g_option_context_parse (context, &argc, &argv, &error))
    {
        g_print ("option parsing failed: %s\n", error->message);
        g_error_free (error);
        g_option_context_free (context);
        exit (EXIT_FAILURE);
    }
    g_option_context_free (context);

    g_set_application_name ("Grab graph data");
    gtk_window_set_default_icon_name ("g3data-icon");

    options = (struct g3data_options *) g_malloc0 (sizeof (struct g3data_options));
    options->height = height;
    options->width = width;
    options->scale = scale;
    options->x_is_log = x_is_log;
    options->y_is_log = y_is_log;
    options->control_point_coords[0] = coords[0];
    options->control_point_coords[1] = coords[1];
    options->control_point_coords[2] = coords[2];
    options->control_point_coords[3] = coords[3];

    load_files (filenames, options);

    return (EXIT_SUCCESS);
}
