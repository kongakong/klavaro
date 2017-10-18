/*****************************************************************************/
/*  Klavaro - a flexible touch typing tutor                                  */
/*  Copyright (C) 2005, 2006, 2007, 2008 Felipe Castro                       */
/*  Copyright (C) 2009, 2010, 2011, 2012, 2013 The Free Software Foundation  */
/*                                                                           */
/*  This program is free software, licensed under the terms of the GNU       */
/*  General Public License as published by the Free Software Foundation,     */
/*  either version 3 of the License, or (at your option) any later version.  */
/*  You should have received a copy of the GNU General Public License        */
/*  along with this program.  If not, see <http://www.gnu.org/licenses/>.    */
/*****************************************************************************/

/*
 * Charts management
 */
#include <math.h>
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gdk/gdk.h>
#include <gtk/gtk.h>
#include <gtkdatabox_typedefs.h>
#include <gtkdatabox.h>
#include <gtkdatabox_grid.h>
#include <gtkdatabox_lines.h>
#include <gtkdatabox_points.h>
#include <gtkdatabox_bars.h>

#include "auxiliar.h"
#include "main.h"
#include "callbacks.h"
#include "tutor.h"
#include "basic.h"
#include "accuracy.h"
#include "keyboard.h"
#include "translation.h"
#include "plot.h"

static struct
{
	GtkWidget *databox;
	GtkWidget *gtkgrid;

	struct
	{
		gfloat x[DATA_POINTS+1];
		gfloat y[DATA_POINTS+1];
	} data;
	GtkDataboxGraph *point_kernel;
	GtkDataboxGraph *point_frame;
	GtkDataboxGraph *line_kernel;
	GtkDataboxGraph *line_frame;
	GtkDataboxGraph *line_outter;

	struct
	{
		gfloat x[1];
		gfloat y[1];
	} mark;
	GtkDataboxGraph *point_marker;

	struct
	{
		gfloat x[2];
		gfloat y[2];
	} goal;
	GtkDataboxGraph *line_goal;
	GtkDataboxGraph *grid;
	GtkWidget * label_y[MAX_Y_LABELS];

	struct
	{
		gfloat x[2];
		gfloat y[2];
	} lim;
	GtkDataboxGraph *limits;
} plot;

gfloat accur[DATA_POINTS+1];
gfloat velo[DATA_POINTS+1];
gfloat fluid[DATA_POINTS+1];
gfloat score[DATA_POINTS+1];
gchar date[DATA_POINTS+1][20];
gchar hour[DATA_POINTS+1][20];
gint nchars[DATA_POINTS+1];
gchar lesson[DATA_POINTS+1][299];
gchar language[DATA_POINTS+1][80+1];
glong n_points;
gint plot_type; /* used to communicate the plotting type, for updating the cursor marker, etc */

/*******************************************************************************
 * Interface functions
 */
GtkWidget *
plot_get_databox ()
{
	return plot.databox;
}

/*******************************************************************************
 * Private functions

static void
plot_clip_data (gint i)
{
		if (accur[i] > 100)
			accur[i] = 100.1;
		if (accur[i] < 60)
			accur[i] = 59.9;
		if (velo[i] > 100)
			velo[i] = 100.1;
		if (velo[i] < 0)
			velo[i] = -0.1;
		if (fluid[i] > 100)
			fluid[i] = 100.1;
		if (fluid[i] < 0)
			fluid[i] = -0.1;
		if (score[i] < 0)
			score[i] = -0.1;
}
 */

