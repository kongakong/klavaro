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
 * Contest for fluidness performance
 */

#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <time.h>
#include <math.h>
#include <sys/stat.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <curl/curl.h>
#include <curl/easy.h>

#include "auxiliar.h"
#include "main.h"
#include "translation.h"
#include "top10.h"

/**************************************************
 * Variables
 */
Statistics top10_local[10];
Statistics top10_global[10];
GKeyFile *keyfile = NULL;

/**************************************************
 * Functions
 */

void
top10_init ()
{
	gint i;
	gchar *str;
	GtkListStore *list;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;
	GtkTreeView *tv;
	GtkTreeIter iter;
	GtkComboBox *cmb;

	renderer = gtk_cell_renderer_text_new ();

	/* Main info on treeview
	 */
	tv = GTK_TREE_VIEW (get_wg ("treeview_top10_1"));
	list = gtk_list_store_new (3, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (tv, GTK_TREE_MODEL (list));

	column = gtk_tree_view_column_new_with_attributes ("#", renderer, "text", 0, NULL);
	gtk_tree_view_append_column (tv, column);
	column = gtk_tree_view_column_new_with_attributes (_("Name"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column (tv, column);
	column = gtk_tree_view_column_new_with_attributes (_("Score"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column (tv, column);

	for (i = 0; i < 10; i++)
	{
		str = g_strdup_printf ("%02i", i + 1);
		gtk_list_store_append (list, &iter);
		gtk_list_store_set (list, &iter, 0, str, 1, "--------------------------", 2, "0.000", -1);
		g_free (str);
	}

	/* Extra info on treeview
	 */
	tv = GTK_TREE_VIEW (get_wg ("treeview_top10_2"));
	list = gtk_list_store_new (5, G_TYPE_STRING, G_TYPE_STRING, G_TYPE_STRING,
		       		      G_TYPE_STRING, G_TYPE_STRING);
	gtk_tree_view_set_model (tv, GTK_TREE_MODEL (list));

	column = gtk_tree_view_column_new_with_attributes (_("Accuracy"), renderer, "text", 0, NULL);
	gtk_tree_view_append_column (tv, column);
	column = gtk_tree_view_column_new_with_attributes (_("Speed"), renderer, "text", 1, NULL);
	gtk_tree_view_append_column (tv, column);
	column = gtk_tree_view_column_new_with_attributes (_("Fluidity"), renderer, "text", 2, NULL);
	gtk_tree_view_append_column (tv, column);
	column = gtk_tree_view_column_new_with_attributes (_("Chars"), renderer, "text", 3, NULL);
	gtk_tree_view_append_column (tv, column);
	column = gtk_tree_view_column_new_with_attributes (_("When"), renderer, "text", 4, NULL);
	gtk_tree_view_append_column (tv, column);

	for (i = 0; i < 10; i++)
	{
		gtk_list_store_append (list, &iter);
		gtk_list_store_set (list, &iter, 0, "--", 1, "--", 2, "--", 3, "--", 4, "--", -1);
	}

	/* Set modules combo
	 */
	cmb = GTK_COMBO_BOX (get_wg ("combobox_stat_module"));
	for (i = 0; i < 4; i++)
		gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (cmb), 0);
	str = g_strdup_printf ("1 - %s", _("Basic course"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), str);
	g_free (str);
	str = g_strdup_printf ("2 - %s", _("Adaptability"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), str);
	g_free (str);
	str = g_strdup_printf ("3 - %s", _("Speed"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), str);
	g_free (str);
	str = g_strdup_printf ("4 - %s", _("Fluidity"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), str);
	g_free (str);
	// Fix missing translation bug
	cmb = GTK_COMBO_BOX (get_wg ("combobox_top10"));
	gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (cmb), 0);
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), _("Local scores"));
	gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), _("External scores"));
}

