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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <pango/pango-attributes.h>
#include <gtk/gtk.h>
#include <curl/curl.h>

#include "auxiliar.h"
#include "callbacks.h"
#include "translation.h"
#include "keyboard.h"
#include "tutor.h"
#include "accuracy.h"
#include "top10.h"
#include "main.h"

/*******************************************************************************
 * Variables
 */
GtkBuilder *gui;

gchar *KEYB_CUSTOM;
gchar *KEYB_EDIT;
gchar *OTHER_DEFAULT;

static GKeyFile *preferences = NULL;
static gboolean curl_ok;
static struct
{
	gchar *user;
	gchar *stats;
	gchar *data;
	gchar *score;
} path;

/*******************************************************************************
 * Interface functions
 */
gchar *
main_path_user ()
{
	return (path.user);
}

gchar *
main_path_stats ()
{
	return (path.stats);
}

gchar *
main_path_data ()
{
	return (path.data);
}

gchar *
main_path_score ()
{
	return (path.score);
}

gboolean
main_curl_ok ()
{
	return (curl_ok);
}

gboolean
main_preferences_exist (gchar * group, gchar * key)
{
	return (g_key_file_has_key (preferences, group, key, NULL));
}

void
main_preferences_remove (gchar * group, gchar * key)
{
	g_key_file_remove_key (preferences, group, key, NULL);
}

gchar *
main_preferences_get_string (gchar * group, gchar * key)
{
	return (g_key_file_get_string (preferences, group, key, NULL));
}

void
main_preferences_set_string (gchar * group, gchar * key, gchar * value)
{
	g_key_file_set_string (preferences, group, key, value);
}

gint
main_preferences_get_int (gchar * group, gchar * key)
{
	return (g_key_file_get_integer (preferences, group, key, NULL));
}

void
main_preferences_set_int (gchar * group, gchar * key, gint value)
{
	g_key_file_set_integer (preferences, group, key, value);
}

gboolean
main_preferences_get_boolean (gchar * group, gchar * key)
{
	return (g_key_file_get_boolean (preferences, group, key, NULL));
}

void
main_preferences_set_boolean (gchar * group, gchar * key, gboolean value)
{
	g_key_file_set_boolean (preferences, group, key, value);
}

void
main_preferences_save ()
{
	gchar *tmp_name;
	FILE *fh;

	/*
	 * Save preferences
	 */
	assert_user_dir ();
	tmp_name = g_build_filename (main_path_user (), "preferences.ini", NULL);
	fh = (FILE *) g_fopen (tmp_name, "w");
	if (fh == NULL)
		g_warning ("couldn't save your preferences at the user folder:\n %s", tmp_name);
	else
	{
		g_free (tmp_name);
		tmp_name = NULL;
		tmp_name = g_key_file_to_data (preferences, NULL, NULL);
		if (tmp_name != NULL)
			fputs (tmp_name, fh);
		else
			g_warning ("couldn't save your preferences at the user folder:\n %s", tmp_name);
		fclose (fh);
	}
	g_free (tmp_name);
}

/*
 * End of interface, start of private functions
 */

/**********************************************************************
 * Initialize the value of the global variables
 */
static void
main_initialize_global_variables ()
{
	gchar *tmp;
	FILE *fh;

	/* Set the home user dir
	 */
	path.user = g_build_filename (g_get_user_config_dir (), "klavaro", NULL);
	if (!g_file_test (path.user, G_FILE_TEST_IS_DIR))
	{
		if (g_mkdir_with_parents (path.user, DIR_PERM) == -1)
			g_error ("Sorry, not able to create the user config dir: %s", path.user);
	}

	/* Set the home user stats dir (~/.local/share dir)
	 */
	if (UNIX_OK)
		path.stats = g_build_filename (g_get_user_data_dir (), "klavaro", NULL);
	else
		path.stats = g_build_filename (g_get_user_config_dir (), "klavaro", NULL);
	if (!g_file_test (path.stats, G_FILE_TEST_IS_DIR))
	{
		g_mkdir_with_parents (path.stats, DIR_PERM);
	}

	/* Get a valid data path.
	 * First searches the path at the source directory.
	 */
	path.data = g_build_filename ("..", "data", NULL);
	tmp = g_build_filename (path.data, "basic_lessons.txt", NULL);
	fh = (FILE *) g_fopen (tmp, "r");
	if (fh == NULL)
	{
		g_free (path.data);
		g_free (tmp);
		path.data = g_build_filename (PACKAGE_DATA_DIR, PACKAGE, NULL);
		tmp = g_build_filename (path.data, "basic_lessons.txt", NULL);
		fh = (FILE *) g_fopen (tmp, "r");
	}
	if (fh == NULL)
		g_error ("couldn't find a data file at the data path:\n %s", tmp);
	fclose (fh);
	g_free (tmp);

	/* Get a valid scoring path.
	 */
	path.score = g_build_filename (path.stats, "ksc", NULL);
	if (!g_file_test (path.score, G_FILE_TEST_IS_DIR))
	{
		g_mkdir_with_parents (path.score, DIR_PERM);
	}

	/* Retrieve initial or saved preferences
	 */
	preferences = g_key_file_new ();
	tmp = g_build_filename (main_path_user (), "preferences.ini", NULL);
	if (!g_file_test (tmp, G_FILE_TEST_EXISTS))
	{
		g_free (tmp);
		tmp = g_strdup ("/etc/klavaro_preferences.ini");
	}
	g_key_file_load_from_file (preferences, tmp, G_KEY_FILE_NONE, NULL);
	g_free (tmp);

	/* Other initializations
	 */
	trans_init_lang_name_code ();
	trans_init_language_env ();
	srand (time (0));
	tutor_init_timers ();

	KEYB_CUSTOM = g_strdup (_("(Custom)"));
	KEYB_EDIT = g_strdup (_("(Edit custom)"));
	OTHER_DEFAULT = g_strdup (_("(Default)"));
}