static void
plot_error_frequencies ()
{
	gint i;
	GdkRGBA color;
	GdkRGBA color_black;
	GtkDatabox *box;

	box = GTK_DATABOX (plot.databox);
	n_points = accur_terror_n_get ();
	if (n_points < 1)
		return;
	accur_terror_sort ();
	for (i = 0; i < DATA_POINTS; i++)
	{
		if (i < n_points)
			plot.data.y[i] = accur_wrong_get (i);
		else
			plot.data.y[i] = 0;
	}

	/* Format the chart
	 */
	plot.mark.x[0] = -7;
	plot.mark.y[0] = -7;
	plot.lim.x[0] = 0;
	plot.lim.x[1] = DATA_POINTS + 2;
	plot.lim.y[0] = 0;
	plot.lim.y[1] = 1.05 * accur_wrong_get (0);
	if (plot.lim.y[1] < 1)
		plot.lim.y[1] = 1.05;

	/* White background
	 */
	gdk_rgba_parse (&color, "#ffffff");
	gtk_widget_override_background_color (plot.databox, GTK_STATE_FLAG_NORMAL, &color);
	 
	gdk_rgba_parse (&color_black, "#000000");
	gdk_rgba_parse (&color, PLOT_GREEN_2);

	/* Point limits */
	plot.limits = gtk_databox_points_new (2, plot.lim.x, plot.lim.y, &color_black, 1);
	gtk_databox_graph_add (box, plot.limits);
	gtk_databox_auto_rescale (box, 0.0);

	/* Bar kernel
	 */
	plot.point_kernel = gtk_databox_bars_new (i, plot.data.x, plot.data.y, &color, 5);
	gtk_databox_graph_add (box, plot.point_kernel);

	/* Bar frame
	 */
	plot.point_frame = gtk_databox_bars_new (i, plot.data.x, plot.data.y, &color_black, 7);
	gtk_databox_graph_add (box, plot.point_frame);

	/* Data marker
	 */
	plot.point_marker = gtk_databox_points_new (1, plot.mark.x, plot.mark.y, &color_black, 7);
	gtk_databox_graph_add (box, plot.point_marker);

	/* Redraw the plot
	 */
	gtk_widget_show_all (plot.gtkgrid);
}

static void
plot_touch_times ()
{
	gint i;
	GdkRGBA color;
	GdkRGBA color_black;
	GtkDatabox *box;

	box = GTK_DATABOX (plot.databox);
	n_points = accur_ttime_n_get ();
	if (n_points < 1)
		return;
	accur_ttime_sort ();
	for (i = 0; i < DATA_POINTS; i++)
	{
		if (i < n_points)
			plot.data.y[i] = accur_profi_aver (i);
		else
			plot.data.y[i] = 0;
	}

	/* Format the chart
	 */
	plot.mark.x[0] = -7;
	plot.mark.y[0] = -7;
	plot.lim.x[0] = 0;
	plot.lim.x[1] = DATA_POINTS + 2;
	plot.lim.y[0] = 0;
	plot.lim.y[1] = 1.05 * accur_profi_aver (0);
	if (plot.lim.y[1] < 0.01)
		plot.lim.y[1] = 0.0105;

	/* White background
	 */
	gdk_rgba_parse (&color, "#ffffff");
	gtk_widget_override_background_color (plot.databox, GTK_STATE_FLAG_NORMAL, &color);
	 
	gdk_rgba_parse (&color_black, "#000000");
	gdk_rgba_parse (&color, PLOT_PURPLE);

	/* Point limits */
	plot.limits = gtk_databox_points_new (2, plot.lim.x, plot.lim.y, &color_black, 1);
	gtk_databox_graph_add (box, plot.limits);
	gtk_databox_auto_rescale (box, 0.0);

	/* Bar kernel
	 */
	plot.point_kernel = gtk_databox_bars_new (i, plot.data.x, plot.data.y, &color, 5);
	gtk_databox_graph_add (box, plot.point_kernel);

	/* Bar frame
	 */
	plot.point_frame = gtk_databox_bars_new (i, plot.data.x, plot.data.y, &color_black, 7);
	gtk_databox_graph_add (box, plot.point_frame);

	/* Data marker
	 */
	plot.point_marker = gtk_databox_points_new (1, plot.mark.x, plot.mark.y, &color_black, 7);
	gtk_databox_graph_add (box, plot.point_marker);

	/* Redraw the plot
	 */
	gtk_widget_show_all (plot.gtkgrid);
}

/*******************************************************************************
 * Functions to manage the plottings on the progress window
 */
