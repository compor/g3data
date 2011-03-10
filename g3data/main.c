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
#include <gdk-pixbuf/gdk-pixbuf.h>
#include <gdk/gdkkeysyms.h>
#include <stdlib.h>									/* Include stdlib library */
#include <string.h>									/* Include string library */
#include <math.h>									/* Include math library */
#include <libgen.h>
#include "main.h"
#include "drawing.h"
#include "strings.h"
#include "points.h"

#ifdef NOSPACING
#define SECT_SEP 0
#define GROUP_SEP 0
#define ELEM_SEP 0
#define FRAME_INDENT 0
#define WINDOW_BORDER 0
#else
#define SECT_SEP 12
#define GROUP_SEP 12
#define ELEM_SEP 6
#define FRAME_INDENT 18
#define WINDOW_BORDER 12
#endif

/* Declaration of gtk variables */
GtkWidget	*window;								/* Window */
GtkWidget	*drawing_area, *zoom_area;			/* Drawing areas */
GtkWidget	*xyentry[4];
GtkWidget	*remlastbutton;
GtkWidget	*setxybutton[4];
GtkWidget	*remallbutton;						/* Even more various buttons */
GtkWidget	*xc_entry,*yc_entry;
GtkWidget	*nump_entry;
GtkWidget	*xerr_entry,*yerr_entry;			/* Coordinate and filename entries */
GtkWidget       *logbox = {NULL}, *zoomareabox = {NULL}, *oppropbox = {NULL};
GtkWidget	*pm_label, *pm_label2, *file_label;
GtkWidget	*ViewPort = NULL;
GdkColor        *colors;								/* Pointer to colors */
GdkPixbuf       *gpbimage;
GtkWidget *mainvbox;
GtkActionGroup	*tab_action_group;

/* Declaration of global variables */
/* axiscoords[][][0] will be set to -1 when not used */
gint		axiscoords[4][2];						/* X,Y coordinates of axispoints */
gint		**points;							/* Indexes of graphpoints and their coordinates */
gint		numpoints;
gint		ordering;
gint		XSize, YSize;
gint		file_name_length;
gint 		MaxPoints = {MAXPOINTS};
gint		NoteBookNumPages = 0;
gint xpointer = -1;
gint ypointer = -1;
gdouble		realcoords[4];						/* X,Y coords on graph */
gboolean	UseErrors;
gboolean	setxypressed[4];
gboolean	bpressed[4];						/* What axispoints have been set out ? */
gboolean	valueset[4];
gboolean	logxy[2] = {FALSE,FALSE};
gboolean        ShowLog = FALSE, ShowZoomArea = FALSE, ShowOpProp = FALSE;
const gchar *file_name;
gchar		*FileNames;
FILE		*FP;									/* File pointer */

GtkWidget 	*drawing_area_alignment;

static gint close_application(GtkWidget *widget, GdkEvent *event, gpointer data);
static void SetButtonSensitivity(void);
static gint button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data);
static gint motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data);
static gboolean expose_event_callback(GtkWidget *widget, GdkEventExpose *event, gpointer data);
static gint expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data);
static gint configure_event(GtkWidget *widget, GdkEventConfigure *event,gpointer data);
static void toggle_xy(GtkWidget *widget, gpointer func_data);
static void SetOrdering(GtkWidget *widget, gpointer func_data);
static void UseErrCB(GtkWidget *widget, gpointer func_data);
static void read_xy_entry(GtkWidget *entry, gpointer func_data);
static void islogxy(GtkWidget *widget, gpointer func_data);
static void remove_last(GtkWidget *widget, gpointer data);
static void remove_all(GtkWidget *widget, gpointer data) ;
static gint key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer pointer);
static gint InsertImage(char *filename, gdouble Scale, gdouble maxX, gdouble maxY);
static void update_preview_cb (GtkFileChooser *file_chooser, gpointer data);
static gint SetupNewTab(char *filename, gdouble Scale, gdouble maxX, gdouble maxY, gboolean UsePreSetCoords);
static void drag_data_received(GtkWidget *widget,
                              GdkDragContext *drag_context,
                              gint x, gint y,
                              GtkSelectionData *data,
                              guint info,
                              guint event_time,
                              gpointer user_data);
static GCallback menu_file_open(void);
static GCallback menu_help_about(void);
static GCallback full_screen_action_callback(GtkWidget *widget, gpointer func_data);
static GCallback hide_zoom_area_callback(GtkWidget *widget, gpointer func_data);
static GCallback hide_axis_settings_callback(GtkWidget *widget, gpointer func_data);
static GCallback hide_output_prop_callback(GtkWidget *widget, gpointer func_data);

/****************************************************************/
/* This function closes the window when the application is 	*/
/* killed.							*/
/****************************************************************/
static gint close_application(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    gtk_main_quit();									/* Quit gtk */
    return FALSE;
}


/****************************************************************/
/* This function sets the sensitivity of the buttons depending	*/
/* the control variables.					*/
/****************************************************************/
static void SetButtonSensitivity(void)
{
    if (numpoints==0 &&
        axiscoords[0][0] == -1 &&
        axiscoords[1][0] == -1 &&
        axiscoords[2][0] == -1 &&
        axiscoords[3][0] == -1) {
        gtk_widget_set_sensitive(remlastbutton,FALSE);
        gtk_widget_set_sensitive(remallbutton,FALSE);
    } else if (numpoints==0 &&
              (axiscoords[0][0] != -1 ||
               axiscoords[1][0] != -1 ||
               axiscoords[2][0] != -1 ||
               axiscoords[3][0] != -1)) {
        gtk_widget_set_sensitive(remlastbutton,FALSE);
        gtk_widget_set_sensitive(remallbutton,TRUE);
    } else {
        gtk_widget_set_sensitive(remlastbutton,TRUE);
        gtk_widget_set_sensitive(remallbutton,TRUE);
    }
}


