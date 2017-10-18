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
 * Fluidness exercise
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "auxiliar.h"
#include "main.h"
#include "callbacks.h"
#include "translation.h"
#include "keyboard.h"
#include "tutor.h"
#include "velocity.h"
#include "fluidness.h"

typedef struct
{
	gchar *buffer;
	gint len;
	gchar name[21];
} Paragraph;

Paragraph par = { NULL, 0, "" };

extern gchar *OTHER_DEFAULT;

/*******************************************************************************
 * Interface functions
 */
gchar *
fluid_get_paragraph_name ()
{
	return par.name;
}

void
fluid_reset_paragraph ()
{
	g_free (par.buffer);
	par.buffer = NULL;
	par.len = 0;
	par.name[0] = '\0';
}

/*
 * Get from the structure 'par' the paragraph defined by 'index'
 *
 */
gchar *
get_par (gint index)
{
	gint i;
	gint size;
	gint stops;
	gchar *par_1;
	gchar *par_2;
	gchar *par_i;

	par_1 = par.buffer;
	par_2 = strchr (par_1, '\n') + 1;
	if (par_2 == NULL)
		par_2 = par_1;
	for (i = 0; i < index && i < par.len; i++)
	{
		par_1 = par_2;
		par_2 = strchr (par_1, '\n') + 1;
		if (par_2 == NULL)
			par_2 = par_1;
	}
	size = par_2 - par_1;
	if (size < 1)
	{
		g_message ("internal error while picking the paragraph %i.", index);
		par_i = g_strdup_printf ("#%i\n", index);
	}
	else
	{
		stops = 0;
		for (i = 1; i < size; i++)
			if (par_1[i] == ' ')
			       if (par_1[i - 1] == '.' || par_1[i - 1] == '!' || par_1[i - 1] == '?')
					stops++;
		par_i = g_malloc (size + stops + 10);
		strncpy (par_i, par_1, size);
		par_i[size] = '\0';
		//g_message ("Paragraph %i: %i stops", index, stops);

		if (main_preferences_get_boolean ("tutor", "double_spaces"))
		{
			for (i = 0; i < size - 1; i++)
			{
				if (par_i[i + 1] == ' ' && par_i[i + 2] != ' ')
					if (par_i[i] == '.' || par_i[i] == '!' || par_i[i] == '?')
					{
						memmove (par_i + i + 3, par_i + i + 2, size - i - 1);
						par_i[i+2] = ' ';
						i += 2;
						size++;
					}
			}
		}
	}

	size = strlen (par_i);
	if (size > 0)
		par_i[size - 1] = '\n';
	return (par_i);
}

/**********************************************************************
 * Initialize the fluid exercise window.
 */
void
fluid_init ()
{
	gchar *tmp_name;
	gchar *tmp_str;
	FILE *fh;
	GtkWidget *wg;

	if (!main_preferences_exist ("tutor", "fluid_paragraphs"))
		main_preferences_set_int ("tutor", "fluid_paragraphs", 3);

	callbacks_shield_set (TRUE);
	wg = get_wg ("spinbutton_lesson");
	gtk_spin_button_set_value (GTK_SPIN_BUTTON (wg),
				   main_preferences_get_int ("tutor", "fluid_paragraphs"));
	callbacks_shield_set (FALSE);

	if (par.len == 0)
	{
		if (main_preferences_exist ("tutor", "paragraph_list"))
		{
			tmp_str = main_preferences_get_string ("tutor", "paragraph_list");
			tmp_name = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, tmp_str, ".paragraphs", NULL);
			if ((fh = (FILE *) g_fopen (tmp_name, "r")))
			{
				fluid_init_paragraph_list (tmp_str);
				fclose (fh);
			}
			g_free (tmp_str);
			g_free (tmp_name);
		}
	}
	if (par.len == 0)
		fluid_init_paragraph_list (NULL);
}

/**********************************************************************
 * Reads paragraphs from the text file.
 */
