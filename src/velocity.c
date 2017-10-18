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
 * Velocity exercise
 */
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "auxiliar.h"
#include "main.h"
#include "translation.h"
#include "keyboard.h"
#include "tutor.h"
#include "velocity.h"

GList *word_list;

struct
{
	GList *list;
	gint len;
	gchar *name;
} dict;

extern gchar *OTHER_DEFAULT;

/*******************************************************************************
 * Interface functions
 */
gchar *
velo_get_dict_name ()
{
	return (dict.name);
}

void
velo_reset_dict ()
{
	g_list_free (dict.list);
	dict.list = NULL;
	dict.len = 0;
	g_free (dict.name);
	dict.name = NULL;
}

/**********************************************************************
 * Initialize the velo exercise window.
 */
void
velo_init ()
{
	gchar *wl_name;

	if (dict.len != 0)
		return;

	if (main_preferences_exist ("tutor", "word_list"))
	{
		wl_name = main_preferences_get_string ("tutor", "word_list");
		velo_init_dict (wl_name);
		g_free (wl_name);
	}

	if (dict.len == 0)
		velo_init_dict (NULL);
}

/**********************************************************************
 * Retrieves words from the dictionary.
 */
void
velo_init_dict (gchar * list_name)
{
	static gchar *word_buf = NULL;
	gchar *word;
	gchar *tmp_buf;
	gchar *tmp_name;
	gchar *tmp_code;
	gchar *dict_name;

	if (list_name && !g_str_equal (list_name, OTHER_DEFAULT) )
	{
		main_preferences_set_string ("tutor", "word_list", list_name);
		tmp_name = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, list_name, ".words", NULL);
		g_message ("loading dictionary: %s.words", list_name);
		dict_name = g_strdup (list_name);
	}
	else
	{
		main_preferences_remove ("tutor", "word_list");
		tmp_code = main_preferences_get_string ("interface", "language");
		tmp_name = g_strconcat (main_path_data (), G_DIR_SEPARATOR_S, tmp_code, ".words", NULL);
		g_message ("loading dictionary: %s.words", tmp_code);
		g_free (tmp_code);
		dict_name = g_strdup ("Default");
	}

	if (!g_file_test (tmp_name, G_FILE_TEST_IS_REGULAR))
	{
		g_free (tmp_name);
		tmp_name = trans_lang_get_similar_file_name (".words");
		g_message ("not found, loading from file:\n %s", tmp_name);
	}

	if (g_file_get_contents (tmp_name, &tmp_buf, NULL, NULL))
	{
		velo_reset_dict ();
		dict.name = dict_name;
		g_free (word_buf);
		word_buf = tmp_buf;
		g_print ("Tens of words:\n 0");
		while (*tmp_buf != '\0')
		{
			word = tmp_buf;
			dict.list = g_list_prepend (dict.list, word);
			dict.len++;
			if (dict.len % 10 == 0)
				g_print (" - %u", dict.len / 10);
			while (*tmp_buf != '\n' && *tmp_buf != '\r' && *tmp_buf != '\0')
			       tmp_buf++;
			if (*tmp_buf == '\0')
				break;
			while (*tmp_buf == '\n' || *tmp_buf == '\r')
			{
				*tmp_buf = '\0';
				tmp_buf++;
			}
		}
		g_print ("\n");
		g_message ("Dictionary loaded!\n\n");
	}
	else
	{
		g_free (dict_name);
		g_message ("could not open the file: %s", tmp_name);
		tmp_code = main_preferences_get_string ("interface", "language");
		if (g_str_equal (tmp_code, "C"))
			g_error ("something wrong, we must quit!");
		main_preferences_set_string ("interface", "language", "C");
		velo_init_dict (list_name);
		main_preferences_set_string ("interface", "language", tmp_code);
		g_free (tmp_code);
	}
	g_free (tmp_name);
}

/**********************************************************************
 * Draw random phrases with words selected from a 'discretionary'
 */