/****************************************************************/
/* When a button is pressed inside the drawing area this 	*/
/* function is called, it handles axispoints and graphpoints	*/
/* and paints a square in that position.			*/
/****************************************************************/
static gint button_press_event(GtkWidget *widget, GdkEventButton *event, gpointer data)
{
    gint x, y, i, j;

    gdk_window_get_pointer (event->window, &x, &y, NULL);

    if (event->button == 1) {
        /* If none of the set axispoint buttons been pressed */
        if (!setxypressed[0] && !setxypressed[1] && !setxypressed[2] && !setxypressed[3]) {
            if (numpoints > MaxPoints-1) {
                i = MaxPoints;
                MaxPoints += MAXPOINTS;
                points = realloc(points,sizeof(gint *) * MaxPoints);
                if (points==NULL) {
                    printf("Error reallocating memory for points. Exiting.\n");
                    exit(-1);
                }
                for (;i<MaxPoints;i++) {
                    points[i] = malloc(sizeof(gint) * 2);
                    if (points[i]==NULL) {
                        printf("Error allocating memory for points[%d]. Exiting.\n",i);
                        exit(-1);
                    }
                }
            }
            points[numpoints][0]=x;
            points[numpoints][1]=y;
            numpoints++;
            SetNumPointsEntry(nump_entry, numpoints);
        } else {
            for (i=0;i<4;i++) {
                /* If any of the set axispoint buttons are pressed */
                if (setxypressed[i]) {
                    axiscoords[i][0]=x;
                    axiscoords[i][1]=y;
                    for (j=0;j<4;j++) {
                        if (i!=j) {
                            gtk_widget_set_sensitive(setxybutton[j],TRUE);
                        }
                    }
                    gtk_widget_set_sensitive(xyentry[i],TRUE);
                    gtk_editable_set_editable(GTK_EDITABLE(xyentry[i]),TRUE);
                    gtk_widget_grab_focus(xyentry[i]);
                    setxypressed[i]=FALSE;
                    bpressed[i]=TRUE;
                    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(setxybutton[i]),FALSE);
                }
            }
        }
    } else if (event->button == 2) {
        for (i=0;i<2;i++) {
            if (!bpressed[i]) {
                axiscoords[i][0]=x;
                axiscoords[i][1]=y;
                for (j=0;j<4;j++) {
                    if (i!=j) {
                        gtk_widget_set_sensitive(setxybutton[j],TRUE);
                    }
                }
                gtk_widget_set_sensitive(xyentry[i],TRUE);
                gtk_editable_set_editable(GTK_EDITABLE(xyentry[i]),TRUE);
                gtk_widget_grab_focus(xyentry[i]);
                setxypressed[i]=FALSE;
                bpressed[i]=TRUE;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(setxybutton[i]),FALSE);

                break;
            }
        }
    } else if (event->button == 3) {
        for (i=2;i<4;i++) {
            if (!bpressed[i]) {
                axiscoords[i][0]=x;
                axiscoords[i][1]=y;
                for (j=0;j<4;j++) {
                    if (i!=j) {
                        gtk_widget_set_sensitive(setxybutton[j],TRUE);
                    }
                }
                gtk_widget_set_sensitive(xyentry[i],TRUE);
                gtk_editable_set_editable(GTK_EDITABLE(xyentry[i]),TRUE);
                gtk_widget_grab_focus(xyentry[i]);
                setxypressed[i]=FALSE;
                bpressed[i]=TRUE;
                gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(setxybutton[i]),FALSE);

                break;
            }
        }
    }
    SetButtonSensitivity();
    gtk_widget_queue_draw(drawing_area);

    return TRUE;
}


static gint motion_notify_event(GtkWidget *widget, GdkEventMotion *event, gpointer data)
{
    gint x, y;
    gchar buf[32];
    struct PointValue CalcVal;

    gdk_window_get_pointer (event->window, &x, &y, NULL);
    xpointer = x;
    ypointer = y;

    /* If pointer over image, and axis points have been set,
       then print the coordinates. */
    if (x >= 0 && y >= 0 && x < XSize && y < YSize &&
       (valueset[0] && valueset[1] && valueset[2] && valueset[3])) {

        CalcVal = CalcPointValue(x,y);
        g_ascii_formatd(buf, 32, "%.5f", CalcVal.Xv);
        gtk_entry_set_text(GTK_ENTRY(xc_entry),buf);
        g_ascii_formatd(buf, 32, "%.5f", CalcVal.Yv);
        gtk_entry_set_text(GTK_ENTRY(yc_entry),buf);
        g_ascii_formatd(buf, 32, "%.5f", CalcVal.Xerr);
        gtk_entry_set_text(GTK_ENTRY(xerr_entry),buf);
        g_ascii_formatd(buf, 32, "%.5f", CalcVal.Yerr);
        gtk_entry_set_text(GTK_ENTRY(yerr_entry),buf);
    } else {
        gtk_entry_set_text(GTK_ENTRY(xc_entry),"");
        gtk_entry_set_text(GTK_ENTRY(yc_entry),"");
        gtk_entry_set_text(GTK_ENTRY(xerr_entry),"");
        gtk_entry_set_text(GTK_ENTRY(yerr_entry),"");
    }
    gtk_widget_queue_draw(zoom_area);

    return TRUE;
}


/* expose_event_callback for the zoom area. */
static gboolean expose_event_callback(GtkWidget *widget, GdkEventExpose *event, gpointer data) {
    cairo_t *cr;

    cr = gdk_cairo_create (gtk_widget_get_window(widget));

    if (xpointer >= 0 && ypointer >= 0 && xpointer < XSize && ypointer < YSize) {
        cairo_save(cr);
        cairo_translate(cr, -xpointer*ZOOMFACTOR + ZOOMPIXSIZE/2, -ypointer*ZOOMFACTOR + ZOOMPIXSIZE/2);
        cairo_scale(cr, 1.0*ZOOMFACTOR, 1.0*ZOOMFACTOR);
        gdk_cairo_set_source_pixbuf (cr, gpbimage, 0, 0);
        cairo_paint(cr);
        cairo_restore(cr);
    }

    /* Then draw the square in the middle of the zoom area */
    DrawMarker(cr, ZOOMPIXSIZE/2, ZOOMPIXSIZE/2, 2, colors);
    cairo_destroy (cr);
    return TRUE;
}


/****************************************************************/
/* This function is called when the drawing area is exposed, it	*/
/* simply redraws the pixmap on it.				*/
/****************************************************************/
static gint expose_event(GtkWidget *widget, GdkEventExpose *event, gpointer data)
{
    gint i;
    cairo_t *cr = gdk_cairo_create (gtk_widget_get_window(widget));

    gdk_cairo_set_source_pixbuf (cr, gpbimage, 0, 0);
    cairo_paint (cr);

    for (i=0;i<4;i++) if (bpressed[i]) DrawMarker(cr, axiscoords[i][0], axiscoords[i][1], i/2, colors);
    for (i=0;i<numpoints;i++) DrawMarker(cr, points[i][0], points[i][1], 2, colors);

    cairo_destroy (cr);
    return FALSE;
}   


/****************************************************************/
/* This function is called when the drawing area is configured	*/
/* for the first time, currently this function does not perform	*/
/* any task.							*/
/****************************************************************/
static gint configure_event(GtkWidget *widget, GdkEventConfigure *event,gpointer data)
{
    return TRUE;
}


/****************************************************************/
/* This function is called when the "Set point 1/2 on x/y axis"	*/
/* button is pressed. It inactivates the other "Set" buttons	*/
/* and makes sure the button stays down even when pressed on.	*/
/****************************************************************/
static void toggle_xy(GtkWidget *widget, gpointer func_data)
{
    gint i, j;

    i = GPOINTER_TO_INT (func_data);

    if (gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON (widget))) {
	setxypressed[i] = TRUE;						/* The button is pressed down */
	for (j = 0; j < 4; j++) {
	    if (i != j) gtk_widget_set_sensitive(setxybutton[j],FALSE);
	}
	if (bpressed[i]) {								/* If the x axis point is already set */
        axiscoords[i][0] = -1;
        axiscoords[i][1] = -1;
	}
	bpressed[i]=FALSE;						/* Set x axis point 1 to unset */
    } else {										/* If button is trying to get unpressed */
	if (setxypressed[i]) 
	    gtk_toggle_button_set_active(GTK_TOGGLE_BUTTON(widget),TRUE); 		/* Set button down */
    }
    gtk_widget_queue_draw(drawing_area);
}