void
fluid_init_paragraph_list (gchar * list_name)
{
	guint len;
	gchar *memory_ok;
	gchar *tmp_name;
	gchar *tmp_code;
	gchar str_9000[9001];
	FILE *fh;

	if (list_name && !g_str_equal (list_name, OTHER_DEFAULT))
	{
		main_preferences_set_string ("tutor", "paragraph_list", list_name);
		tmp_name = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, list_name, ".paragraphs", NULL);
		g_message ("loading text file: %s.paragraphs", list_name);
		strncpy (par.name, list_name, 20);
		par.name[20] = '\0';
	}
	else
	{
		main_preferences_remove ("tutor", "paragraph_list");
		tmp_code = main_preferences_get_string ("interface", "language");
		tmp_name = g_strconcat (main_path_data (), G_DIR_SEPARATOR_S, tmp_code, ".paragraphs", NULL);
		g_message ("loading text file: %s.paragraphs", tmp_code);
		strcpy (par.name, "Default");
		g_free (tmp_code);
	}

	fh = (FILE *) g_fopen (tmp_name, "r");
	if (fh == NULL && g_str_equal (par.name, "Default"))
		fh = trans_lang_get_similar_file (".paragraphs");

	if (fh)
	{
		g_free (par.buffer);
		par.buffer = g_strdup ("");
		par.len = 0;
		g_print ("Paragraphs:\n0");
		while (fgets (str_9000, 9001, fh))
		{
			len = strlen (str_9000);
			if (len < 2)
				continue;
			memory_ok = g_try_renew (gchar, par.buffer, strlen (par.buffer) + len + 2);
			if (memory_ok)
				par.buffer = memory_ok;
			else
			{
				g_print ("\nThere was truncation: memory restrictions...");
				break;
			}
			strcat (par.buffer, str_9000);
			if (len == 9000)
				strcat (par.buffer, "\n");
			par.len++;
			g_print (" - %i", par.len);
		}
		fclose (fh);
		g_print ("\nText file loaded!\n\n");
	}
	else
	{
		g_message ("could not open the file: %s", tmp_name);
		g_free (tmp_name);
		tmp_code = main_preferences_get_string ("interface", "language");
		if (g_str_equal (tmp_code, "C"))
			g_error ("so, we must quit!");
		main_preferences_set_string ("interface", "language", "C");
		fluid_init_paragraph_list (list_name);
		main_preferences_set_string ("interface", "language", tmp_code);
		g_free (tmp_code);
		return;
	}
	g_free (tmp_name);
}

/**********************************************************************
 * Draw random sentences selected from a '.paragraphs' file
 */
void
fluid_draw_random_paragraphs ()
{
	gint i, j;
	gint par_num;
	gint rand_i[10];
#	define FLUID_PARBUF 50
	static gchar *text[FLUID_PARBUF] = {
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, 
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL, NULL
	};

	par_num = main_preferences_get_int ("tutor", "fluid_paragraphs");

	/* Use all the text, without mangling it
	 */
	if (par_num == 0)
	{
		par_num = par.len > FLUID_PARBUF ? FLUID_PARBUF : par.len;

		for (i = 0; i < par_num; i++)
			g_free (text[i]);

		for (i = 0; i < par_num; i++)
		{
			text[i] = get_par (i);
			tutor_draw_paragraph (text[i]);
		}
		return;
	}

	/* Use some paragraphs, pseudo-randomly
	 */
	for (i = 0; (i < par_num) && (i < par.len); i++)
		g_free (text[i]);

	for (i = 0; (i < par_num) && (i < par.len); i++)
	{
		do
		{
			rand_i[i] = rand () % par.len;
			for (j = 0; j < i; j++)
			{
				if (rand_i[i] == rand_i[j])
					rand_i[i] = par.len;
			}
		}
		while (rand_i[i] == par.len);

		text[i] = get_par (rand_i[i]);
		tutor_draw_paragraph (text[i]);
	}
}