void
plot_initialize ()
{
	static gboolean inited = FALSE;
	gint i;

	if (inited)
		return;
	inited = TRUE;

	/* Initialize X data
	 */
	for (i = 0; i < DATA_POINTS; i++)
		plot.data.x[i] = i + 1;

	/* Data Box
	 */
	gtk_databox_create_box_with_scrollbars_and_rulers (&plot.databox, &plot.gtkgrid, FALSE, FALSE, FALSE, FALSE);
	gtk_container_add (GTK_CONTAINER (get_wg ("frame_stat")), plot.gtkgrid);
	g_signal_connect (G_OBJECT (plot.databox), "motion_notify_event", G_CALLBACK (on_databox_hovered), NULL);

	/* Y labels
	 */
	for (i = 0; i < MAX_Y_LABELS; i++)
	{
		plot.label_y[i] = gtk_label_new ("???");
		gtk_box_pack_start (GTK_BOX (get_wg ("box_grid_label_y")), plot.label_y[i], TRUE, TRUE, 0);
	}

	plot_draw_chart (1);
}

/**********************************************************************
 * Plots the statistics
 */
void
plot_draw_chart (gint field)
{
	gint i, len;
	gint lesson_n;
	gchar *kb_name;
	gchar *tmp_name;
	gchar *tmp_locale;
	gchar *tmp_lang;
	gchar tmp_str[2000];
	FILE *fh;
	GdkRGBA color, color2, color3;
	GdkRGBA color_black;
	GtkDatabox *box;

	box = GTK_DATABOX (plot.databox);
	/* Blank the chart
	 */
	n_points = 0;
	gtk_databox_graph_remove_all (box);
	gtk_widget_hide (plot.gtkgrid);

	/* Set plot type for external reference */
	plot_type = field;

	/* Error frequencies or touch times
	 */
	gtk_widget_set_tooltip_text (get_wg ("entry_stat_x"), _("Character"));
	gtk_widget_hide (get_wg ("box_grid_label_y"));
	if (field == 6)
	{
		plot_error_frequencies ();
		return;
	}
	else if (field == 7)
	{
		plot_touch_times ();
		return;
	}
	gtk_widget_set_tooltip_text (get_wg ("entry_stat_x"), _("Date & Time"));
	gtk_widget_show (get_wg ("box_grid_label_y"));

	/* Auxiliar variable to track the lesson to be plot
	 */
	lesson_n = gtk_spin_button_get_value (GTK_SPIN_BUTTON (get_wg ("spinbutton_stat_lesson")));

	if (tutor_get_type () == TT_BASIC)
	{
		gtk_widget_show (get_wg ("label_stat_lesson"));
		gtk_widget_show (get_wg ("spinbutton_stat_lesson"));
	}
	else
	{
		gtk_widget_hide (get_wg ("label_stat_lesson"));
		gtk_widget_hide (get_wg ("spinbutton_stat_lesson"));
	}

	/* Get the file name
	 */
	if (field < 4)
		tmp_name = g_strconcat (main_path_stats (), G_DIR_SEPARATOR_S "stat_",
			       	tutor_get_type_name (), ".txt", NULL);
	else
		tmp_name = g_build_filename (main_path_stats (), "scores_fluid.txt", NULL);

	/* Open the data file
	 */
	fh = (FILE *) g_fopen (tmp_name, "r");
	if (!fh)
	{
		g_message ("no data yet, no statistic file:\n%s", tmp_name);
		g_free (tmp_name);
		return;
	}
	
	/* Change to "C" locale, but keep current language
	 */
        tmp_lang = trans_get_current_language ();
	tmp_locale = g_strdup (setlocale (LC_NUMERIC, NULL));
	if (tmp_locale != NULL)
		setlocale (LC_NUMERIC, "C");

	/* Keyboard names are compared without spaces (may be needed for custom files)
	 */
	kb_name = g_strdup (keyb_get_name ());
	for (i=0; kb_name[i]; i++)
		kb_name[i] = (kb_name[i] == ' ') ? '_' : kb_name[i];

	/* Read the first line (header)
	 */
	if (!fgets (tmp_str, 2000, fh))
	{
		g_message ("no data on the statistic file:\n%s", tmp_name);
		g_free (tmp_name);
		fclose (fh);
		return;
	}

	/* Read the first DATA_POINTS points
	 */
	for (i = 0; i < DATA_POINTS; i++)
		plot.data.y[i] = -1000;
	i = 0;
	while (1)
	{
		gint itens;
		gchar *lang_extra;

		language[i][0] = '\0';
		if (field < 4)
		{
			itens = fscanf (fh, "%f%f%f%s%s%s\t", &accur[i], &velo[i], &fluid[i],
				       	date[i], hour[i], lesson[i]);
			if (itens != 6)	break;
		}
		else
		{
			itens = fscanf (fh, "%f%s%s%i\t", &score[i], date[i], hour[i], &nchars[i]);
			if (itens != 4)	break;
		}
		lang_extra = fgets (language[i], 80, fh); 
		if (language[i][len = (strlen(language[i])-1)] == '\n')
			language[i][len] = '\0';

		//plot_clip_data (i);

		if (tutor_get_type () == TT_BASIC)
		{
			if (g_ascii_strtoll (lesson[i], NULL, 10) != lesson_n)
				continue;
			if (strcmp (language[i], kb_name) != 0)
				continue;
		}
		if (tutor_get_type () == TT_ADAPT && strcmp (lesson[i], kb_name) != 0)
			continue;
		if (tutor_get_type () == TT_VELO && strcmp (language[i], tmp_lang) != 0)
			continue;
		if (tutor_get_type () == TT_FLUID && strcmp (language[i], tmp_lang) != 0)
			continue;
		switch (field)
		{
		case 1:
			plot.data.y[i] = accur[i];
			break;
		case 2:
			plot.data.y[i] = velo[i];
			break;
		case 3:
			plot.data.y[i] = fluid[i];
			break;
		case 4:
			plot.data.y[i] = score[i];
			break;
		}
		if (++i == DATA_POINTS)
			break;
	}

	/* Read until the end, keeping the last DATA_POINTS points 
	 */
	while (1)
	{
		gint itens;
		gchar *lang_extra;

		language[i][0] = '\0';
		if (field < 4)
		{
			itens = fscanf (fh, "%f%f%f%s%s%s\t", &accur[i], &velo[i], &fluid[i],
				       	date[i], hour[i], lesson[i]);
			if (itens != 6)	break;
		}
		else
		{
			itens = fscanf (fh, "%f%s%s%i\t", &score[i], date[i], hour[i], &nchars[i]);
			if (itens != 4)	break;
		}
		lang_extra = fgets (language[i], 80, fh); 
		if (language[i][len = (strlen(language[i])-1)] == '\n')
			language[i][len] = '\0';

		//plot_clip_data (i);

		if (tutor_get_type () == TT_BASIC)
		{
			if (g_ascii_strtoll (lesson[i], NULL, 10) != lesson_n)
				continue;
			if (strcmp (language[i], kb_name) != 0)
				continue;
		}
		if (tutor_get_type () == TT_ADAPT && strcmp (lesson[i], kb_name) != 0)
			continue;
		if (tutor_get_type () == TT_VELO && strcmp (language[i], tmp_lang) != 0)
			continue;
		if (tutor_get_type () == TT_FLUID && strcmp (language[i], tmp_lang) != 0)
			continue;
		for (i = 0; i < DATA_POINTS - 1; i++)
		{
			plot.data.y[i] = plot.data.y[i + 1];
			strcpy (date[i], date[i + 1]);
			strcpy (hour[i], hour[i + 1]);
		}
		strcpy (date[i], date[i + 1]);
		strcpy (hour[i], hour[i + 1]);

		switch (field)
		{
		case 1:
			plot.data.y[i] = accur[i + 1];
			break;
		case 2:
			plot.data.y[i] = velo[i + 1];
			break;
		case 3:
			plot.data.y[i] = fluid[i + 1];
			break;
		case 4:
			plot.data.y[i] = score[i + 1];
			break;
		}
		i = DATA_POINTS;
	}
	fclose (fh);
	g_free (kb_name);
	g_free (tmp_name);

	/* Coming back to the right locale
	 */
	if (tmp_locale != NULL)
		setlocale (LC_NUMERIC, tmp_locale);
	g_free (tmp_locale);

	/* Set apropriate background
	 */
	if (i == 0)
	{
		g_message ("no valid data to plot.");
		gdk_rgba_parse (&color, "#ccccce");
		gtk_widget_override_background_color (plot.databox, GTK_STATE_FLAG_NORMAL, &color);
		return;
	}
	else
	{
		gdk_rgba_parse (&color, "#ffffff");
		gtk_widget_override_background_color (plot.databox, GTK_STATE_FLAG_NORMAL, &color);
	}
	n_points = i;

	/* Format the chart
	 */
	plot.mark.x[0] = -7;
	plot.mark.y[0] = -7;
	plot.goal.x[0] = plot.lim.x[0] = 0;
	plot.goal.x[1] = plot.lim.x[1] = DATA_POINTS + 2;
	plot.lim.y[0] = 0;
	plot.lim.y[1] = 100;
	 
	gdk_rgba_parse (&color_black, "#000000");
	switch (field)
	{
	case 1:
		plot.lim.y[0] = 60;
		plot.goal.y[0] = tutor_goal_accuracy ();
		gdk_rgba_parse (&color, PLOT_GREEN);
		gdk_rgba_parse (&color2, PLOT_GREEN_2);
		gdk_rgba_parse (&color3, PLOT_GREEN_3);
		break;
	case 2:
		plot.lim.y[1] = 120;
		plot.goal.y[0] = tutor_goal_speed ();
		gdk_rgba_parse (&color, PLOT_RED);
		gdk_rgba_parse (&color2, PLOT_RED_2);
		gdk_rgba_parse (&color3, PLOT_RED_3);
		break;
	case 3:
		plot.goal.y[0] = tutor_goal_fluidity ();
		gdk_rgba_parse (&color, PLOT_BLUE);
		gdk_rgba_parse (&color2, PLOT_BLUE_2);
		gdk_rgba_parse (&color3, PLOT_BLUE_3);
		break;
	case 4:
		plot.lim.y[1] = 10;
		plot.goal.y[0] = -1;
		gdk_rgba_parse (&color, PLOT_ORANGE);
		gdk_rgba_parse (&color2, PLOT_ORANGE_2);
		gdk_rgba_parse (&color3, PLOT_ORANGE_3);
	}
	plot.goal.y[1] = plot.goal.y[0];

	/* Point limits */
	plot.limits = gtk_databox_points_new (2, plot.lim.x, plot.lim.y, &color_black, 1);
	gtk_databox_graph_add (box, plot.limits);
	gtk_databox_auto_rescale (box, 0.0);

	/* Point kernel */
	plot.point_kernel = gtk_databox_points_new (i, plot.data.x, plot.data.y, &color, 3);
	gtk_databox_graph_add (box, plot.point_kernel);

	/* Point frame */
	plot.point_frame = gtk_databox_points_new (i, plot.data.x, plot.data.y, &color_black, 5);
	gtk_databox_graph_add (box, plot.point_frame);

	/* Data marker */
	plot.point_marker = gtk_databox_points_new (1, plot.mark.x, plot.mark.y, &color_black, 7);
	gtk_databox_graph_add (box, plot.point_marker);

	/* Kernel line */
	plot.line_kernel = gtk_databox_lines_new (i, plot.data.x, plot.data.y, &color, 1);
	gtk_databox_graph_add (box, plot.line_kernel);

	/* Frame line */
	plot.line_frame = gtk_databox_lines_new (i, plot.data.x, plot.data.y, &color2, 3);
	gtk_databox_graph_add (box, plot.line_frame);

	/* Outter line */
	plot.line_outter = gtk_databox_lines_new (i, plot.data.x, plot.data.y, &color3, 5);
	gtk_databox_graph_add (box, plot.line_outter);

	/* Goal limit */
	gdk_rgba_parse (&color3, "#999999");
	plot.line_goal = gtk_databox_lines_new (2, plot.goal.x, plot.goal.y, &color3, 1);
	gtk_databox_graph_add (box, plot.line_goal);

	/* Grid and y labels */
	gdk_rgba_parse (&color, "#dddddd");
	if (field == 1) /* Correctness (%) */
	{
		plot.grid = gtk_databox_grid_new (3, 3, &color, 1);
		for (i = 0; i < 5; i++)
		{
			g_sprintf (tmp_str, "%u", 100 - 10*i);
			gtk_label_set_text (GTK_LABEL (plot.label_y[i]), tmp_str);
			gtk_misc_set_alignment (GTK_MISC (plot.label_y[i]), 1.0, i / 4.0);
			gtk_widget_show (plot.label_y[i]);
		}
		for (; i < MAX_Y_LABELS; i++)
			gtk_widget_hide (plot.label_y[i]);
	}
	else if (field == 2) /* Speed (WPM) */
	{
		plot.grid = gtk_databox_grid_new (11, 3, &color, 1);
		for (i = 0; i < 13; i++)
		{
			g_sprintf (tmp_str, "%u", 120 - 10*i);
			gtk_label_set_text (GTK_LABEL (plot.label_y[i]), tmp_str);
			gtk_misc_set_alignment (GTK_MISC (plot.label_y[i]), 1.0, i / 12.0);
			gtk_widget_show (plot.label_y[i]);
		}
	}
	else
	{
		plot.grid = gtk_databox_grid_new (9, 3, &color, 1);
		for (i = 0; i < 11; i++)
		{
			if (field == 3)
				g_sprintf (tmp_str, "%u", 100 - 10*i);
			else
				g_sprintf (tmp_str, "%u", 10 - i);
			gtk_label_set_text (GTK_LABEL (plot.label_y[i]), tmp_str);
			gtk_misc_set_alignment (GTK_MISC (plot.label_y[i]), 1.0, i / 10.0);
			gtk_widget_show (plot.label_y[i]);
		}
		for (; i < MAX_Y_LABELS; i++)
			gtk_widget_hide (plot.label_y[i]);
	}
	gtk_databox_graph_add (GTK_DATABOX (plot.databox), plot.grid);

	/* Redraw the plot
	 */
	gtk_widget_show_all (plot.gtkgrid);
}