/****************************************************************/
/* Set type of ordering at output of data.			*/
/****************************************************************/
static void SetOrdering(GtkWidget *widget, gpointer func_data)
{
    ordering = GPOINTER_TO_INT (func_data);				/* Set ordering control variable */
}


/****************************************************************/
/* Set whether to use error evaluation and printing or not.	*/
/****************************************************************/
static void UseErrCB(GtkWidget *widget, gpointer func_data)
{
    UseErrors = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
}


/****************************************************************/
/* When the value of the entry of any axis point is changed, 	*/
/* this function gets called.					*/
/****************************************************************/
static void read_xy_entry(GtkWidget *entry, gpointer func_data)
{
    const gchar *xy_text;
    gint i;
    
    i = GPOINTER_TO_INT (func_data);

    xy_text = gtk_entry_get_text(GTK_ENTRY (entry));
    sscanf(xy_text,"%lf",&realcoords[i]);				/* Convert string to double value and */
											/* store in realcoords[0]. */
    if (logxy[i/2] && realcoords[i] > 0) valueset[i]=TRUE;
    else if (logxy[i/2]) valueset[i]=FALSE;
    else valueset[i] = TRUE;

    SetButtonSensitivity();
}


/****************************************************************/
/* If the "X/Y axis is logarithmic" check button is toggled	*/
/* this function gets called. It sets the logx variable to its	*/
/* correct value corresponding to the buttons state.		*/
/****************************************************************/
static void islogxy(GtkWidget *widget, gpointer func_data)
{
  gint i;

    i = GPOINTER_TO_INT (func_data);

    logxy[i] = gtk_toggle_button_get_active(GTK_TOGGLE_BUTTON(widget));
											/* logxy = TRUE else FALSE. */
    if (logxy[i]) {
	if (realcoords[i*2] <= 0) {					/* If a negative value has been insert */
	    valueset[i*2]=FALSE;
	    gtk_entry_set_text(GTK_ENTRY(xyentry[i*2]),"");		/* Zero it */
	}
	if (realcoords[i*2+1] <= 0) {					/* If a negative value has been insert */
	    valueset[i*2+1]=FALSE;
	    gtk_entry_set_text(GTK_ENTRY(xyentry[i*2+1]),"");		/* Zero it */
        }
    }
}


/* Removes the last data point inserted */
static void remove_last(GtkWidget *widget, gpointer data)
{
    /* If there are any points, remove one. */
    if (numpoints > 0) {
        points[numpoints][0] = -1;
        points[numpoints][1] = -1;
        numpoints--;
        SetNumPointsEntry(nump_entry, numpoints);
    }

    SetButtonSensitivity();
    gtk_widget_queue_draw(drawing_area);
}


/****************************************************************/
/* This function sets the proper variables and then calls 	*/
/* remove_last, to remove all points except the axis points.	*/
/****************************************************************/
static void remove_all(GtkWidget *widget, gpointer data) 
{
    gint i;

    /* set axiscoords to -1, so the axis points do not get drawn*/
    for (i = 0; i < 4; i++) {
        axiscoords[i][0] = -1;
        axiscoords[i][1] = -1;
        /* Clear axis points text entries, make buttons insensitive */
	    valueset[i] = FALSE;
	    bpressed[i] = FALSE;
	    gtk_entry_set_text(GTK_ENTRY(xyentry[i]), "");
        gtk_widget_set_sensitive(xyentry[i], FALSE);
    }

    numpoints = 0;
    SetNumPointsEntry(nump_entry, numpoints);

    remove_last(widget, data);
}


/****************************************************************/
/* This function handles all of the keypresses done within the	*/
/* main window and handles the  appropriate measures.		*/
/****************************************************************/
static gint key_press_event(GtkWidget *widget, GdkEventKey *event, gpointer pointer)
{
  GtkAdjustment *adjustment;
  gdouble adj_val;
  GdkCursor	*cursor;

    if (ViewPort != NULL) {

    if (event->keyval==GDK_Left) {
	adjustment = gtk_viewport_get_hadjustment(GTK_VIEWPORT(ViewPort));
	adj_val = gtk_adjustment_get_value(adjustment);
	adj_val -= gtk_adjustment_get_page_size(adjustment)/10.0;
	if (adj_val < gtk_adjustment_get_lower(adjustment)) adj_val = gtk_adjustment_get_lower(adjustment);
	gtk_adjustment_set_value(adjustment, adj_val);
	gtk_viewport_set_hadjustment(GTK_VIEWPORT(ViewPort), adjustment);
    } else if (event->keyval==GDK_Right) {
	adjustment = gtk_viewport_get_hadjustment(GTK_VIEWPORT(ViewPort));
	adj_val = gtk_adjustment_get_value(adjustment);
	adj_val += gtk_adjustment_get_page_size(adjustment)/10.0;
	if (adj_val > (gtk_adjustment_get_upper(adjustment)-gtk_adjustment_get_page_size(adjustment))) adj_val = (gtk_adjustment_get_upper(adjustment)-gtk_adjustment_get_page_size(adjustment));
	gtk_adjustment_set_value(adjustment, adj_val);
	gtk_viewport_set_hadjustment(GTK_VIEWPORT(ViewPort), adjustment);
    } else if (event->keyval==GDK_Up) {
	adjustment = gtk_viewport_get_vadjustment(GTK_VIEWPORT(ViewPort));
	adj_val = gtk_adjustment_get_value(adjustment);
	adj_val -= gtk_adjustment_get_page_size(adjustment)/10.0;
	if (adj_val < gtk_adjustment_get_lower(adjustment)) adj_val = gtk_adjustment_get_lower(adjustment);
	gtk_adjustment_set_value(adjustment, adj_val);
	gtk_viewport_set_vadjustment(GTK_VIEWPORT(ViewPort), adjustment);
    } else if (event->keyval==GDK_Down) {
	adjustment = gtk_viewport_get_vadjustment(GTK_VIEWPORT(ViewPort));
	adj_val = gtk_adjustment_get_value(adjustment);
	adj_val += gtk_adjustment_get_page_size(adjustment)/10.0;
	if (adj_val > (gtk_adjustment_get_upper(adjustment)-gtk_adjustment_get_page_size(adjustment))) adj_val = (gtk_adjustment_get_upper(adjustment)-gtk_adjustment_get_page_size(adjustment));
	gtk_adjustment_set_value(adjustment, adj_val);
	gtk_viewport_set_vadjustment(GTK_VIEWPORT(ViewPort), adjustment);
    }
    }

  return 0;
}