/**********************************************************************
 * Takes text and validate it as UTF-8
 */
gchar *
fluid_filter_utf8 (gchar * text)
{
	gulong i;
	gunichar uch = 0;
	gboolean is_symbol;
	struct INPUT_TEXT
	{
		gchar *pt;
		gulong len;
		guint npar;
	} raw;
	struct KEYBOARD_SYMBOLS
	{
		gunichar set[200];
		guint n;
	} sym;
	struct FILTERED_TEXT
	{
		gchar *txt;
		gulong i;
		gulong len;
	} flt;

	raw.len = strlen (text);

	/* Verify empty input string
	 */
	if (raw.len == 0)
	{
		flt.txt = g_strdup_printf ("%i\n", rand () % 9999);
		return (flt.txt);
	}

	/* Allocate memory space for the result
	 */
	flt.i = 0;
	flt.len = raw.len + 100;
	flt.txt = g_malloc (flt.len);

	/* By-pass BOM
	 */
	raw.pt = text;
	if (g_utf8_get_char_validated (raw.pt, 16) == 0xEFBBBF)
		raw.pt = g_utf8_find_next_char (raw.pt, raw.pt + 16);

	/* Replace Win/MAC-returns
	 */
	for (i = 0; i < raw.len; i++)
		if (text[i] == '\r')
			text[i] = '\n';

	/* Filter
	 */
	sym.n = keyb_get_symbols (sym.set);
	raw.npar = 0;
	while (raw.pt && raw.npar < MAX_PARAGRAPHS)
	{
		if (*raw.pt == '\0')
			break;
		/* Test valid utf8 char
		 */
		if ((uch = g_utf8_get_char_validated (raw.pt, 16)) == (gunichar) -1
		    || uch == (gunichar) -2)
			uch = L' ';

		/* Increase the pointer for the input text
		 */
		raw.pt = g_utf8_find_next_char (raw.pt, raw.pt + 16);

		/* Test reazonable char as valid for fluidness exercise
		 */
		if (!(uch == L' ' || uch == L'\n' || g_unichar_isalnum (uch)))
		{
			is_symbol = FALSE;
			for (i = 0; i < sym.n; i++)
				if (uch == sym.set[i])
				{
					is_symbol = TRUE;
					break;
				}
			if (!is_symbol)
				uch = L' ';
		}

		/* Verify memory space of output buffer
		 */
		if (flt.i < flt.len - 7)
		{
			flt.len += 100;
			flt.txt = g_realloc (flt.txt, flt.len);
		}

		/* Verify new line and form the next UTF-8 char to be appended
		 */
		if (uch == L'\n')
		{
			raw.npar++;
			flt.txt[flt.i++] = '\n';
			flt.txt[flt.i++] = '\n';
			for (; *raw.pt == '\n' || *raw.pt == ' '; raw.pt++);
		}
		else
			flt.i += g_unichar_to_utf8 (uch, &flt.txt[flt.i]);
	}
	if (uch != L'\n')
	{
		raw.npar++;
		flt.txt[flt.i++] = '\n';
		flt.txt[flt.i++] = '\n';
	}
	flt.txt[flt.i++] = '\0';

	return (flt.txt);
}

/**********************************************************************
 * Paste clipboard or dropped text in a file, so that it can be used as
 * a customized exercise.
 */
void
fluid_text_write_to_file (gchar * text_raw)
{
	gchar *pars_path;
	gchar *pars_name;
	gchar *text_filtered;
	FILE *fh_destiny;

	pars_name = g_strdup_printf ("(%s)", _("Pasted_or_dropped"));
	pars_path = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, pars_name, ".paragraphs", NULL);
	assert_user_dir ();
	if (!(fh_destiny = (FILE *) g_fopen (pars_path, "w")))
	{
		gdk_beep ();
		g_warning ("couldn't create the file:\n %s", pars_path);
		g_free (pars_path);
		g_free (pars_name);
		return;
	}
	g_free (pars_path);

	/* Filter the text
	 */
	text_filtered = fluid_filter_utf8 (text_raw);
	fwrite (text_filtered, sizeof (gchar), strlen (text_filtered), fh_destiny);
	fclose (fh_destiny);

	g_free (text_filtered);

	fluid_init_paragraph_list (pars_name);
	g_free (pars_name);
	tutor_set_query (QUERY_INTRO);
	tutor_process_touch ('\0');

	velo_text_write_to_file (text_raw, FALSE);
}