void
plot_pointer_update (gdouble x)
{
	static glong n_prev = 0;
	glong n = 0;
	gchar *xstr;
	gchar *ystr;
	GtkDatabox *box;
	gint width;

	box = GTK_DATABOX (plot.databox);

	gtk_window_get_size (get_win ("window_stat"), &width, NULL);
	if (plot_type > 4)
		width -= 25;
	else
		width -= 50;

	n = rintf (x / width * (DATA_POINTS + 2)) - 1; // Round integer from float
	if (n == n_prev)
		return;
	n_prev = n;

	if (n < 0 || n >= n_points || n >= DATA_POINTS) 
	{
		xstr = g_strdup ("--");
		ystr = g_strdup ("--");
		plot.mark.x[0] = -7;
		plot.mark.y[0] = -7;
	}
	else
	{
		if (plot_type < 6)
		{
			xstr = g_strdup_printf ("%s - %s", date[n], hour[n]);
			ystr = g_strdup_printf ("%.2f", plot.data.y[n]);
		}
		else if (plot_type == 6)
		{
			xstr = accur_terror_char_utf8 (n);
			ystr = g_strdup_printf ("%.0f/%.lu", plot.data.y[n], accur_error_total ());
		}
		else
		{
			xstr = accur_ttime_char_utf8 (n);
			ystr = g_strdup_printf ("%.3f", plot.data.y[n]);
		}
		plot.mark.x[0] = plot.data.x[n];
		plot.mark.y[0] = plot.data.y[n];
	}
	gtk_entry_set_text (GTK_ENTRY (get_wg ("entry_stat_x")), xstr);
	gtk_entry_set_text (GTK_ENTRY (get_wg ("entry_stat_y")), ystr);
	gtk_widget_queue_draw (plot.databox);
	g_free (xstr);
	g_free (ystr);
}