/****************************************************************/
/* This function loads the image, and inserts it into the tab	*/
/* and sets up all of the different signals associated with it.	*/
/****************************************************************/
static gint InsertImage(char *filename, gdouble Scale, gdouble maxX, gdouble maxY) {
    gboolean has_alpha;
    gint width, height;
    GdkPixbuf *loadgpbimage;
    GdkCursor *cursor;
    GtkWidget *dialog;

    loadgpbimage = gdk_pixbuf_new_from_file(filename,NULL);				/* Load image */
    if (loadgpbimage==NULL) {								/* If unable to load image */
	dialog = gtk_message_dialog_new (GTK_WINDOW(window),				/* Notify user of the error */
                                  GTK_DIALOG_DESTROY_WITH_PARENT,			/* with a dialog */
                                  GTK_MESSAGE_ERROR,
                                  GTK_BUTTONS_CLOSE,
                                  "Error loading file '%s'",
                                  filename);
 	gtk_dialog_run (GTK_DIALOG (dialog));
 	gtk_widget_destroy (dialog);

	return -1;									/* exit */
    }

    width = gdk_pixbuf_get_width(loadgpbimage);
    height = gdk_pixbuf_get_height(loadgpbimage);
    has_alpha = gdk_pixbuf_get_has_alpha(loadgpbimage);

    if (maxX != -1 && maxY != -1 && Scale == -1) {
        if (width > maxX || height > maxY) {
            Scale = fmin((double) (maxX/width), (double) (maxY/height));
        }
    }

    if (Scale != -1) {
        width = width * Scale;
        height = height * Scale;
        gpbimage = gdk_pixbuf_new (GDK_COLORSPACE_RGB, has_alpha, 8, width, height);
        gdk_pixbuf_composite(loadgpbimage, gpbimage, 0, 0, width, height,
    	         0, 0, Scale, Scale, GDK_INTERP_BILINEAR, 255);
        g_object_unref(loadgpbimage);
    } else {
        gpbimage = loadgpbimage;
    }

    XSize = width;
    YSize = height;

    drawing_area = gtk_drawing_area_new ();					/* Create new drawing area */
    gtk_widget_set_size_request (drawing_area, XSize, YSize);

    g_signal_connect (G_OBJECT (drawing_area), "expose_event",			/* Connect drawing area to */
              G_CALLBACK (expose_event), NULL);			/* expose_event. */

    g_signal_connect (G_OBJECT (drawing_area), "configure_event",		/* Connect drawing area to */
              G_CALLBACK (configure_event), NULL);			/* configure_event. */

    g_signal_connect (G_OBJECT (drawing_area), "button_press_event",		/* Connect drawing area to */
              G_CALLBACK (button_press_event), NULL);		/* button_press_event. */

    g_signal_connect (G_OBJECT (drawing_area), "motion_notify_event",		/* Connect drawing area to */
              G_CALLBACK (motion_notify_event), NULL);		/* motion_notify_event. */

    gtk_widget_set_events (drawing_area, GDK_EXPOSURE_MASK |			/* Set the events active */
			   GDK_BUTTON_PRESS_MASK | 
			   GDK_BUTTON_RELEASE_MASK |
			   GDK_POINTER_MOTION_MASK | 
			   GDK_POINTER_MOTION_HINT_MASK);

    gtk_container_add(GTK_CONTAINER(drawing_area_alignment), drawing_area);

    gtk_widget_show(drawing_area);

    cursor = gdk_cursor_new (GDK_CROSSHAIR);
    gdk_window_set_cursor (gtk_widget_get_window(drawing_area), cursor);
 
    return 0;
}


/****************************************************************/
/* This callback sets up the thumbnail in the Fileopen dialog.	*/
/****************************************************************/
static void update_preview_cb (GtkFileChooser *file_chooser, gpointer data) {
  GtkWidget *preview;
  gchar *filename;
  GdkPixbuf *pixbuf;
  gboolean have_preview;

    preview = GTK_WIDGET (data);
    filename = gtk_file_chooser_get_preview_filename (file_chooser);
    if (filename != NULL) {
        pixbuf = gdk_pixbuf_new_from_file_at_size (filename, 128, 128, NULL);
        have_preview = (pixbuf != NULL);
        g_free (filename);

        gtk_image_set_from_pixbuf (GTK_IMAGE (preview), pixbuf);
        if (pixbuf)
            g_object_unref (pixbuf);

        gtk_file_chooser_set_preview_widget_active (file_chooser, have_preview);
    }
}