void
top10_message (gchar * msg)
{
	static gboolean init = TRUE;

	if (!init)
		gtk_statusbar_pop (GTK_STATUSBAR (get_wg ("statusbar_top10_message")), 0);
	if (msg == NULL)
	{
		gtk_statusbar_push (GTK_STATUSBAR (get_wg ("statusbar_top10_message")), 0, "");
		return;
	}
	gtk_statusbar_push (GTK_STATUSBAR (get_wg ("statusbar_top10_message")), 0, msg);
}

#define NOBODY "xxx"
void
top10_clean_stat (gint i, gboolean locally)
{
	Statistics *top10;

	top10 = locally ? top10_local : top10_global;

	if (i > 9)
		return;

	top10[i].lang[0] = 'x';
	top10[i].lang[1] = 'x';
	top10[i].genv = 'x';
	top10[i].when = 0;
	top10[i].nchars = MIN_CHARS_TO_LOG;
	top10[i].accur = 0.0;
	top10[i].velo = 0.0;
	top10[i].fluid = 0.0;
	top10[i].score = 0.0;
	top10[i].name_len = strlen (NOBODY);
	strcpy (top10[i].name, NOBODY);
}

void
top10_init_stats (gboolean locally)
{
	gint i;

	for (i = 0; i < 10; i++)
		top10_clean_stat (i, locally);
}

void
top10_insert_stat (Statistics * stat, gint i, gboolean locally)
{
	gint j;
	Statistics *top10;

	top10 = locally ? top10_local : top10_global;

	if (i > 9)
		return;

	for (j = 8; j >= i; j--)
		memmove (&top10[j + 1], &top10[j], sizeof (Statistics));

	memmove (&top10[i], stat, sizeof (Statistics));
}

gboolean
top10_compare_insert_stat (Statistics * stat, gboolean locally)
{
	gint i, j;
	gint statnamelen;
	gchar *pos;
	Statistics *top10;

	top10 = locally ? top10_local : top10_global;

	statnamelen = strlen (stat->name); 
	pos = strrchr (stat->name, '[');
	if (pos != NULL)
		if (pos > stat->name && *(pos - 1) == ' ')
		{
			statnamelen = pos - stat->name - 1; 
			if (statnamelen < 1)
				statnamelen = strlen (stat->name); 
		}
	for (i = 0; i < 10; i++)
	{
		if (stat->score > top10[i].score)
		{
			for (j = i - 1; j >= 0; j--)
			{
				if (strncmp (stat->name, top10[j].name, statnamelen) == 0)
					return (FALSE);
			}

			for (j = i; j < 10; j++)
			{
				if (strncmp (stat->name, top10[j].name, statnamelen) == 0 &&
							stat->score > 0)
				{
					top10_delete_stat (j, locally);
					j--;
				}
			}
			top10_insert_stat (stat, i, locally);
			return (TRUE);
		}
	}
	return (FALSE);
}

void
top10_delete_stat (gint i, gboolean locally)
{
	gint j;
	Statistics *top10;

	top10 = locally ? top10_local : top10_global;

	if (i > 9)
		return;

	for (j = i; j < 9; j++)
		memmove (&top10[j], &top10[j + 1], sizeof (Statistics));

	top10_clean_stat (9, locally);
}

gfloat
top10_calc_score_old (Statistics * stat)
{
	gfloat score;
	// Don't touch this anymore!
	score = (6 * stat->velo + 3 * stat->accur + stat->fluid) / 100;
	return (score);
}

gfloat
top10_calc_score (Statistics * stat)
{
	gfloat score;
	// Don't touch this anymore! (touched: new_WPM = 1.2 old_WPM)
	score = (6 * stat->velo / 1.2 + 3 * stat->accur + stat->fluid) / 100;
	return (score);
}