void
velo_draw_random_words ()
{
	gint i, j;
	gchar *word;
	struct PARAGRAPH
	{
		gchar *text;
		gsize size;
		gsize i;
	} par;

	par.size = 1024;
	par.text = g_new (gchar, par.size);
	for (i = 0; i < 4; i++)	/* 4 paragraphs per exercise */
	{		
		par.i = 0;
		for (j = 0; j < 20; j++) /* 20 words per paragraph */
		{		
			word = g_strdup (g_list_nth_data (dict.list, rand () % dict.len));
			if (j == 0)
				word[0] = g_ascii_toupper (word[0]);

			if (par.i + strlen (word) + 4 > par.size)
			{
				par.size += 1024;
				par.text = g_renew (gchar, par.text, par.size);
			}

			strcpy (par.text + par.i, word);
			par.i += strlen (word);
			par.text[par.i++] = ' ';
			g_free (word);
		}
		par.i--;
		if (trans_lang_has_stopmark ())
			par.text[par.i++] = '.';
		par.text[par.i++] = '\n';
		par.text[par.i++] = '\0';
		tutor_draw_paragraph (par.text);
	}
	g_free (par.text);
}

/**********************************************************************
 * Takes text and make a list of words, one per line, validating as UTF-8
 */
gchar *
velo_filter_utf8 (gchar * text)
{
	gunichar uch;
	gboolean is_searching_word;
	struct INPUT_TEXT
	{
		gchar *pt;
		gulong len;
		guint nwords;
	} raw;
	struct FILTERED_TEXT
	{
		gchar *txt;
		gulong i;
		gulong len;
	} flt;

	raw.len = strlen (text);

	/* Verify empty string
	 */
	if (raw.len == 0)
	{
		flt.txt = g_strdup ("01234\n56789\n43210\n98765\n:-)\n");
		return (flt.txt);
	}

	/* Allocate memory space for the result
	 */
	flt.i = 0;
	flt.len = raw.len + 1024;
	flt.txt = g_malloc (flt.len);

	raw.pt = text;

	/* Filter
	 */
	raw.nwords = 0;
	is_searching_word = TRUE;
	while (raw.pt && raw.nwords < MAX_WORDS)
	{
		if (*raw.pt == '\0')
			break;

		/* Read valid utf8 char
		 */
		if ((uch = g_utf8_get_char_validated (raw.pt, -1)) == (gunichar) - 1
		    || uch == (gunichar) - 2)
			uch = L' ';

		/* Increase the pointer for the input text
		 */
		raw.pt = g_utf8_find_next_char (raw.pt, NULL);

		/* Verify memory space of output buffer
		 */
		if (flt.i < flt.len - 8)
		{
			flt.len += 1024;
			flt.txt = g_realloc (flt.txt, flt.len);
		}

		/* Test alphabetic char to form a word
		 */
		if (g_unichar_isalpha (uch) || g_unichar_ismark (uch))
		{
			flt.i += g_unichar_to_utf8 (uch, &flt.txt[flt.i]);
			is_searching_word = FALSE;
		}
		else if (!is_searching_word)
		{
			raw.nwords++;
			flt.txt[flt.i++] = '\n';
			is_searching_word = TRUE;
		}
	}
	flt.txt[flt.i++] = '\0';

	if (raw.nwords == 0)
	{
		g_free (flt.txt);
		flt.txt = g_strdup ("01234\n56789\n43210\n98765\n:-)\n");
	}

	return (flt.txt);
}

/**********************************************************************
 * Reads the text "text_raw" and write to the dictionary.
 * If overwrite is TRUE, then besides overwriting any .words file,
 * it loads a lesson with the new dictionary.
 */
void
velo_text_write_to_file (gchar * text_raw, gboolean overwrite)
{
	gchar *dict_path;
	gchar *dictio_name;
	gchar *text_filtered;
	FILE *fh_destiny;

	dictio_name = g_strdup_printf ("(%s)", _("Pasted_or_dropped"));
	dict_path = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, dictio_name, ".words", NULL);
	assert_user_dir ();
	if (!(fh_destiny = (FILE *) g_fopen (dict_path, "w")))
	{
		gdk_beep ();
		g_warning ("couldn't create the file:\n <%s>", dict_path);
		if (overwrite == FALSE)
		{
			g_free (dict_path);
			g_free (dictio_name);
			return;
		}
	}
	g_free (dict_path);

	/* Filter the text
	 */
	text_filtered = velo_filter_utf8 (text_raw);
	fwrite (text_filtered, sizeof (gchar), strlen (text_filtered), fh_destiny);
	fclose (fh_destiny);

	g_free (text_filtered);

	if (overwrite == TRUE)
	{
		velo_init_dict (dictio_name);
		tutor_set_query (QUERY_INTRO);
		tutor_process_touch ('\0');
	}
	g_free (dictio_name);
}

/**********************************************************************
 * Reads the text file "file_name" and write to the dictionary.
 * If overwrite is TRUE, then besides overwriting any .words file,
 * it loads a lesson with the new dictionary.
 */