/****************************************************************/
/* This function sets up a new tab, sets up all of the widgets 	*/
/* needed.							*/
/****************************************************************/
static gint SetupNewTab(char *filename, gdouble Scale, gdouble maxX, gdouble maxY, gboolean UsePreSetCoords)
{
  GtkWidget 	*table;									/* GTK table/box variables for packing */
  GtkWidget	*tophbox, *bottomhbox;
  GtkWidget	*trvbox, *tlvbox, *brvbox, *blvbox, *subvbox;
  GtkWidget 	*xy_label[4];								/* Labels for texts in window */
  GtkWidget 	*logcheckb[2];								/* Logarithmic checkbuttons */
  GtkWidget 	*nump_label, *ScrollWindow;						/* Various widgets */
  GtkWidget	*APlabel, *PIlabel, *ZAlabel, *Llabel, *tab_label;
  GtkWidget 	*alignment;
  GtkWidget 	*x_label, *y_label, *tmplabel;
  GtkWidget	*ordercheckb[3], *UseErrCheckB;
  GtkWidget	*Olabel, *Elabel;
  GSList 	*group;
  GtkWidget	*dialog;

    gchar buf[20];
    gchar *buffer;
  gint 		i;

    table = gtk_table_new(1, 2 ,FALSE);							/* Create table */
    gtk_container_set_border_width (GTK_CONTAINER (table), WINDOW_BORDER);
    gtk_table_set_row_spacings(GTK_TABLE(table), SECT_SEP);				/* Set spacings */
    gtk_table_set_col_spacings(GTK_TABLE(table), 0);
    gtk_box_pack_start (GTK_BOX (mainvbox), table, FALSE, FALSE, 0);

/* Init datastructures */
    FileNames = g_strdup_printf("%s", basename(filename));

    for (i = 0; i < 4; i++) {
        axiscoords[i][0] = -1;
        axiscoords[i][1] = -1;
        bpressed[i] = FALSE;
        valueset[i] = FALSE;
    }

    numpoints = 0;
    ordering = 0;

    points = (void *) malloc(sizeof(gint *) * MaxPoints);
    if (points==NULL) {
	printf("Error allocating memory for points. Exiting.\n");
	return -1;
    }
    for (i=0;i<MaxPoints;i++) {
	points[i] = (gint *) malloc(sizeof(gint) * 2);
	if (points[i]==NULL) {
	    printf("Error allocating memory for points[%d]. Exiting.\n",i);
	    return -1;
	}
    }

    for (i=0;i<4;i++) {
    /* buttons for setting axis points x_1, x_2, etc. */
	tmplabel = gtk_label_new(NULL);
	gtk_label_set_markup_with_mnemonic(GTK_LABEL(tmplabel), setxylabel[i]);
	setxybutton[i] = gtk_toggle_button_new();				/* Create button */
	gtk_container_add(GTK_CONTAINER(setxybutton[i]), tmplabel);
	g_signal_connect (G_OBJECT (setxybutton[i]), "toggled",			/* Connect button */
			  G_CALLBACK (toggle_xy), GINT_TO_POINTER (i));
        gtk_widget_set_tooltip_text(setxybutton[i],setxytts[i]);

    /* labels for axis points x_1, x_2, etc. */
	xy_label[i] = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(xy_label[i]), xy_label_text[i]);

    /* text entries to enter axis points x_1, x_2, etc. */
	xyentry[i] = gtk_entry_new();  						/* Create text entry */
	gtk_entry_set_max_length (GTK_ENTRY (xyentry[i]), 20);
	gtk_widget_set_sensitive(xyentry[i],FALSE);				/* Inactivate it */
	g_signal_connect (G_OBJECT (xyentry[i]), "changed",			/* Init the entry to call */
			  G_CALLBACK (read_xy_entry), GINT_TO_POINTER (i));		/* read_x1_entry whenever */
        gtk_widget_set_tooltip_text (xyentry[i],entryxytt[i]);
    }

    /* Processing information labels and text entries */
    x_label = gtk_label_new(x_string);
    y_label = gtk_label_new(y_string);
    xc_entry = gtk_entry_new();							/* Create text entry */
    gtk_entry_set_max_length (GTK_ENTRY (xc_entry), 16);
    gtk_editable_set_editable(GTK_EDITABLE(xc_entry),FALSE);
    yc_entry = gtk_entry_new();							/* Create text entry */
    gtk_entry_set_max_length (GTK_ENTRY (yc_entry), 16);
    gtk_editable_set_editable(GTK_EDITABLE(yc_entry),FALSE);

    /* plus/minus (+/-) symbol labels */
    pm_label = gtk_label_new(pm_string);
    pm_label2 = gtk_label_new(pm_string);
    /* labels and error text entries */
    xerr_entry = gtk_entry_new();						/* Create text entry */
    gtk_entry_set_max_length (GTK_ENTRY (xerr_entry), 16);
    gtk_editable_set_editable(GTK_EDITABLE(xerr_entry),FALSE);
    yerr_entry = gtk_entry_new();						/* Create text entry */
    gtk_entry_set_max_length (GTK_ENTRY (yerr_entry), 16);
    gtk_editable_set_editable(GTK_EDITABLE(yerr_entry),FALSE);

    /* Number of points label and entry */
    nump_label = gtk_label_new(nump_string);
    nump_entry = gtk_entry_new();						/* Create text entry */
    gtk_entry_set_max_length (GTK_ENTRY (nump_entry), 10);
    gtk_editable_set_editable(GTK_EDITABLE(nump_entry),FALSE);
    SetNumPointsEntry(nump_entry, numpoints);

    /* Zoom area */
    zoom_area = gtk_drawing_area_new ();					/* Create new drawing area */
    gtk_widget_set_size_request (zoom_area, ZOOMPIXSIZE, ZOOMPIXSIZE);
    g_signal_connect(G_OBJECT(zoom_area), "expose_event", G_CALLBACK(expose_event_callback), NULL);

    setcolors(&colors);

    /* Remove points buttons and labels */
    remlastbutton = gtk_button_new_with_mnemonic (RemLastBLabel);		/* Create button */
    g_signal_connect (G_OBJECT (remlastbutton), "clicked",			/* Connect button */
                  G_CALLBACK (remove_last), NULL);
    gtk_widget_set_sensitive(remlastbutton,FALSE);
    gtk_widget_set_tooltip_text(remlastbutton,removeltt);

    remallbutton = gtk_button_new_with_mnemonic (RemAllBLabel);			/* Create button */
    g_signal_connect (G_OBJECT (remallbutton), "clicked",			/* Connect button */
                  G_CALLBACK (remove_all), NULL);
    gtk_widget_set_sensitive(remallbutton,FALSE);
        gtk_widget_set_tooltip_text(remallbutton,removeatts);

    /* Logarithmic axes */
    for (i=0;i<2;i++) {
	logcheckb[i] = gtk_check_button_new_with_mnemonic(loglabel[i]);			/* Create check button */
	g_signal_connect (G_OBJECT (logcheckb[i]), "toggled",				/* Connect button */
			  G_CALLBACK (islogxy), GINT_TO_POINTER (i));
        gtk_widget_set_tooltip_text (logcheckb[i],logxytt[i]);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(logcheckb[i]), logxy[i]);
    }

    tophbox = gtk_hbox_new (FALSE, SECT_SEP);
    alignment = gtk_alignment_new (0,0,0,0);
    gtk_table_attach(GTK_TABLE(table), alignment, 0, 1, 0, 1, 5, 0, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), tophbox);

    bottomhbox = gtk_hbox_new (FALSE, SECT_SEP);
    alignment = gtk_alignment_new (0, 0, 1, 1);
    gtk_table_attach(GTK_TABLE(table), alignment, 0, 1, 1, 2, 5, 5, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), bottomhbox);

    /* Packing the axis points labels and entries */
    tlvbox = gtk_vbox_new (FALSE, ELEM_SEP);
    gtk_box_pack_start (GTK_BOX (tophbox), tlvbox, FALSE, FALSE, ELEM_SEP);
    APlabel = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (APlabel), APheader);
    alignment = gtk_alignment_new (0, 1, 0, 0);
    gtk_container_add((GtkContainer *) alignment, APlabel);
    gtk_box_pack_start (GTK_BOX (tlvbox), alignment, FALSE, FALSE, 0);
    table = gtk_table_new(3, 4 ,FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), ELEM_SEP);
    gtk_table_set_col_spacings(GTK_TABLE(table), ELEM_SEP);
    gtk_box_pack_start (GTK_BOX (tlvbox), table, FALSE, FALSE, 0);
    for (i=0;i<4;i++) {
	    gtk_table_attach_defaults(GTK_TABLE(table), setxybutton[i], 0, 1, i, i+1);
	    gtk_table_attach_defaults(GTK_TABLE(table), xy_label[i], 1, 2, i, i+1);
	    gtk_table_attach_defaults(GTK_TABLE(table), xyentry[i], 2, 3, i, i+1);
    }

    /* Packing the point information boxes */
    trvbox = gtk_vbox_new (FALSE, ELEM_SEP);
    gtk_box_pack_start (GTK_BOX (tophbox), trvbox, FALSE, FALSE, ELEM_SEP);

    PIlabel = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (PIlabel), PIheader);
    alignment = gtk_alignment_new (0, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), PIlabel);
    gtk_box_pack_start (GTK_BOX (trvbox), alignment, FALSE, FALSE, 0);

    table = gtk_table_new(4, 2 ,FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), ELEM_SEP);
    gtk_table_set_col_spacings(GTK_TABLE(table), ELEM_SEP);
    gtk_box_pack_start (GTK_BOX (trvbox), table, FALSE, FALSE, 0);
    gtk_table_attach_defaults(GTK_TABLE(table), x_label, 0, 1, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), xc_entry, 1, 2, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), pm_label, 2, 3, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), xerr_entry, 3, 4, 0, 1);
    gtk_table_attach_defaults(GTK_TABLE(table), y_label, 0, 1, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), yc_entry, 1, 2, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), pm_label2, 2, 3, 1, 2);
    gtk_table_attach_defaults(GTK_TABLE(table), yerr_entry, 3, 4, 1, 2);

    /* Pack number of points boxes */
    table = gtk_table_new(3, 1 ,FALSE);
    gtk_table_set_row_spacings(GTK_TABLE(table), 6);
    gtk_table_set_col_spacings(GTK_TABLE(table), 6);
    gtk_box_pack_start (GTK_BOX (trvbox), table, FALSE, FALSE, 0);
    alignment = gtk_alignment_new (0, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), nump_label);
    gtk_table_attach(GTK_TABLE(table), alignment, 0, 1, 0, 1, 0, 0, 0, 0);
    gtk_table_attach(GTK_TABLE(table), nump_entry, 1, 2, 0, 1, 0, 0, 0, 0);

    /* Pack remove points buttons */
    blvbox = gtk_vbox_new (FALSE, GROUP_SEP);
    gtk_box_pack_start (GTK_BOX (bottomhbox), blvbox, FALSE, FALSE, ELEM_SEP);

    subvbox = gtk_vbox_new (FALSE, ELEM_SEP);
    gtk_box_pack_start (GTK_BOX (blvbox), subvbox, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (subvbox), remlastbutton, FALSE, FALSE, 0);	/* Pack button in vert. box */
    gtk_box_pack_start (GTK_BOX (subvbox), remallbutton, FALSE, FALSE, 0);		/* Pack button in vert. box */

    /* Pack zoom area */
    subvbox = gtk_vbox_new (FALSE, ELEM_SEP);
    zoomareabox = subvbox;
    gtk_box_pack_start (GTK_BOX (blvbox), subvbox, FALSE, FALSE, 0);
    ZAlabel = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (ZAlabel), ZAheader);
    alignment = gtk_alignment_new (0, 1, 0, 0);
    gtk_container_add((GtkContainer *) alignment, ZAlabel);
    gtk_box_pack_start (GTK_BOX (subvbox), alignment, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (subvbox), zoom_area, FALSE, FALSE, 0);

    /* Pack logarithmic axes */
    subvbox = gtk_vbox_new (FALSE, ELEM_SEP);
    logbox = subvbox;
    gtk_box_pack_start (GTK_BOX (blvbox), subvbox, FALSE, FALSE, 0);
    Llabel = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (Llabel), Lheader);
    alignment = gtk_alignment_new (0, 1, 0, 0);
    gtk_container_add((GtkContainer *) alignment, Llabel);
    gtk_box_pack_start (GTK_BOX (subvbox), alignment, FALSE, FALSE, 0);
    for (i=0;i<2;i++) {
	gtk_box_pack_start (GTK_BOX (subvbox), logcheckb[i], FALSE, FALSE, 0);			/* Pack checkbutton in vert. box */
    }

    /* Create and pack radio buttons for sorting */
    group = NULL;
    for (i=0;i<ORDERBNUM;i++) {
	ordercheckb[i] = gtk_radio_button_new_with_label (group, orderlabel[i]);	/* Create radio button */
	g_signal_connect (G_OBJECT (ordercheckb[i]), "toggled",				/* Connect button */
			  G_CALLBACK (SetOrdering), GINT_TO_POINTER (i));
	group = gtk_radio_button_get_group (GTK_RADIO_BUTTON (ordercheckb[i]));		/* Get buttons group */
    }
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (ordercheckb[0]), TRUE);		/* Set no ordering button active */

    subvbox = gtk_vbox_new (FALSE, ELEM_SEP);
    oppropbox = subvbox;
    gtk_box_pack_start (GTK_BOX (blvbox), subvbox, FALSE, FALSE, 0);
    Olabel = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (Olabel), Oheader);
    alignment = gtk_alignment_new (0, 1, 0, 0);
    gtk_container_add((GtkContainer *) alignment, Olabel);
    gtk_box_pack_start (GTK_BOX (subvbox), alignment, FALSE, FALSE, 0);
    for (i=0;i<ORDERBNUM;i++) {
	gtk_box_pack_start (GTK_BOX (subvbox), ordercheckb[i], FALSE, FALSE, 0);			/* Pack radiobutton in vert. box */
    }

    /* Create and pack value errors button */
    UseErrCheckB = gtk_check_button_new_with_mnemonic(PrintErrCBLabel);
    g_signal_connect (G_OBJECT (UseErrCheckB), "toggled",
		      G_CALLBACK (UseErrCB), NULL);
    gtk_widget_set_tooltip_text (UseErrCheckB,uetts);
    gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON(UseErrCheckB), UseErrors);

    Elabel = gtk_label_new (NULL);
    gtk_label_set_markup (GTK_LABEL (Elabel), Eheader);
    alignment = gtk_alignment_new (0, 1, 0, 0);
    gtk_container_add(GTK_CONTAINER(alignment), Elabel);
    gtk_box_pack_start (GTK_BOX (subvbox), alignment, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (subvbox), UseErrCheckB, FALSE, FALSE, 0);

    /* Print current image name in title bar*/
    buffer = g_strdup_printf(Window_Title, filename);
    gtk_window_set_title (GTK_WINDOW (window), buffer);

    brvbox = gtk_vbox_new (FALSE, GROUP_SEP);
    gtk_box_pack_start (GTK_BOX (bottomhbox), brvbox, TRUE, TRUE, 0);

    /* Create a scroll window to hold image */
    ScrollWindow = gtk_scrolled_window_new(NULL,NULL);
    gtk_scrolled_window_set_policy (GTK_SCROLLED_WINDOW(ScrollWindow), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
    ViewPort = gtk_viewport_new(NULL,NULL);
    gtk_box_pack_start (GTK_BOX (brvbox), ScrollWindow, TRUE, TRUE, 0);
    drawing_area_alignment = gtk_alignment_new (0, 0, 0, 0);
    gtk_container_add (GTK_CONTAINER (ViewPort), drawing_area_alignment);
    gtk_container_add (GTK_CONTAINER (ScrollWindow), ViewPort);

    gtk_widget_show_all(window);

    if (InsertImage(filename, Scale, maxX, maxY) == -1) {
	return -1;
    }

    if (UsePreSetCoords) {
	axiscoords[0][0] = 0;
	axiscoords[0][1] = YSize-1;
	axiscoords[1][0] = XSize-1;
	axiscoords[1][1] = YSize-1;
	axiscoords[2][0] = 0;
	axiscoords[2][1] = YSize-1;
	axiscoords[3][0] = 0;
	axiscoords[3][1] = 0;
	for (i=0;i<4;i++) {
	    gtk_widget_set_sensitive(xyentry[i],TRUE);
	    gtk_editable_set_editable(GTK_EDITABLE(xyentry[i]),TRUE);
        g_ascii_formatd(buf, 20, "%lf", realcoords[i]);
	    gtk_entry_set_text(GTK_ENTRY(xyentry[i]), buf);
	    valueset[i] = TRUE;
	    bpressed[i] = TRUE;
	    setxypressed[i]=FALSE;
	}
    }

    gtk_action_group_set_sensitive(tab_action_group, TRUE);

    if (ShowZoomArea)
        if (zoomareabox != NULL)
            gtk_widget_show(zoomareabox);
    if (ShowLog)
        if (logbox != NULL)
            gtk_widget_show(logbox);
    if (ShowOpProp)
        if (oppropbox != NULL)
            gtk_widget_show(oppropbox);

  return 0;
}