gboolean
top10_validate_stat (Statistics * stat)
{

	if (stat->score > 10.0 || stat->score < 0.0)
	{
		g_message ("Invalid score (%g): it must be between 0 and 10", stat->score);
		return (FALSE);
	}

	if ((stat->score - top10_calc_score (stat) > 1.0e-5) && (stat->score - top10_calc_score_old (stat) > 1.0e-5))
	{
		g_message ("Invalid score (%g): did you try to edit the scores file?", stat->score);
		return (FALSE);
	}

	if (stat->fluid > 95)
	{
		g_message
			("Invalid score (fluidness = %g): robots shouldn't compete with humans...",
			 stat->fluid);
		return (FALSE);
	}

	return (TRUE);
}

gchar *
top10_get_score_file (gboolean locally, gint lang)
{
	gchar *tmp = NULL;
	gchar *ksc;

	if (lang < 0)
		tmp = main_preferences_get_string ("interface", "language");
	else
		tmp = g_strdup (trans_get_code (lang));
	if (tmp == NULL)
		tmp = g_strdup ("en");

	if (tmp[0] == 'C')
		ksc = g_strdup (locally ? "local_en.ksc" : "global_en.ksc");
	else
		ksc = g_strdup_printf ("%s_%c%c.ksc", locally ? "local" : "global", tmp[0], tmp[1]);
	g_free (tmp);

	return (ksc);
}

static void
top10_merge_stats_from_file (gchar * file)
{
	gint i, j;
	gint32 n;
	FILE *fh;
	Statistics top10;

	fh = g_fopen (file, "r");
	if (fh == NULL)
		return;

	for (i = 0; i < 10; i++)
	{
		/* lang[0] */
		top10.lang[0] = getc (fh);
		if (!g_ascii_isalpha (top10.lang[0]))
		{
			g_message ("Problem: lang[0] = %c", top10.lang[0]);
			break;
		}

		/* lang[1] */
		top10.lang[1] = getc (fh);
		if (!g_ascii_isalpha (top10.lang[1]))
		{
			top10.lang[0] = 'e';
			top10.lang[1] = 'o';
		}

		/* genv */
		top10.genv = getc (fh);
		if (top10.genv != 'x' && top10.genv != 'w')
		{
			g_message ("Problem: genv = %c", top10.genv);
			break;
		}

		/* when */
		//n = fread (&top10.when, sizeof (time_t), 1, fh);
		n = fread (&top10.when, sizeof (gint32), 1, fh);
		if (n == 0)
		{
			g_message ("Problem: when = %li", top10.when);
			break;
		}

		/* nchars */
		n = fread (&top10.nchars, sizeof (gint32), 1, fh);
		if (n == 0 || top10.nchars < MIN_CHARS_TO_LOG)
		{
			g_message ("Problem: nchars = %i, < %i", top10.nchars, MIN_CHARS_TO_LOG);
			break;
		}

		/* accur */
		n = fread (&top10.accur, sizeof (gfloat), 1, fh);
		if (n == 0 || top10.accur < 0 || top10.accur > 100)
		{
			g_message ("Problem: accur = %f <> [0, 100]", top10.accur);
			break;
		}

		/* velo */
		n = fread (&top10.velo, sizeof (gfloat), 1, fh);
		if (n == 0 || top10.velo < 0 || top10.velo > 300)
		{
			g_message ("Problem: velo = %f <> [0, 300]", top10.velo);
			break;
		}

		/* fluid */
		n = fread (&top10.fluid, sizeof (gfloat), 1, fh);
		if (n == 0 || top10.fluid < 0 || top10.fluid > 100)
		{
			g_message ("Problem: fluid = %f <> [0, 100]", top10.fluid);
			break;
		}

		/* score */
		n = fread (&top10.score, sizeof (gfloat), 1, fh);
		if (n == 0 || top10.score < 0 || top10.score > 20)
		{
			g_message ("Problem: score = %f <> [0, 20]", top10.score);
			break;
		}

		/* name_len */
		n = fread (&top10.name_len, sizeof (gint32), 1, fh);
		if (n == 0 || top10.name_len < 0 || top10.name_len > MAX_NAME_LEN)
		{
			g_message ("Problem: name_len = %i <> [0, MAX_NAME_LEN]", top10.name_len);
			break;
		}

		/* name */
		n = fread (&top10.name, sizeof (gchar), top10.name_len, fh);
		top10.name[top10.name_len] = '\0';
		if (!g_utf8_validate (top10.name, -1, NULL))
		{
			for (j = 0; j < top10.name_len; j++)
				if (!g_ascii_isalpha (top10.name[j]))
					top10.name[j] = '?';
		}

		top10_compare_insert_stat (&top10, TRUE);
	}
	fclose (fh);
}