void
velo_create_dict (gchar * file_name, gboolean overwrite)
{
	gchar *dict_path;
	gchar *dictio_name;
	gchar *text_raw;
	gchar *text_filtered;
	FILE *fh_destiny;

	if (!file_name)
	{
		gdk_beep ();
		g_warning ("velo_create_dict(): null file name as first argument.");
		return;
	}

	if (!g_file_get_contents (file_name, &text_raw, NULL, NULL))
	{
		gdk_beep ();
		g_warning ("couldn't read the file:\n <%s>", file_name);
		return;
	}

	dictio_name = g_path_get_basename (file_name);
	dict_path = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, dictio_name, ".words", NULL);
	assert_user_dir ();
	if (!(fh_destiny = (FILE *) g_fopen (dict_path, "w")))
	{
		gdk_beep ();
		g_warning ("couldn't create the file:\n <%s>", dict_path);
		if (overwrite == FALSE)
		{
			g_free (dict_path);
			g_free (dictio_name);
			return;
		}
	}
	g_free (dict_path);

	/* Filter the text
	 */
	text_filtered = velo_filter_utf8 (text_raw);
	fwrite (text_filtered, sizeof (gchar), strlen (text_filtered), fh_destiny);
	fclose (fh_destiny);

	g_free (text_raw);
	g_free (text_filtered);

	if (overwrite == TRUE)
	{
		velo_init_dict (dictio_name);
		tutor_set_query (QUERY_INTRO);
		tutor_process_touch ('\0');
	}
	g_free (dictio_name);
}

/**********************************************************************
 * Put on the screen the final comments
 */
void
velo_comment (gdouble accuracy, gdouble velocity)
{
	gchar *tmp_str;
	GtkWidget *wg;
	GtkTextBuffer *buf;

	/*
	 * Comments
	 */
	if (accuracy < tutor_goal_accuracy ())
		tmp_str = g_strdup (":-(\n");
	else if (velocity < tutor_goal_level(0))
		tmp_str = g_strdup (_(" You are just beginning.\n"
				" Be patient, try it again every day, rest and don't worry so much:\n"
				" persistence and practice will improve your velocity.\n"));
	else if (velocity < tutor_goal_level(1))
		tmp_str = g_strdup_printf (_(" Still away from the highway. You can do better...\n"
					" Try to reach at least %.0f WPM.\n"), tutor_goal_level(1));
	else if (velocity < tutor_goal_level(2))
		tmp_str = g_strdup_printf (_(" You are doing well, but need to go faster.\n"
				      " And don't forget the accuracy. Try to get %.0f WPM.\n"), tutor_goal_level(2));
	else if (velocity < tutor_goal_level(3))
		tmp_str = g_strdup_printf (_(" Fine. Now you need to start running.\n"
						" Can you reach %.0f WPM?\n"), tutor_goal_level(3));
	else if (velocity < tutor_goal_speed ())
		tmp_str = g_strdup_printf (_(" Very good. You are almost there.\n "
					"Can you finally reach %.0f WPM?\n"), tutor_goal_speed ());
	else if (velocity < tutor_goal_level(4))
		tmp_str = g_strdup (_(" Excellent. For this course, that is enough.\n"
					" Try now the fluidness exercises, OK?\n"));
	else if (velocity < tutor_goal_level(5))
		tmp_str = g_strdup_printf (_(" Fast! Are you a professional?\n"
						" So, try to get %.0f WPM!\n"), tutor_goal_level(5));
	else if (velocity < tutor_goal_level(6))
		/* Translators: Speed Racer is a reference to a Japanese anime franchise
		   about automobile racing, also known as Mach GoGoGo. */
		tmp_str = g_strdup_printf (_(" Ranking good, Speed Racer!"
				      " Are you afraid of reaching %.0f WPM?\n"), tutor_goal_level(6));
	else if (velocity < tutor_goal_level(7))
		/* Translators: feel free to change the reference to that martial art, if you find another good analogy. */
		tmp_str = g_strdup_printf (_(" Kung-fu mastery!\n Can you fly at %.0f WPM?\n"), tutor_goal_level(7));
	else
		/* Translators: Dvorak here means that infamous ergonomic keyboard layout. */
		tmp_str = g_strdup (_(" Dvorak master!\n"
				   " I have no words to express my admiration!\n"));

	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));
	gtk_text_buffer_insert_at_cursor (buf, tmp_str, strlen (tmp_str));
	g_free (tmp_str);
}