/****************************************************************/
/****************************************************************/
static void drag_data_received(GtkWidget *widget,
                              GdkDragContext *drag_context,
                              gint x, gint y,
                              GtkSelectionData *data,
                              guint info,
                              guint event_time,
                              gpointer user_data)
{
    gchar *filename;
    gchar **uri_list;
    gint i;
    GError *error;

    if (info == URI_LIST) {
        uri_list = gtk_selection_data_get_uris(data);
        i = 0;
        while (uri_list[i] != NULL) {
            error = NULL;
            filename = g_filename_from_uri(uri_list[i], NULL, &error);
            if (filename == NULL) {
                g_message("Null filename: %s", error->message);
                g_error_free(error);
            } else{
        	    SetupNewTab(filename, 1.0, -1, -1, FALSE);
            }
            i++;
        }
        g_strfreev(uri_list);
    }
    gtk_drag_finish (drag_context, TRUE, FALSE, event_time);
}


/****************************************************************/
/* This callback handles the file - open dialog.		*/
/****************************************************************/
static GCallback menu_file_open(void)
{
  GtkWidget *dialog, *scalespinbutton, *hboxextra, *scalelabel;
  GtkImage *preview;
  GtkAdjustment *scaleadj;
  GtkFileFilter *filefilter;

    dialog = gtk_file_chooser_dialog_new ("Open File",
				          GTK_WINDOW (window),
				          GTK_FILE_CHOOSER_ACTION_OPEN,
				          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
				          GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
				          NULL);
    
// Set filtering of files to open to filetypes gdk_pixbuf can handle
    filefilter = gtk_file_filter_new();
    gtk_file_filter_add_pixbuf_formats(filefilter);
    gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(dialog), GTK_FILE_FILTER(filefilter));


    hboxextra = gtk_hbox_new(FALSE, ELEM_SEP);

    scalelabel = gtk_label_new(scale_string);

    scaleadj = (GtkAdjustment *) gtk_adjustment_new(1, 0.1, 100, 0.1, 0.1, 0);
    scalespinbutton = gtk_spin_button_new(GTK_ADJUSTMENT(scaleadj), 0.1, 1);

    gtk_box_pack_start (GTK_BOX (hboxextra), scalelabel, FALSE, FALSE, 0);
    gtk_box_pack_start (GTK_BOX (hboxextra), scalespinbutton, FALSE, FALSE, 0);

    gtk_file_chooser_set_extra_widget(GTK_FILE_CHOOSER(dialog), hboxextra);

    gtk_widget_show(hboxextra);
    gtk_widget_show(scalelabel);
    gtk_widget_show(scalespinbutton);

    preview = (GtkImage *) gtk_image_new ();
    gtk_file_chooser_set_preview_widget (GTK_FILE_CHOOSER(dialog), GTK_WIDGET(preview));
    g_signal_connect (dialog, "update-preview",
		      G_CALLBACK (update_preview_cb), preview);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
	char *filename;

    	filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
	SetupNewTab(filename, gtk_spin_button_get_value(GTK_SPIN_BUTTON(scalespinbutton)), -1, -1, FALSE);

    	g_free (filename);
    }

    gtk_widget_destroy (dialog);

    return NULL;
}