gboolean
top10_read_stats_from_file (gboolean locally, gchar * file)
{
	gint i, j;
	gint32 n;
	gboolean success;
	FILE *fh;
	Statistics *top10;

	top10 = locally ? top10_local : top10_global;

	fh = g_fopen (file, "r");
	if (fh == NULL)
		return FALSE;

	for (i = 0; i < 10; i++)
	{
		success = FALSE;

		/* lang[0] */
		top10[i].lang[0] = fgetc (fh);
		if (!g_ascii_isalpha (top10[i].lang[0]))
		{
			g_message ("Problem: lang[0] = %c", top10[i].lang[0]);
			break;
		}

		/* lang[1] */
		top10[i].lang[1] = fgetc (fh);
		if (!g_ascii_isalpha (top10[i].lang[1]))
		{
			top10[i].lang[0] = 'e';
			top10[i].lang[1] = 'o';
		}

		/* genv */
		top10[i].genv = fgetc (fh);
		if (top10[i].genv != 'x' && top10[i].genv != 'w')
		{
			g_message ("Problem: genv = %c", top10[i].genv);
			break;
		}

		/* when */
		//n = fread (&top10[i].when, sizeof (time_t), 1, fh);
		n = fread (&top10[i].when, sizeof (gint32), 1, fh);
		if (n == 0)
		{
			g_message ("Problem: when = %li", top10[i].when);
			break;
		}

		/* nchars */
		n = fread (&top10[i].nchars, sizeof (gint32), 1, fh);
		if (n == 0 || top10[i].nchars < MIN_CHARS_TO_LOG)
		{
			g_message ("Problem: nchars = %i, < %i", top10[i].nchars, MIN_CHARS_TO_LOG);
			break;
		}

		/* accur */
		n = fread (&top10[i].accur, sizeof (gfloat), 1, fh);
		if (n == 0 || top10[i].accur < 0 || top10[i].accur > 100)
		{
			g_message ("Problem: accur = %f <> [0, 100]", top10[i].accur);
			break;
		}

		/* velo */
		n = fread (&top10[i].velo, sizeof (gfloat), 1, fh);
		if (n == 0 || top10[i].velo < 0 || top10[i].velo > 300)
		{
			g_message ("Problem: velo = %f <> [0, 300]", top10[i].velo);
			break;
		}

		/* fluid */
		n = fread (&top10[i].fluid, sizeof (gfloat), 1, fh);
		if (n == 0 || top10[i].fluid < 0 || top10[i].fluid > 100)
		{
			g_message ("Problem: fluid = %f <> [0, 100]", top10[i].fluid);
			break;
		}

		/* score */
		n = fread (&top10[i].score, sizeof (gfloat), 1, fh);
		if (n == 0 || top10[i].score < 0 || top10[i].score > 20)
		{
			g_message ("Problem: score = %f <> [0, 20]", top10[i].score);
			break;
		}

		/* name_len */
		n = fread (&top10[i].name_len, sizeof (gint32), 1, fh);
		if (n == 0 || top10[i].name_len < 0 || top10[i].name_len > MAX_NAME_LEN)
		{
			g_message ("Problem: name_len = %i <> [0, MAX_NAME_LEN]", top10[i].name_len);
			break;
		}

		/* name */
		n = fread (&top10[i].name, sizeof (gchar), top10[i].name_len, fh);
		top10[i].name[top10[i].name_len] = '\0';
		if (!g_utf8_validate (top10[i].name, -1, NULL))
		{
			for (j = 0; j < top10[i].name_len; j++)
				if (!g_ascii_isalpha (top10[i].name[j]))
					top10[i].name[j] = '?';
		}
		success = TRUE;
	}
	fclose (fh);

	return (success);
}