/**********************************************************************
 * Copy the file 'file_name' so that it can be used as a customized
 * exercise.
 */
void
fluid_copy_text_file (gchar * file_name)
{
	gchar *pars_path;
	gchar *pars_name;
	gchar *text_raw;
	gchar *text_filtered;
	FILE *fh_destiny;

	if (!file_name)
	{
		gdk_beep ();
		g_warning ("fluid_copy_text_file(): null file name as argument.");
		return;
	}

	if (!g_file_get_contents (file_name, &text_raw, NULL, NULL))
	{
		gdk_beep ();
		g_warning ("couldn't read the file:\n %s\n", file_name);
		return;
	}

	pars_name = g_strdup (strrchr (file_name, DIRSEP) + 1);
	pars_path = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, pars_name, ".paragraphs", NULL);
	assert_user_dir ();
	if (!(fh_destiny = (FILE *) g_fopen (pars_path, "w")))
	{
		gdk_beep ();
		g_warning ("couldn't create the file:\n %s", pars_path);
		g_free (pars_path);
		g_free (pars_name);
		return;
	}
	g_free (pars_path);

	/* Filter the text
	 */
	text_filtered = fluid_filter_utf8 (text_raw);
	fwrite (text_filtered, sizeof (gchar), strlen (text_filtered), fh_destiny);
	fclose (fh_destiny);

	g_free (text_raw);
	g_free (text_filtered);

	fluid_init_paragraph_list (pars_name);
	g_free (pars_name);
	tutor_set_query (QUERY_INTRO);
	tutor_process_touch ('\0');

	velo_create_dict (file_name, FALSE);
}

/**********************************************************************
 * Put on the screen the final comments
 */
#define FLUID_1 60
void
fluid_comment (gdouble accuracy, gdouble velocity, gdouble fluidness)
{
	gchar *tmp_str;
	GtkWidget *wg;
	GtkTextBuffer *buf;

	/*
	 * Comments
	 */
	if (accuracy < tutor_goal_accuracy ())
		tmp_str = g_strdup (":-(\n");
	else if (velocity < tutor_goal_speed ())
		tmp_str = g_strdup_printf (_(" You type accurately but not so fast.\n"
				   " Can you reach %.0f WPM?\n"), tutor_goal_speed ());
	else if (fluidness < tutor_goal_level (0))
		tmp_str = g_strdup_printf (_(" Your rhythm is not so constant. Calm down.\n"
				      " For now, try to make the fluidness greater than %i%%.\n"), (gint) tutor_goal_level(0));
	else if (fluidness < tutor_goal_fluidity ())
		tmp_str = g_strdup_printf (_(" You are almost getting there. Type more fluently.\n"
				      " I want a fluidness greater than %.0f%%.\n"), tutor_goal_fluidity ());
	else if (velocity < tutor_goal_level (1))
		tmp_str = g_strdup (_(" Congratulations!\n"
				      " It seems to me that you are a professional.\n"
				      " You don't need this program (me) anymore.\n"
				      " Hope you have enjoyed. Thanks and be happy!\n"));
	else
		tmp_str = g_strdup (_(" How can you type so fast?\n"
				      " You have exceeded all my expectations.\n"
				      " Are you a machine? Could you teach me?\n"
				      " I can not help you anymore. Go to an expert!\n"));

	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));
	gtk_text_buffer_insert_at_cursor (buf, tmp_str, strlen (tmp_str));
	g_free (tmp_str);
}