/* The File -> Save As... dialog.		*/
static void file_save_as_dialog (GtkAction *action, gpointer data)
{
    GtkWidget *dialog;
    gchar *filename;
    FILE *fp;

    dialog = gtk_file_chooser_dialog_new ("Save As...",
                                          GTK_WINDOW (window),
                                          GTK_FILE_CHOOSER_ACTION_OPEN,
                                          GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
                                          GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT,
                                          NULL);

    if (gtk_dialog_run (GTK_DIALOG (dialog)) == GTK_RESPONSE_ACCEPT) {
        filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (dialog));
        fp = fopen (filename, "w");
        if (fp == NULL) {
            g_free (filename);
            return;
        } else {
            print_results (fp);
            g_free (filename);
        }
    }

    gtk_widget_destroy (dialog);

    return;
}


/****************************************************************/
/* This Callback generates the help - about dialog.		*/
/****************************************************************/
static GCallback menu_help_about(void)
{
    const gchar *authors[] = AUTHORS;

    gtk_show_about_dialog(GTK_WINDOW(window), 
	"authors", authors, 
	"comments", COMMENTS,
	"copyright", COPYRIGHT,
	"license", LICENSE,
	"program-name", PROGNAME,
	"version", VERSION,
	"website", HOMEPAGEURL,
	"website-label", HOMEPAGELABEL,
	NULL);

  return NULL;
}


/****************************************************************/
/* This callback handles the fullscreen toggling.		*/
/****************************************************************/
static GCallback full_screen_action_callback(GtkWidget *widget, gpointer func_data)
{
    if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget))) {
	gtk_window_fullscreen(GTK_WINDOW (window));
    } else {
	gtk_window_unfullscreen(GTK_WINDOW (window));
    }
  return NULL;
}

/****************************************************************/
/* This callback handles the hide zoom area toggling.		*/
/****************************************************************/
static GCallback hide_zoom_area_callback(GtkWidget *widget, gpointer func_data)
{
    if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget))) {
        if (zoomareabox != NULL)
            gtk_widget_show(zoomareabox);
    	ShowZoomArea = TRUE;
    } else {
        if (zoomareabox != NULL)
            gtk_widget_hide(zoomareabox);
    	ShowZoomArea = FALSE;
    }
    return NULL;
}

/****************************************************************/
/* This callback handles the hide axis settings toggling.	*/
/****************************************************************/
static GCallback hide_axis_settings_callback(GtkWidget *widget, gpointer func_data)
{
    if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget))) {
        if (logbox != NULL)
            gtk_widget_show(logbox);
    	ShowLog = TRUE;
    } else {
        if (logbox != NULL)
            gtk_widget_hide(logbox);
	    ShowLog = FALSE;
    }
    return NULL;
}

/****************************************************************/
/* This callback handles the hide output properties toggling.	*/
/****************************************************************/
static GCallback hide_output_prop_callback(GtkWidget *widget, gpointer func_data)
{
    if (gtk_toggle_action_get_active(GTK_TOGGLE_ACTION(widget))) {
        if (oppropbox != NULL)
            gtk_widget_show(oppropbox);
    	ShowOpProp = TRUE;
    } else {
        if (oppropbox != NULL)
            gtk_widget_hide(oppropbox);
    	ShowOpProp = FALSE;
    }
    return NULL;
}