void
top10_read_stats (gboolean locally, gint lang)
{
	gchar *local_ksc;
	gchar *tmp;
	gchar *uname;
	gchar *stat_dir;
	gboolean success;
	GDir *home;

	top10_init_stats (locally);

	local_ksc = top10_get_score_file (locally, lang);

	tmp = g_build_filename (main_path_score (), local_ksc, NULL);
	if (!top10_read_stats_from_file (locally, tmp))
	{
		g_message ("Could not read the scores file '%s'.\n Creating a blank one.", tmp);
		top10_init_stats (locally);
		top10_write_stats (locally, lang);
		//top10_show_stat (locally ? top10_local : top10_global);
	}
	g_free (tmp);

	if (!locally)
	{
		g_free (local_ksc);
		return;
	}

	/* Go search other users' subdirs
	 */
	tmp = strchr (main_path_stats (), G_DIR_SEPARATOR);
	if (tmp)
	       	tmp = strchr (tmp + 1, G_DIR_SEPARATOR);
	if (tmp)
	       	tmp = strchr (tmp + 1, G_DIR_SEPARATOR);
	if (tmp)
	{
	       	stat_dir = tmp + 1;
		home = g_dir_open ("/home", 0, NULL);
	}
	else
	{
		stat_dir = NULL;
	       	home = NULL;
	}

	success = FALSE;
	if (home)
	{
		while ((uname = (gchar*) g_dir_read_name (home)))
		{
			if (g_str_equal (uname, "root"))
				continue;
			if (g_str_equal (uname, "lost+found"))
				continue;
			if (g_str_equal (uname, g_get_user_name ()))
				continue;

			tmp = g_build_filename ("/home", uname, stat_dir, "ksc", local_ksc, NULL);
			if (g_file_test (tmp, G_FILE_TEST_IS_REGULAR))
			{
				top10_merge_stats_from_file (tmp);
				success = TRUE;
			}
			g_free (tmp);
		}
		g_dir_close (home);
	}
	g_free (local_ksc);

	if (success)
		top10_write_stats (TRUE, -1);
}

void
top10_write_stats (gboolean locally, gint lang)
{
	gint i;
	gchar *filename;
	gchar *lsfile;
	FILE *fh;
	Statistics *top10;

	top10 = locally ? top10_local : top10_global;

	if (!g_file_test (main_path_score (), G_FILE_TEST_IS_DIR))
		g_mkdir_with_parents (main_path_score (), DIR_PERM);

	filename = top10_get_score_file (locally, lang);

	lsfile = g_build_filename (main_path_score (), filename, NULL);

	fh = g_fopen (lsfile, "w");
	if (fh == NULL)
	{
		g_warning ("Could not write the scores file in %s", main_path_score ());
		g_free (filename);
		g_free (lsfile);
		return;
	}

	for (i = 0; i < 10; i++)
	{
		fputc (top10[i].lang[0], fh);
		fputc (top10[i].lang[1], fh);
		fputc (top10[i].genv, fh);
		//fwrite (&top10[i].when, sizeof (time_t), 1, fh);
		fwrite (&top10[i].when, sizeof (gint32), 1, fh);
		fwrite (&top10[i].nchars, sizeof (gint32), 1, fh);
		fwrite (&top10[i].accur, sizeof (gfloat), 1, fh);
		fwrite (&top10[i].velo, sizeof (gfloat), 1, fh);
		fwrite (&top10[i].fluid, sizeof (gfloat), 1, fh);
		fwrite (&top10[i].score, sizeof (gfloat), 1, fh);
		fwrite (&top10[i].name_len, sizeof (gint32), 1, fh);
		if (top10[i].name && top10[i].name_len > 0)
			fputs (top10[i].name, fh);
	}
	fputs ("KLAVARO!", fh);
	fclose (fh);

	g_free (filename);
	g_free (lsfile);
}