static void
main_gtkbuilder_translation_workaround ()
{
	gchar *lb;
	GSList *ls;
	GSList *it;
	GtkWidget *wg;

	ls = gtk_builder_get_objects (gui);
	it = ls;
	do
	{
		if (G_OBJECT_TYPE (it->data) == GTK_TYPE_LABEL)
		{
			lb = (gchar *) gtk_label_get_label (GTK_LABEL (it->data));
			if (lb[0] != '\0')
				gtk_label_set_text_with_mnemonic (GTK_LABEL (it->data), _(lb));
		}
		else if (G_OBJECT_TYPE (it->data) == GTK_TYPE_ENTRY)
		{
			lb = gtk_entry_get_icon_tooltip_text (GTK_ENTRY (it->data), GTK_ENTRY_ICON_SECONDARY);
			if (lb)
			{
				if (lb[0] != '\0')
					gtk_entry_set_icon_tooltip_text (GTK_ENTRY (it->data), GTK_ENTRY_ICON_SECONDARY, _(lb));
			}
		}
		else if (G_OBJECT_TYPE (it->data) == GTK_TYPE_WINDOW)
			gtk_window_set_title (GTK_WINDOW (it->data), _(gtk_window_get_title (GTK_WINDOW (it->data))));

		if (GTK_IS_WIDGET (it->data))
		{
			wg = GTK_WIDGET (it->data);
			if (gtk_widget_get_has_tooltip (wg))
				gtk_widget_set_tooltip_text (wg, _(gtk_widget_get_tooltip_text (wg)));
		}

		it = g_slist_next (it);
	} while (it);

	g_slist_free (ls);
}

/*******************************************************************************
 * Initialize some interface widgets
 */
static void
main_window_init ()
{
	gchar *tmp;
	gchar *ttip;
	PangoAttrList *palist;

	/* Workaround to make GtkBuilder translate all text, on Windows :-(
	 */
	if (! UNIX_OK)
		main_gtkbuilder_translation_workaround ();

	/* Set the language
	 */
	trans_set_combo_language ();

	/* Set keyboard
	 */
	keyb_create_virtual_keys ();
	hints_init ();

	/* Set if speech is enabled
	 */
	callbacks_shield_set (TRUE);
	if (!main_preferences_exist ("interface", "speech"))
		main_preferences_set_boolean ("interface", "speech", TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (get_wg ("checkbutton_speech")),
		       	main_preferences_get_boolean ("interface", "speech"));
	tmp = dngettext (PACKAGE, "Dictation mode (depends on this speech synthesizer: %s)",
		       	"Dictation mode (depends on one of these speech synthesizers: %s)", 1);
	ttip = g_strdup_printf (tmp, "Espeak");
	gtk_widget_set_tooltip_text (get_wg ("checkbutton_speech"), ttip);
	callbacks_shield_set (FALSE);
	

	/* Set the initial keyboard to use
	 */
	keyb_set_combo_kbd ("combobox_kbd_country", "combobox_kbd_variant");
	keyb_set_combo_kbd ("combobox_keyboard_country", "combobox_keyboard_variant");

	/* Set window icons
	 */
	gtk_window_set_default_icon_name ("klavaro");

	/* Set pixmaps
	 */
	set_pixmap ("image_fluid", "fluid.png");
	set_pixmap ("image_keyboard", "key.png");
	set_pixmap ("image_beep", "beep.png");
	set_pixmap ("image_progress", "progress.png");
	set_pixmap ("image_other", "other.png");
	set_pixmap ("image_top10", "top10.png");

	/* Set Top10 TreeViews and Combo
	 */
	top10_init ();

	/* Set big and bold labels
	 */
	pango_parse_markup ("<big><b>----------------------------------------------------------------</b></big>",
			    -1, 0, &palist, NULL, NULL, NULL);
	gtk_label_set_attributes (GTK_LABEL (get_wg ("label_keyboard_title")), palist);
	gtk_label_set_attributes (GTK_LABEL (get_wg ("label_main_intro")), palist);
	gtk_label_set_attributes (GTK_LABEL (get_wg ("label_main_basic")), palist);
	gtk_label_set_attributes (GTK_LABEL (get_wg ("label_main_adapt")), palist);
	gtk_label_set_attributes (GTK_LABEL (get_wg ("label_main_velo")), palist);
	gtk_label_set_attributes (GTK_LABEL (get_wg ("label_main_fluid")), palist);

	/* Set main labels (for translation)
	 */
	tmp = NULL;
	tmp = g_strdup_printf ("0 - %s", _("Introduction"));
	gtk_label_set_text (GTK_LABEL (get_wg ("label_main_intro")), tmp);
	g_free (tmp);
	tmp = g_strdup_printf ("1 - %s", _("Basic course"));
	gtk_label_set_text (GTK_LABEL (get_wg ("label_main_basic")), tmp);
	g_free (tmp);
	tmp = g_strdup_printf ("2 - %s", _("Adaptability"));
	gtk_label_set_text (GTK_LABEL (get_wg ("label_main_adapt")), tmp);
	g_free (tmp);
	tmp = g_strdup_printf ("3 - %s", _("Speed"));
	gtk_label_set_text (GTK_LABEL (get_wg ("label_main_velo")), tmp);
	g_free (tmp);
	tmp = g_strdup_printf ("4 - %s", _("Fluidity"));
	gtk_label_set_text (GTK_LABEL (get_wg ("label_main_fluid")), tmp);
	g_free (tmp);

	/* Set version
	 */
	gtk_about_dialog_set_version (GTK_ABOUT_DIALOG (get_obj ("aboutdialog_klavaro")), VERSION);

	/* For remembering the window's position
	 */
	gtk_widget_show (get_wg ("window_main"));
	window_restore ("main");

	/* Run last used module
	 */
	if (!main_preferences_exist ("interface", "autostart"))
		main_preferences_set_boolean ("interface", "autostart", FALSE);
	if (!main_preferences_exist ("interface", "exercise"))
		main_preferences_set_int ("interface", "exercise", 1);
	if (main_preferences_get_boolean ("interface", "autostart"))
	{
		switch (main_preferences_get_int ("interface", "exercise"))
		{
		case TT_BASIC:
			on_button_basic_clicked (NULL, NULL);
			break;
		case TT_ADAPT:
			on_button_adapt_clicked (NULL, NULL);
			break;
		case TT_VELO:
			on_button_velo_clicked (NULL, NULL);
			break;
		case TT_FLUID:
			on_button_fluid_clicked (NULL, NULL);
			break;
		}
	}

	/* Set accuracy accumulators
	 */
	accur_init ();
}