/****************************************************************/
/* This is the main function, this function gets called when	*/
/* the program is executed. It allocates the necessary work-	*/
/* spaces and initialized the main window and its widgets.	*/
/****************************************************************/
int main (int argc, char **argv)
{
  gint 		FileIndex, i, maxX, maxY;
  gdouble 	Scale;
  gboolean	UsePreSetCoords, UseError, Uselogxy[2];
  gdouble	TempCoords[4] = {0.0, 0.0, 0.0, 0.0};

  GtkWidget *menubar;
  GtkActionGroup *action_group;
  GtkUIManager *ui_manager;
  GtkAccelGroup *accel_group;
  GError *error;

#include "vardefs.h"

    gtk_init (&argc, &argv);								/* Init GTK */

    if (argc > 1) if (strcmp(argv[1],"-h")==0 ||					/* If no parameters given, -h or --help */
	strcmp(argv[1],"--help")==0) {
	printf("%s",HelpText);								/* Print help */
	exit(0);									/* and exit */
    }

    maxX = -1;
    maxY = -1;
    Scale = -1;
   UseError = FALSE;
    UsePreSetCoords = FALSE;
    Uselogxy[0] = FALSE;
    Uselogxy[1] = FALSE;
    for (i=1;i<argc;i++) {
	if (*(argv[i])=='-') {
	    if (strcmp(argv[i],"-scale")==0) {
		if (argc-i < 2) {
		    printf("Too few parameters for -scale\n");
		    exit(0);
		}
		if (sscanf(argv[i+1],"%lf",&Scale)!=1) {
		    printf("-scale parameter in invalid form !\n");
		    exit(0);
		}
		i++;
		if (i >= argc) break;
	    } else if (strcmp(argv[i],"-errors")==0) {
		UseError = TRUE;
	    } else if (strcmp(argv[i],"-lnx")==0) {
		Uselogxy[0] = TRUE;
	    } else if (strcmp(argv[i],"-lny")==0) {
		Uselogxy[1] = TRUE;
	    } else if (strcmp(argv[i],"-max")==0) {
		if (argc-i < 3) {
		    printf("Too few parameters for -max\n");
		    exit(0);
		}
		if (sscanf(argv[i+1],"%d", &maxX)!=1) {
		    printf("-max first parameter in invalid form !\n");
		    exit(0);
		}
		if (sscanf(argv[i+2],"%d", &maxY)!=1) {
		    printf("-max second parameter in invalid form !\n");
		    exit(0);
		}
		i+=2;
		if (i >= argc) break;
	    } else if (strcmp(argv[i],"-coords")==0) {
		UsePreSetCoords = TRUE;
		if (argc-i < 5) {
		    printf("Too few parameters for -coords\n");
		    exit(0);
		}
		if (sscanf(argv[i+1],"%lf", &TempCoords[0])!=1) {
		    printf("-max first parameter in invalid form !\n");
		    exit(0);
		}
		if (sscanf(argv[i+2],"%lf", &TempCoords[1])!=1) {
		    printf("-max second parameter in invalid form !\n");
		    exit(0);
		} 
		if (sscanf(argv[i+3],"%lf", &TempCoords[2])!=1) {
		    printf("-max third parameter in invalid form !\n");
		    exit(0);
		} 
		if (sscanf(argv[i+4],"%lf", &TempCoords[3])!=1) {
		    printf("-max fourth parameter in invalid form !\n");
		    exit(0);
		}
		i+=4;
		if (i >= argc) break;
/*	    } else if (strcmp(argv[i],"-hidelog")==0) {
		HideLog = TRUE;
	    } else if (strcmp(argv[i],"-hideza")==0) {
		HideZoomArea = TRUE;
	    } else if (strcmp(argv[i],"-hideop")==0) {
		HideOpProp = TRUE; */
	    } else {
		printf("Unknown parameter : %s\n", argv[i]);
		exit(0);
	    }
	    continue;
	} else {
	    FileIndex = i;
	}
    }


    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);					/* Create window */
    gtk_window_set_default_size(GTK_WINDOW(window), 640, 480);
    gtk_window_set_title(GTK_WINDOW (window), Window_Title_NoneOpen);			/* Set window title */
    gtk_window_set_resizable(GTK_WINDOW (window), TRUE);
    gtk_container_set_border_width(GTK_CONTAINER (window), 0);				/* Set borders in window */
    mainvbox = gtk_vbox_new(FALSE, 0);
    gtk_container_add( GTK_CONTAINER(window), mainvbox);

    g_signal_connect(G_OBJECT (window), "delete_event",					/* Init delete event of window */
                        G_CALLBACK (close_application), NULL);

    gtk_drag_dest_set(window, GTK_DEST_DEFAULT_ALL, ui_drop_target_entries, NUM_IMAGE_DATA, (GDK_ACTION_COPY | GDK_ACTION_MOVE));
    g_signal_connect(G_OBJECT (window), "drag-data-received",				/* Drag and drop catch */
                        G_CALLBACK (drag_data_received), NULL);

/* Create menues */
    action_group = gtk_action_group_new("MenuActions");
    gtk_action_group_add_actions(action_group, entries, G_N_ELEMENTS (entries), window);
    gtk_action_group_add_toggle_actions(action_group, full_screen, G_N_ELEMENTS (full_screen), window);

    tab_action_group = gtk_action_group_new("TabActions");
    gtk_action_group_add_toggle_actions(tab_action_group, toggle_entries, G_N_ELEMENTS (toggle_entries), window);
    gtk_action_group_set_sensitive(tab_action_group, FALSE);

    ui_manager = gtk_ui_manager_new();
    gtk_ui_manager_insert_action_group(ui_manager, action_group, 0);
    gtk_ui_manager_insert_action_group(ui_manager, tab_action_group, 0);
 
    accel_group = gtk_ui_manager_get_accel_group(ui_manager);
    gtk_window_add_accel_group(GTK_WINDOW (window), accel_group);
 
    error = NULL;
    if (!gtk_ui_manager_add_ui_from_string(ui_manager, ui_description, -1, &error)) {
        g_message("building menus failed: %s", error->message);
        g_error_free(error);
        exit(EXIT_FAILURE);
    }
 
    menubar = gtk_ui_manager_get_widget(ui_manager, "/MainMenu");
    gtk_box_pack_start(GTK_BOX (mainvbox), menubar, FALSE, FALSE, 0);

    realcoords[0] = TempCoords[0];
    realcoords[2] = TempCoords[1];
    realcoords[1] = TempCoords[2];
    realcoords[3] = TempCoords[3];
    logxy[0] = Uselogxy[0];
    logxy[1] = Uselogxy[1];
    UseErrors = UseError;
    SetupNewTab(argv[FileIndex], Scale, maxX, maxY, UsePreSetCoords);

    g_signal_connect_swapped (G_OBJECT (window), "key_press_event",
			          G_CALLBACK (key_press_event), NULL);

    gtk_widget_show_all(window);							/* Show all widgets */

    gtk_main();										/* This is where it all starts */
              
    free(colors);									/* Deallocate memory */

    return(0);										/* Exit. */
}