/* Test function to show every field of a scoring record, at the terminal
 */
void
top10_show_stat (Statistics * stat)
{
	g_print ("Language: %c%c\n", stat->lang[0], stat->lang[1]);
	g_print ("Graphical environment: %s\n", stat->genv == 'x' ? "Linux" : "Windows");
	g_print ("When: %li\n", stat->when);
	g_print ("# of characters: %i\n", stat->nchars);
	g_print ("Accuracy: %2.1f\n", stat->accur);
	g_print ("Velocity: %2.1f\n", stat->velo);
	g_print ("Fluidness: %2.1f\n", stat->fluid);
	g_print ("Score: %05.1f\n", stat->score);
	g_print ("Name length: %i\n", stat->name_len);
	g_print ("Name: %s\n", stat->name);
}

void
top10_show_stats (gboolean locally)
{
	gint i;
	gboolean success;
	gchar *tmp;
	gchar *url;
	gchar *accur;
	gchar *velo;
	gchar *fluid;
	gchar *nchars;
	gchar *date;
	struct tm *ltime;
	Statistics *top10;
	GtkListStore *list1;
	GtkListStore *list2;
	GtkTreeIter iter1;
	GtkTreeIter iter2;

	top10 = locally ? top10_local : top10_global;

	top10_read_stats (locally, -1);
	top10_read_stats (!locally, -1);

	/* Set layout
	 */
	if (top10[0].score == 0.0)
	{
		top10_message (_("Empty ranking. Please practice fluidness."));
		gtk_widget_set_sensitive (get_wg ("treeview_top10_1"), FALSE);
		gtk_widget_set_sensitive (get_wg ("treeview_top10_2"), FALSE);
	}
	else
	{
		gtk_widget_set_sensitive (get_wg ("treeview_top10_1"), TRUE);
		gtk_widget_set_sensitive (get_wg ("treeview_top10_2"), TRUE);
	}

	gtk_widget_set_sensitive (get_wg ("button_top10_update"), !locally);
	if (top10_local[0].score == 0.0)
		gtk_widget_set_sensitive (get_wg ("button_top10_publish"), FALSE);
	else
		gtk_widget_set_sensitive (get_wg ("button_top10_publish"), !locally);

	/* Set web link
	 */
	tmp = main_preferences_get_string ("interface", "language");
	if (tmp[0] == 'C')
	{
		g_free (tmp);
		tmp = g_strdup ("en");
	}
	url = g_strdup_printf ("http://" DOWNHOST "/%c%c/", tmp[0], tmp[1]);
	gtk_widget_set_tooltip_text (get_wg ("button_top10_go_www"), url);
	g_free (tmp);
	g_free (url);

	/* Print the ranking
	 */
	list1 = GTK_LIST_STORE ( gtk_tree_view_get_model (GTK_TREE_VIEW (get_wg ("treeview_top10_1"))) );
	list2 = GTK_LIST_STORE ( gtk_tree_view_get_model (GTK_TREE_VIEW (get_wg ("treeview_top10_2"))) );
	success = gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list1), &iter1);
	success = (success && gtk_tree_model_get_iter_first (GTK_TREE_MODEL (list2), &iter2));
	if (! success)
	{
		g_warning ("not able to set Top10 Treeviews");
		return;
	}
	for (i = 0; i < 10; i++)
	{
		if (top10[i].score == 0)
		{
			gtk_list_store_set (list1, &iter1, 1, "", 2, "", -1);
			gtk_list_store_set (list2, &iter2, 0, "", 1, "", 2, "", 3, "", 4, "", -1);
		}
		else
		{
			/* First treeview: main info
			 */
			tmp = g_strdup_printf ("%3.4f", top10[i].score);
			gtk_list_store_set (list1, &iter1, 1, top10[i].name, 2, tmp, -1);
			g_free (tmp);

			/* Second treeview: further info
			 */
			accur = g_strdup_printf ("%2.1f", top10[i].accur); 
			velo = g_strdup_printf ("%2.1f", top10[i].velo); 
			fluid = g_strdup_printf ("%2.1f", top10[i].fluid); 
			nchars = g_strdup_printf ("%i", top10[i].nchars); 
			ltime = localtime (&top10[i].when);
			date = g_strdup_printf ("%i-%2.2i-%2.2i %02i:%02i", (ltime->tm_year) + 1900,
						(ltime->tm_mon) + 1, (ltime->tm_mday),
						(ltime->tm_hour), (ltime->tm_min));

			gtk_list_store_set (list2, &iter2, 0, accur, 1, velo, 2, fluid, 3, nchars, 4, date, -1);

			g_free (accur);
			g_free (velo);
			g_free (fluid);
			g_free (nchars);
			g_free (date);
		}

		gtk_tree_model_iter_next (GTK_TREE_MODEL (list1), &iter1);
		gtk_tree_model_iter_next (GTK_TREE_MODEL (list2), &iter2);
	}
}