/*******************************************************************************
 * Main program
 */
int
main (int argc, char *argv[])
{
	gchar *tmp;
	gboolean success = FALSE;
	gboolean show_version = FALSE;
	GOptionContext *opct;
	GOptionEntry option[] = {
		{"version", 'v', 0, G_OPTION_ARG_NONE, &show_version, "Versio", NULL},
		{NULL}
	};
	GError *gerr;

	/* Localization
	 */
#ifdef ENABLE_NLS
	bindtextdomain (GETTEXT_PACKAGE, PACKAGE_LOCALE_DIR);
	bind_textdomain_codeset (GETTEXT_PACKAGE, "UTF-8");
	textdomain (GETTEXT_PACKAGE);
#endif

	/* Command-line arguments
	 */
	opct = g_option_context_new ("");
	g_option_context_set_translation_domain (opct, GETTEXT_PACKAGE);
	g_option_context_add_main_entries (opct, option, GETTEXT_PACKAGE);
	g_option_context_add_group (opct, gtk_get_option_group (TRUE));
	g_setenv ("NO_AT_BRIDGE", "1", FALSE); /* to eliminate annoying accessibility bus warning */
	g_option_context_parse (opct, &argc, &argv, &gerr);

	if (show_version)
	{
		g_printf (VERSION"\n");
		return 0;
	}

	curl_ok = curl_global_init (CURL_GLOBAL_WIN32) == CURLE_OK ? TRUE : FALSE;

	main_initialize_global_variables ();	/* Here the locale is got. */

	/* Create all the interface stuff
	 */
	gui = gtk_builder_new ();
	gtk_builder_set_translation_domain (gui, NULL);

	tmp = g_build_filename (main_path_data (), "klavaro.glade", NULL);
	if (g_file_test (tmp, G_FILE_TEST_IS_REGULAR))
		success = gtk_builder_add_from_file (gui, tmp, NULL);
	else
		g_error ("GUI file not found. Aborting.\n %s", tmp);
	if (!success)
		g_error ("GUI file found but couldn't create the GUI. Aborting.");
	if (!g_module_supported ())
		g_error ("GUI created but can't connect signals.");
	g_free (tmp);
	gtk_builder_connect_signals (gui, NULL);

	main_window_init ();	/* and initialize its parameters */

	gtk_main ();

	return 0;
}

/*******************************************************************************
 * Quit the application
 */
void
main_window_pass_away ()
{
	main_preferences_save ();
	accur_close ();
	g_rmdir ("tmp/klavaro");
	if (curl_ok) curl_global_cleanup ();
	g_print ("\nAdiaux!\n");
	exit (0);
}
