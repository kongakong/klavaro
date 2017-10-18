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
 * Some auxiliar functions for cursor operations on a GtkTextView widget
 */
#include "gtk/gtk.h"

#include "auxiliar.h"
#include "tutor.h"
#include "cursor.h"

struct
{
	gint id;
	gboolean blink;
} cursor;

/*******************************************************************************
 * Interface functions
 */
gboolean
cursor_get_blink ()
{
	return (cursor.blink);
}

void
cursor_set_blink (gboolean status)
{
	cursor.blink = status;
}

/*******************************************************************************
 * Advance the cursor n positions on the tutor text view
 */
gint
cursor_advance (gint n)
{
	gint i;
	gboolean cursor_out_screen;
	GtkWidget *wg;
	GtkTextBuffer *buf;
	GtkTextIter new_start;
	GtkTextIter old_start;
	GtkTextIter end;
	GtkTextMark *mark;

	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));

	/* Get current position for the iter old_start */
	gtk_text_buffer_get_iter_at_mark (buf, &new_start, gtk_text_buffer_get_insert (buf));
	old_start = new_start;

	/* Get new position for the iter new_start */
	if (n > 0)
		for (i = 0; i < n && gtk_text_iter_forward_char (&new_start); i++);
	else
		for (i = 0; i > n && gtk_text_iter_backward_char (&new_start); i--);

	/* Move cursor blinking */
	end = old_start;
	gtk_text_iter_forward_char (&end);
	gtk_text_buffer_remove_tag_by_name (buf, "cursor_blink", &old_start, &end);
	gtk_text_buffer_remove_tag_by_name (buf, "cursor_unblink", &old_start, &end);

	end = new_start;
	gtk_text_iter_forward_char (&end);
	gtk_text_buffer_apply_tag_by_name (buf, "cursor_blink", &new_start, &end);

	/* Move cursor */
	gtk_text_buffer_place_cursor (buf, &new_start);

	/* Check need for auto-scrolling */
	if (i == n)
	{
		end = new_start;
		gtk_text_iter_forward_line (&end);
		mark = gtk_text_buffer_create_mark (buf, "aux", &end, FALSE);
		cursor_out_screen = gtk_text_view_move_mark_onscreen (GTK_TEXT_VIEW (wg), mark);
		if (cursor_out_screen)
		{
			gtk_text_view_scroll_to_mark (GTK_TEXT_VIEW (wg),
				       	gtk_text_buffer_get_insert (buf), 0.0, TRUE, 0.5, 0.12);
		}
	}
	return (i);
}


/*******************************************************************************
 * Paint the current char (at cursor) with color defined by a tag name
 */
void
cursor_paint_char (gchar * color_tag_name)
{
	GtkWidget *wg;
	GtkTextBuffer *buf;
	GtkTextIter start;
	GtkTextIter end;

	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));
	gtk_text_buffer_get_iter_at_mark (buf, &start, gtk_text_buffer_get_insert (buf));
	end = start;
	gtk_text_iter_forward_char (&end);

	gtk_text_buffer_apply_tag_by_name (buf, color_tag_name, &start, &end);
}


/*******************************************************************************
 * Get the character under the cursor
 */
gunichar
cursor_get_char ()
{
	gunichar chr;
	gchar *tmp_str;
	GtkWidget *wg;
	GtkTextBuffer *buf;
	GtkTextIter start;
	GtkTextIter end;

	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));
	gtk_text_buffer_get_iter_at_mark (buf, &start, gtk_text_buffer_get_insert (buf));
	gtk_text_buffer_get_iter_at_mark (buf, &end, gtk_text_buffer_get_insert (buf));
	gtk_text_iter_forward_char (&end);
	tmp_str = gtk_text_buffer_get_text (buf, &start, &end, FALSE);
	chr = g_utf8_get_char (tmp_str);
	g_free (tmp_str);
	return (chr);
}

/*******************************************************************************
 * Initialize the cursor so that it blinks
 */
gboolean
cursor_init (gpointer data)
{
	cursor.id = 0;
	cursor.blink = TRUE;
	cursor_on (NULL);
	return FALSE;
}

/**********************************************************************
 * Turns on the cursor and makes it blink if 'cursor_blink' is TRUE
 */
gboolean
cursor_on (gpointer data)
{
	if (cursor.id)
	{
		g_source_remove (cursor.id);
		cursor.id = 0;
	}

	if (cursor.blink == FALSE)
		return FALSE;

	if (! gtk_widget_get_visible (get_wg ("window_tutor")))
	{
		cursor.blink = FALSE;
		return FALSE;
	}

	cursor.id = g_timeout_add (TIME_CURSOR_ON, &cursor_off, NULL);

	cursor_switch_on ();

	return FALSE;
}

/**********************************************************************
 * Turns off the cursor and makes it blink if 'cursor_blink' is TRUE
 */
gboolean
cursor_off (gpointer data)
{
	if (cursor.id)
	{
		g_source_remove (cursor.id);
		cursor.id = 0;
	}

	if (cursor.blink == FALSE)
		return FALSE;

	if (! gtk_widget_get_visible (get_wg ("window_tutor")))
	{
		cursor.blink = FALSE;
		return FALSE;
	}

	cursor.id = g_timeout_add (TIME_CURSOR_OFF, &cursor_on, NULL);

	cursor_switch_off ();

	return FALSE;
}

/**********************************************************************
 * Turns on the cursor immediately
 */
void
cursor_switch_on ()
{
	GtkTextView *wg_text;
	GtkTextBuffer *buf;
	GtkTextIter start;
	GtkTextIter end;

	wg_text = GTK_TEXT_VIEW (get_wg ("text_tutor"));
	buf = gtk_text_view_get_buffer (wg_text);

	gtk_text_buffer_get_iter_at_mark (buf, &start, gtk_text_buffer_get_insert (buf));
	gtk_text_buffer_get_iter_at_mark (buf, &end, gtk_text_buffer_get_insert (buf));
	gtk_text_iter_forward_char (&end);
	if (tutor_get_correcting ())
	{
		gtk_text_buffer_remove_tag_by_name (buf, "cursor_blink", &start, &end);
		gtk_text_buffer_apply_tag_by_name (buf, "cursor_unblink", &start, &end);
	}
	else
	{
		gtk_text_buffer_remove_tag_by_name (buf, "cursor_unblink", &start, &end);
		gtk_text_buffer_apply_tag_by_name (buf, "cursor_blink", &start, &end);
	}
}

/**********************************************************************
 * Turns off the cursor immediately
 */
void
cursor_switch_off ()
{
	GtkTextView *wg_text;
	GtkTextBuffer *buf;
	GtkTextIter start;
	GtkTextIter end;

	wg_text = GTK_TEXT_VIEW (get_wg ("text_tutor"));
	buf = gtk_text_view_get_buffer (wg_text);

	gtk_text_buffer_get_iter_at_mark (buf, &start, gtk_text_buffer_get_insert (buf));
	gtk_text_buffer_get_iter_at_mark (buf, &end, gtk_text_buffer_get_insert (buf));
	gtk_text_iter_forward_char (&end);
	if (tutor_get_correcting ())
	{
		gtk_text_buffer_remove_tag_by_name (buf, "cursor_unblink", &start, &end);
		gtk_text_buffer_apply_tag_by_name (buf, "cursor_blink", &start, &end);
	}
	else
	{
		gtk_text_buffer_remove_tag_by_name (buf, "cursor_blink", &start, &end);
		gtk_text_buffer_apply_tag_by_name (buf, "cursor_unblink", &start, &end);
	}
}