gboolean
top10_global_update (gpointer data)
{
	gboolean fail;
	gchar *tmp;
	gchar *ksc;
	gchar *host;
	gchar *command;
	GtkImage *img;
	CURL *curl;
	FILE *fh;
	
	img = GTK_IMAGE (get_wg ("image_top10_update"));
	top10_message (NULL);

	if (!main_curl_ok ())
	{
		tmp = g_strconcat (_("Not able to download files"), ": 'libcurl' ", _("not found"), ". ",
		       _("Are you sure you have it installed in your system?"), NULL);
		top10_message (tmp);
		g_free (tmp);
		gtk_image_set_from_icon_name (img, "go-bottom", GTK_ICON_SIZE_BUTTON);
		return FALSE;
	}

	/**************************************************
	 * Download from downhost
	 */
	host = g_strdup (DOWNHOST);
	ksc = top10_get_score_file (GLOBAL, -1);
	if (! (curl = curl_easy_init ()) )
	{
		g_message ("Not able to initialize 'curl'");
		gtk_image_set_from_icon_name (img, "go-bottom", GTK_ICON_SIZE_BUTTON);
		g_free (host);
		return FALSE;
	}

	if (!g_file_test (main_path_score (), G_FILE_TEST_IS_DIR))
		g_mkdir_with_parents (main_path_score (), DIR_PERM);
	tmp = g_build_filename (main_path_score (), ksc, NULL);
	fail = TRUE;
	command = g_strdup_printf ("http://%s/%s", host, ksc);
	if ( (fh = g_fopen (tmp, "wb")) )
	{
		/*
		curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
		 */
		curl_easy_setopt (curl, CURLOPT_TIMEOUT, TIMEOUT);
		curl_easy_setopt (curl, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);
		curl_easy_setopt (curl, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME);
		curl_easy_setopt (curl, CURLOPT_URL, command);
		curl_easy_setopt (curl, CURLOPT_WRITEDATA, fh);
		fail = curl_easy_perform (curl);
		fclose (fh);
	}
	curl_easy_cleanup (curl);
	g_free (command);
	g_free (host);

	if (fail)
	{
		top10_message (_("Could not download file from the host server."));
		gtk_image_set_from_icon_name (img, "go-bottom", GTK_ICON_SIZE_BUTTON);
		g_free (ksc);
		g_free (tmp);
		return FALSE;
	}

	if (!g_file_test (tmp, G_FILE_TEST_IS_REGULAR))
		g_message ("No file downloaded from the host server.");

	gtk_image_set_from_icon_name (img, "go-bottom", GTK_ICON_SIZE_BUTTON);
	g_free (tmp);
	g_free (ksc);


	if (gtk_combo_box_get_active (GTK_COMBO_BOX (get_wg ("combobox_top10"))) == 0)
		top10_show_stats (LOCAL);
	else
		top10_show_stats (GLOBAL);

	return FALSE;
}

gboolean
top10_global_publish (gpointer data)
{
	gint i;
	gboolean success;
	gchar *tmp;
	gchar *ksc;
	gchar *host;
	gchar *path;
	gchar *username;
	gchar *url;
	GtkImage *img;
	FILE *fh, *fh2;
	struct stat fs;
	CURL *curl;
	
	fs.st_size = 0;
	img = GTK_IMAGE (get_wg ("image_top10_publish"));
	top10_message (NULL);

	if (!main_curl_ok ())
	{
		tmp = g_strconcat (_("Not able to upload files"), ": 'libcurl' ", _("not found"), ". ",
		       _("Are you sure you have it installed in your system?"), NULL);
		top10_message (tmp);
		g_free (tmp);
		gtk_image_set_from_icon_name (img, "go-top", GTK_ICON_SIZE_BUTTON);
		return FALSE;
	}

	if (! (curl = curl_easy_init ()))
	{
		g_message ("Not able to initialize curl session");
		gtk_image_set_from_icon_name (img, "go-top", GTK_ICON_SIZE_BUTTON);
		return FALSE;
	}

	/**************************************************
	 * Upload to uphost, updating local ranking
	 */
	host = g_strdup (CGI_SERVER);
	tmp = main_preferences_get_string ("interface", "language");
	if (tmp[0] == 'C')
	{
		g_free (tmp);
		tmp = g_strdup ("en");
	}
	ksc = g_strdup_printf ("local_%c%c.ksc", tmp[0], tmp[1]);
	path = g_build_filename (main_path_score (), ksc, NULL);
	g_free (ksc);
	g_stat (path, &fs);

	username = g_strdup (g_get_real_name ());
	username = g_strdelimit (username, " ", '_');
	if (strlen (username) == 0)
	{
		g_free (username);
		username = g_strdup (g_get_user_name ());
	}
	ksc = g_strdup_printf ("%s_%s_%c%c.ksc", username, g_get_host_name (), tmp[0], tmp[1]);
	url = g_strdup_printf ("http://%s?dosiernomo=%s&lingvo=%c%c", host, ksc, tmp[0], tmp[1]);
	g_free (username);
	g_free (host);
	g_free (ksc);
	g_free (tmp);

	/*
	curl_easy_setopt (curl, CURLOPT_VERBOSE, 1);
	 */
	curl_easy_setopt (curl, CURLOPT_TIMEOUT, TIMEOUT);
	curl_easy_setopt (curl, CURLOPT_LOW_SPEED_LIMIT, LOW_SPEED_LIMIT);
	curl_easy_setopt (curl, CURLOPT_LOW_SPEED_TIME, LOW_SPEED_TIME);
	curl_easy_setopt (curl, CURLOPT_UPLOAD, TRUE);
	curl_easy_setopt (curl, CURLOPT_INFILESIZE, (long) fs.st_size);
	curl_easy_setopt (curl, CURLOPT_URL, url);
	success = FALSE;
	if ( (fh = g_fopen (path, "rb")) )
	{
		i = 0;
		while (1)
		{
			tmp = g_strdup_printf ("%s/klavaro_%03i.html", g_get_tmp_dir (), i++);
			if ((fh2 = g_fopen (tmp, "wb")))
				break;
			g_free (tmp);
		}
		g_free (tmp);
		curl_easy_setopt (curl, CURLOPT_READDATA, fh);
		curl_easy_setopt (curl, CURLOPT_WRITEDATA, fh2);
		if (curl_easy_perform (curl))
			g_message ("HTTP upload failed!");
		else
			success = TRUE;
		fclose (fh);
		fclose (fh2);
	}
	curl_easy_cleanup (curl);
	g_free (path);
	g_free (url);

	gtk_image_set_from_icon_name (img, "go-top", GTK_ICON_SIZE_BUTTON);

	if (!success)
		top10_message (_("Could not upload/download scores."));
	else
		g_idle_add ((GSourceFunc) top10_global_update, NULL);

	return FALSE;
}
