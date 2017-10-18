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

/* Functions to implement and manage the keyboard editing operations
 */
#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "auxiliar.h"
#include "main.h"
#include "callbacks.h"
#include "translation.h"
#include "keyboard.h"

#define KEY_DX 35
#define KEY_DY 35

extern gchar *KEYB_CUSTOM;
extern gchar *KEYB_EDIT;

static struct
{
	gchar *name;
	gchar *name_last;
	gboolean modified_status;
	gunichar lochars[4][KEY_LINE_LEN + 1];
	gunichar upchars[4][KEY_LINE_LEN + 1];
	GtkWidget *but[4][KEY_LINE_LEN - 1];
	GtkWidget *lab[4][KEY_LINE_LEN - 1];
	GtkWidget *entry;
	struct
	{
		guint i;
		guint j;
	} pos;
	gint cmb_n;
	gint intro_step;
} keyb;
static guint x0[4] = {0, 49, 59, 43};

static struct
{
	KeybLayout *orig; // Original layouts already defined
	gint n_orig;
	KeybLayout *cust; // Custom layouts created by the user
	gint n_cust;
} layouts;

/* Constants
 */
const gunichar vowels[] = {
	L'a', L'e', L'i', L'o', L'u',
	(gunichar) 945,
	(gunichar) 949,
	(gunichar) 953,
	(gunichar) 959,
	(gunichar) 965,
	(gunichar) 1072,
	(gunichar) 1077,
	(gunichar) 1080,
	(gunichar) 1086,
	(gunichar) 1091,
	L'\0'
};

/* Diacritic chars should never appear one after another
 */
const gunichar diacritics[] = {
	// Urdu diacritics
	(gunichar) 0x0640,
	(gunichar) 0x064B,
	(gunichar) 0x064E,
	(gunichar) 0x064F,
	(gunichar) 0x0650,
	(gunichar) 0x0651,
	(gunichar) 0x0654,
	(gunichar) 0x0670,
	// Other arabic diacritics
	(gunichar) 0x0610,
	(gunichar) 0x0611,
	(gunichar) 0x0612,
	(gunichar) 0x0613,
	(gunichar) 0x0614,
	(gunichar) 0x0615,
	(gunichar) 0x0616,
	(gunichar) 0x0617,
	(gunichar) 0x0618,
	(gunichar) 0x0619,
	(gunichar) 0x061A,
	(gunichar) 0x064C,
	(gunichar) 0x064D,
	(gunichar) 0x0652,
	(gunichar) 0x0653,
	(gunichar) 0x0655,
	(gunichar) 0x0656,
	(gunichar) 0x0657,
	(gunichar) 0x0658,
	(gunichar) 0x0659,
	(gunichar) 0x065A,
	(gunichar) 0x065B,
	(gunichar) 0x065C,
	(gunichar) 0x065D,
	(gunichar) 0x065E,
	(gunichar) 0x06D6,
	(gunichar) 0x06D7,
	(gunichar) 0x06D8,
	(gunichar) 0x06D9,
	(gunichar) 0x06DA,
	(gunichar) 0x06DB,
	(gunichar) 0x06DC,
	(gunichar) 0x06DF,
	(gunichar) 0x06E0,
	(gunichar) 0x06E1,
	(gunichar) 0x06E2,
	(gunichar) 0x06E3,
	(gunichar) 0x06E4,
	(gunichar) 0x06E7,
	(gunichar) 0x06E8,
	(gunichar) 0x06EA,
	(gunichar) 0x06EB,
	(gunichar) 0x06EC,
	(gunichar) 0x06ED,
	L'\0'
};

/*******************************************************************************
 * Interface functions
 */
gchar *
keyb_get_name ()
{
	return (keyb.name);
}

gchar *
keyb_get_name_last ()
{
	return (keyb.name_last);
}

void
keyb_set_name (const gchar * name)
{
	g_free (keyb.name_last);
	keyb.name_last = g_strdup (keyb.name);
	g_free (keyb.name);
	keyb.name = g_strdup (name);
}

void
keyb_init_name (const gchar * name)
{
	keyb.name = g_strdup (name);
	keyb.name_last = g_strdup (name);
}

gunichar
keyb_get_lochars (gint i, gint j)
{
	return (keyb.lochars[i][j]);
}

gunichar
keyb_get_upchars (gint i, gint j)
{
	return (keyb.upchars[i][j]);
}

gboolean
keyb_get_modified_status ()
{
	return (keyb.modified_status);
}

void
keyb_set_modified_status (gboolean new_status)
{
	gtk_widget_set_sensitive (get_wg ("combobox_keyboard_country"), ! new_status);
	gtk_widget_set_sensitive (get_wg ("combobox_keyboard_variant"), ! new_status);
	gtk_widget_set_sensitive (get_wg ("button_kb_save"), new_status);
	keyb.modified_status = new_status;
	if (new_status)
	{
		callbacks_shield_set (TRUE);
		gtk_combo_box_set_active (GTK_COMBO_BOX (get_wg ("combobox_keyboard_country")), 0);
		gtk_combo_box_set_active (GTK_COMBO_BOX (get_wg ("combobox_keyboard_variant")), -1);
		callbacks_shield_set (FALSE);
	}
}

void
keyb_create_virtual_keys ()
{
	gint i, j;
	gchar *hlp;
	GtkFixed *fix;
	GdkRGBA color;
	
	/* Set color of space key
	 */
	if (main_preferences_exist ("colors", "key_5"))
		hlp = main_preferences_get_string ("colors", "key_5");
	else
		hlp = g_strdup (KEYB_PURPLE);
	gdk_rgba_parse (&color, hlp);
	gtk_widget_override_background_color (get_wg ("but_space"), GTK_STATE_FLAG_NORMAL, &color);
	g_free (hlp);

	/* Set text color of keys
	 */
	if (main_preferences_exist ("colors", "key_fg"))
		hlp = main_preferences_get_string ("colors", "key_fg");
	else
		hlp = g_strdup (KEYB_BLACK);
	gdk_rgba_parse (&color, hlp);
	g_free (hlp);

	/* Create and position buttons and labels
	 */
	fix = GTK_FIXED (get_wg ("fixed_keyboard"));
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < KEY_LINE_LEN - 1; j++)
		{
			keyb.but[i][j] = gtk_button_new ();
			gtk_fixed_put (fix, keyb.but[i][j], x0[i] + j * KEY_DX, i * KEY_DY);
			gtk_widget_set_size_request (keyb.but[i][j], 32, 32);
  			g_signal_connect_after ((gpointer) keyb.but[i][j], "clicked",
				       	G_CALLBACK (on_virtual_key_clicked), NULL);
  			g_signal_connect_after ((gpointer) keyb.but[i][j], "grab-focus",
				       	G_CALLBACK (on_virtual_key_grab_focus), NULL);
			keyb.lab[i][j] = gtk_label_new ("0");
			gtk_widget_override_color (keyb.lab[i][j], GTK_STATE_FLAG_NORMAL, &color);
			gtk_widget_override_color (keyb.lab[i][j], GTK_STATE_FLAG_PRELIGHT, &color);
			gtk_container_add (GTK_CONTAINER (keyb.but[i][j]), keyb.lab[i][j]);

			if (i > 0)
			{
				if (i == 1)
				{
					if (j > 12)
						continue;
				}
				else
				{
					if (j > 11)
						continue;
				}
			}
			gtk_widget_show (keyb.but[i][j]);
			gtk_widget_show (keyb.lab[i][j]);
		}
	}
	gtk_widget_set_size_request (keyb.but[1][12], 53, 32);

	/* Key entry little evil
	 */
  	keyb.entry = gtk_entry_new ();
  	gtk_fixed_put (fix, keyb.entry, 2, 2);
	gtk_entry_set_width_chars (GTK_ENTRY (keyb.entry), 1);
	gtk_entry_set_max_length (GTK_ENTRY (keyb.entry), 1);
	gtk_entry_set_alignment (GTK_ENTRY (keyb.entry), 0.5);
	gtk_widget_set_size_request (keyb.entry, 28, 28);
	g_object_set (G_OBJECT (keyb.entry), "shadow-type", GTK_SHADOW_NONE, NULL);
  	g_signal_connect_after ((gpointer) keyb.entry, "changed", G_CALLBACK (on_virtual_key_changed), NULL);
}

/**********************************************************************
 * Read the character sets (keyb.lochars[] & keyb.upchars[])
 * for the keyboard currently selected.
 */
void
keyb_set_chars ()
{
	gint i;
	gchar *tmp_name = NULL;
	gchar tmp_str[6 * KEY_LINE_LEN + 1];
	glong n_itens;
	gunichar *uchs;
	FILE *fh;

	/* Search at home
	 */
	tmp_name = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, keyb.name, ".kbd", NULL);
	fh = (FILE *) g_fopen (tmp_name, "r");
	if (fh == NULL)
	{
		/* Search at data
		 */
		g_free (tmp_name);
		tmp_name = g_strconcat (main_path_data (), G_DIR_SEPARATOR_S, keyb.name, ".kbd", NULL);
		fh = (FILE *) g_fopen (tmp_name, "r");
	}
	g_free (tmp_name);

	/* Success */
	if (fh)
	{
		for (i = 0; i < 4; i++)
		{
			tmp_name = fgets (tmp_str, 6 * KEY_LINE_LEN + 1, fh);
			tmp_str[6 * KEY_LINE_LEN] = '\0';
			uchs = g_utf8_to_ucs4_fast (tmp_str, -1, &n_itens);
			if (n_itens > KEY_LINE_LEN)
				g_error ("invalid keyboard layout: %s\n"
					 "invalid line: %i\n"
					 "invalid number of chars: %li", keyb.name, i + 1, n_itens);
			memcpy (keyb.lochars[i], uchs, (n_itens - 1) * sizeof (gunichar));
			g_free (uchs);
			for (; n_itens < KEY_LINE_LEN; n_itens++)
				keyb.lochars[i][n_itens] = L' ';
		}
		for (i = 0; i < 4; i++)
		{
			tmp_name = fgets (tmp_str, 6 * KEY_LINE_LEN + 1, fh);
			tmp_str[6 * KEY_LINE_LEN] = '\0';
			uchs = g_utf8_to_ucs4_fast (tmp_str, -1, &n_itens);
			if (n_itens > KEY_LINE_LEN)
				g_error ("invalid keyboard layout: %s\n"
					 "invalid line: %i\n"
					 "invalid number of chars: %li", keyb.name, i + 5, n_itens);
			memcpy (keyb.upchars[i], uchs, (n_itens - 1) * sizeof (gunichar));
			g_free (uchs);
			for (; n_itens < KEY_LINE_LEN; n_itens++)
				keyb.upchars[i][n_itens] = L' ';
		}
		fclose (fh);

		keyb_set_modified_status (FALSE);
	}
	/*
	 * Recursively try defaults
	 */
	else
	{
		if (g_str_equal (keyb.name, trans_get_default_keyboard ()))
		{
			main_preferences_remove ("tutor", "keyboard");
			g_error ("couldn't open the default keyboard layout: [%s]", trans_get_default_keyboard ());
		}

		g_message ("couldn't find the keyboard layout: \"%s\"\n"
			   " Opening the default one: \"%s\"", keyb.name, trans_get_default_keyboard ());
		main_preferences_set_string ("tutor", "keyboard", trans_get_default_keyboard());
		keyb_set_name (trans_get_default_keyboard ());
		keyb_set_chars ();
		return;
	}
}

/**********************************************************************
 * Test if chr belongs to the current key set
 */
gboolean keyb_is_inset (gunichar chr)
{
	register gint i, j;

	for (i = 0; i < 4; i++)
		for (j = 0; j <= KEY_LINE_LEN; j++)
			if (chr == keyb.lochars[i][j])
				return (TRUE);

	for (i = 0; i < 4; i++)
		for (j = 0; j <= KEY_LINE_LEN; j++)
			if (chr == keyb.upchars[i][j])
				return (TRUE);
	return (FALSE);
}

/**********************************************************************
 * Test if chr is a vowel 
 */
gboolean
keyb_is_vowel (gunichar chr)
{
	gint i;

	for (i = 0; vowels[i] != L'\0'; i++)
		if (g_unichar_tolower (chr) == vowels[i])
			return (TRUE);
	return (FALSE);
}

/**********************************************************************
 * Test if chr is a diacritic character 
 */
gboolean
keyb_is_diacritic (gunichar chr)
{
	gint i;

	for (i = 0; diacritics[i] != L'\0'; i++)
		if (chr == diacritics[i])
			return (TRUE);
	return (FALSE);
}

/**********************************************************************
 * Get the set of available vowels of the keyboard
 */
gint
keyb_get_vowels (gunichar * vows)
{
	gint i;
	gint j;
	gint k = 0;

	for (i = 0; i < 4; i++)
		for (j = 0; j < KEY_LINE_LEN; j++)
		{
			if (keyb_is_vowel (keyb.lochars[i][j]))
				vows[k++] = keyb.lochars[i][j];
			if (k == 20)
				break;
		}
	if (k == 0)
		for (i = j = 0, k = 5; i < 5 && j < 10; i++, j++)
		{
			for (; keyb_is_diacritic (keyb.lochars[2][j]) && j < 12; j++);
			vows[i] = keyb.lochars[2][j];
		}
	return (k);
}

/**********************************************************************
 * Get the set of available consonants of the keyboard
 */
gint
keyb_get_consonants (gunichar * consonants)
{
	gint i, j;
	gint k = 0;
	gunichar chr;

	for (i = 0; i < 4; i++)
		for (j = 0; j < KEY_LINE_LEN; j++)
		{
			chr = keyb.lochars[i][j];
			if (g_unichar_isalpha (chr) && (!keyb_is_vowel (chr)))
				consonants[k++] = chr;

			chr = g_unichar_tolower (keyb.upchars[i][j]);
			if (g_unichar_isalpha (chr) && (!keyb_is_vowel (chr))
			    && (chr != keyb.lochars[i][j]))
				consonants[k++] = chr;
		}
	return (k);
}

/**********************************************************************
 * Get the set of available symbols of the keyboard
 */
gint
keyb_get_symbols (gunichar * symbols)
{
	gint i, j;
	gint k = 0;
	gunichar chr;

	for (i = 0; i < 4; i++)
		for (j = 0; j < KEY_LINE_LEN; j++)
		{
			chr = keyb.lochars[i][j];
			if (g_unichar_ispunct (chr))
				symbols[k++] = chr;

			chr = keyb.upchars[i][j];
			if (g_unichar_ispunct (chr))
				symbols[k++] = chr;
		}
	return (k);
}

/**********************************************************************
 * Get the set of available non-arabic digits in the keyboard
 */
gint
keyb_get_altnums (gunichar * altnums)
{
	gint i, j;
	gint k = 0;
	gunichar chr;

	for (i = 0; i < 4; i++)
		for (j = 0; j < KEY_LINE_LEN; j++)
		{
			chr = keyb.lochars[i][j];
			if (g_unichar_isdigit (chr) && chr > 255)
				altnums[k++] = chr;

			chr = keyb.upchars[i][j];
			if (g_unichar_isdigit (chr) && chr > 255)
				altnums[k++] = chr;
		}
	return (k);
}

/**********************************************************************
 * Get the upper case of a letter, only if it's included in the keyboard (by shift)
 */
gunichar
keyb_unichar_toupper (gunichar uchar)
{
	gint i,j;
	gunichar Uchar;

	Uchar = g_unichar_toupper (uchar);
	for (i = 0; i < 4; i++)
		for (j = 0; j < KEY_LINE_LEN; j++)
			if (uchar == keyb.lochars[i][j] && Uchar == keyb.upchars[i][j])
				return Uchar;
	return uchar;
}

/**********************************************************************
 * Save the custom keyboard layout created by the user
 */
void
keyb_save_new_layout ()
{
	gint i;
	gchar *tmp_name = NULL;
	FILE *fh;

	assert_user_dir ();
	tmp_name = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, keyb.name, ".kbd", NULL);
	fh = (FILE *) g_fopen (tmp_name, "w");
	g_free (tmp_name);

	for (i = 0; i < 4; i++)
	{
		tmp_name = g_ucs4_to_utf8 (keyb.lochars[i], KEY_LINE_LEN - 1, NULL, NULL, NULL);
		fprintf (fh, "%s\n", tmp_name);
		g_free (tmp_name);
	}
	for (i = 0; i < 4; i++)
	{
		tmp_name = g_ucs4_to_utf8 (keyb.upchars[i], KEY_LINE_LEN - 1, NULL, NULL, NULL);
		fprintf (fh, "%s\n", tmp_name);
		g_free (tmp_name);
	}
	fclose (fh);

	keyb_set_modified_status (FALSE);
}

/**********************************************************************
 * Remove custom keyboard layout created by the user
 */
void
keyb_remove_user_layout ()
{
	guint active;
	gchar *aux;
	gchar *tmp_name;
	GtkComboBox *cmb;

	callbacks_shield_set (TRUE);

	cmb = GTK_COMBO_BOX (get_wg ("combobox_keyboard_variant"));
	aux = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));
	active = gtk_combo_box_get_active (cmb);
	gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (cmb), active);

	tmp_name = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, aux, ".kbd", NULL);
	g_unlink (tmp_name);
	g_free (tmp_name);

	keyb_set_keyboard_layouts ();

	gtk_combo_box_set_active (cmb, -1);

	callbacks_shield_set (FALSE);
}


/**********************************************************************
 * Update the virtual keyboard accordingly to its character set and
 * shift key state.
 */
void
keyb_update_virtual_layout ()
{
	gint i, j;
	gchar ut8[7];
	gunichar uch;
	gboolean tog_state;
	GtkWidget *wg;

	wg = get_wg ("toggle_shift1");
	tog_state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wg));
	for (i = 0; i < 4; i++)
	{
		for (j = 0; j < KEY_LINE_LEN - (i == 0 ? 1 : (i == 1 ? 2 : 3)); j++)
		{
			uch = tog_state ? keyb.upchars[i][j] : keyb.lochars[i][j];
			if (g_unichar_isalpha (uch)
			    && g_unichar_tolower (keyb.upchars[i][j]) == keyb.lochars[i][j])
				uch = g_unichar_toupper (uch);
			ut8[g_unichar_to_utf8 (uch, ut8)] = '\0';
			gtk_label_set_text (GTK_LABEL (keyb.lab[i][j]), ut8);
		}
	}
}

/* Get a list of .kbd file names in the subdir path, stripping their .kbd extensions
 */
GList *
keyb_get_layout_list_from_path (gchar *path)
{
	gsize name_len;
	GDir *dir = NULL;
	gchar *dentry = NULL;
	GList *files = NULL;

	dir = g_dir_open (path, 0, NULL);
	if (dir == NULL)
		g_error ("keyb_get_layout_list_from_path ():\n\tCould not find this directory:\n\t%s\n", path);

	while ( (dentry = g_strdup (g_dir_read_name (dir))) )
	{
		name_len = strlen (dentry);
		if (name_len > 255 || name_len < 5)
		{
			g_free (dentry);
			continue;
		}

		if (! g_str_has_suffix (dentry, ".kbd"))
		{
			g_free (dentry);
			continue;
		}

		dentry[name_len - 4] = '\0';
		if (g_str_equal (dentry, ".tmp"))
		{
			g_free (dentry);
			continue;
		}
		files = g_list_insert_sorted (files, dentry, compare_string_function);
	}
	g_dir_close (dir);

	return (files);
}

/* Get the country from a keyboard name
 */
gchar *
keyb_get_country (const gchar *kbd)
{
	gchar *country = NULL;
	gchar *code = NULL;

	code = strchr (kbd, '_');
	if (code)
		code = strdup (code + 1);
	else
		code = strdup ("xx");
	code[2] = '\0';
	country = g_strdup (trans_code_to_country (code));
	g_free (code);

	return country;
}

/* Get the variant from a keyboard name
 */
gchar *
keyb_get_variant (const gchar *kbd)
{
	gchar *begin;
	gchar *end;

	begin = g_strdup (kbd);
	end = strchr (begin, '_');
	if (end == NULL)
		return begin;
	*end = '\0';
	end++;
	end = strchr (end, '_');
	if (end == NULL)
		return begin;
	end = g_strconcat (begin, end, NULL);
	g_free (begin);
		
	return end;
}

/* Set the array of available keyboard layouts
 */
#define LAYOUT_BLOCK 64
void
keyb_set_keyboard_layouts ()
{
	static gboolean init = FALSE;
	gchar *data;
	gint i;
	GList *files;

	if (! init)
	{

		/* Read original layouts just once, now.
		 */
		files = keyb_get_layout_list_from_path (main_path_data ());
		layouts.n_orig = g_list_length (files);
		layouts.orig = g_malloc (layouts.n_orig * sizeof (KeybLayout));
		//g_printf ("==> Data dir: %s\n", main_path_data ());
		for (i = 0; i < layouts.n_orig; i++)
		{
			data = g_list_nth_data (files, i);
			//g_printf ("kb(%i): %s\n", i, data);
			layouts.orig[i].name = data;
			layouts.orig[i].country = keyb_get_country (data);
			layouts.orig[i].variant = keyb_get_variant (data);
			//g_printf ("kb(%i): %s\t", i, layouts.orig[i].name);
			//g_printf ("%s\t", layouts.orig[i].country);
			//g_printf ("%s\n", layouts.orig[i].variant);
		}
		g_list_free (files);

		init = TRUE;
		layouts.n_cust = 0;
		layouts.cust = g_malloc (LAYOUT_BLOCK * sizeof (KeybLayout));
	}

	/*
	 * Reads the list of custom files
	 */
	for (i = 0; i < layouts.n_cust; i++)
		g_free (layouts.cust[i].name);
	assert_user_dir ();
	files = keyb_get_layout_list_from_path (main_path_user ());
	layouts.n_cust = g_list_length (files);
	if (layouts.n_cust == 0)
		return;
	if (layouts.n_cust > LAYOUT_BLOCK)
		layouts.cust = g_realloc (layouts.cust, layouts.n_cust * sizeof (KeybLayout));
	for (i = 0; i < layouts.n_cust; i++)
	{
		data = g_list_nth_data (files, i);
		layouts.cust[i].name = data;
		//g_printf ("kb(%i): %s\n", i, layouts.cust[i].name);
	} 
	g_list_free (files);
}

void
keyb_update_from_variant (gchar *cmb_country, gchar *cmb_variant)
{
	gint i;
	gchar *country;
	gchar *variant;
	GtkComboBox *cmb;

	cmb = GTK_COMBO_BOX (get_wg (cmb_country));
	country = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));
	if (country == NULL)
		return;

	cmb = GTK_COMBO_BOX (get_wg (cmb_variant));
	variant = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));
	if (variant == NULL)
	{
		g_free (country);
		return;
	}

	callbacks_shield_set (TRUE);

	if (g_str_equal (country, KEYB_CUSTOM))
	{
		/* Update the keyboard for a custom layout
		 */
		if (! g_str_equal (variant, KEYB_EDIT))
		{
			keyb_set_name (variant);
			keyb_set_chars ();
			keyb_update_virtual_layout ();
		}
	}
	else
	{
		/* Update it for a original layout
		 */
		for (i = 0; i < layouts.n_orig; i++)
		{
			if (g_str_equal (layouts.orig[i].country, country))
				if (g_str_equal (layouts.orig[i].variant, variant))
					break;
		}

		if (i == layouts.n_orig)
			g_warning ("selected unavailable keyboard layout.");
		else
		{
			keyb_set_name (layouts.orig[i].name);
			keyb_set_chars ();
			keyb_update_virtual_layout ();
		}
	}

	g_free (country);
	g_free (variant);

	callbacks_shield_set (FALSE);
}

void
keyb_set_combo_kbd_variant (gchar *cmb_country, gchar *cmb_variant)
{
	gint i;
	gint n;
	gchar *country_txt;
	gboolean valid;
	GtkComboBox *cmb;
	GtkTreeModel *tmd;
	GtkTreeIter iter;

	callbacks_shield_set (TRUE);

	/* Clear the combo variant
	 */
	cmb = GTK_COMBO_BOX (get_wg (cmb_variant));
	tmd = gtk_combo_box_get_model (cmb);
	n = 0;
	valid = gtk_tree_model_get_iter_first (tmd, &iter);
	while (valid)
	{
		n++;
		valid = gtk_tree_model_iter_next (tmd, &iter);
	}
	for (i = 0; i < n; i++)
		gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (cmb), 0);

	/* Get the selected country text
	 */
	country_txt = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (get_wg (cmb_country)));
	if (country_txt == NULL)
	{
		g_warning ("Country combo not set, so nothing done with variant combo.");
		callbacks_shield_set (FALSE);
		return;
	}

	/* Set the original variants for the selected country */	
	if (! g_str_equal (country_txt, KEYB_CUSTOM))
	{
		gchar *current;

		n = 0;
		for (i = 0; i < layouts.n_orig; i++)
		{
			if (g_str_equal (layouts.orig[i].country, country_txt))
			{
				gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), layouts.orig[i].variant);
				n++;
			}
		}

		current = keyb_get_variant (keyb.name);
		for (i = 0; i < n; i++)
		{
			gchar *variant;

			gtk_combo_box_set_active (cmb, i);
			variant = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));
			if (g_str_equal (variant, current))
			{
				g_free (variant);
				break;
			}
			g_free (variant);
		}

		if (i == n)
		{
			if (n > 0)
				gtk_combo_box_set_active (cmb, 0);
			else
				gtk_combo_box_set_active (cmb, -1);
		}

		if (n > 1)
			gtk_widget_set_sensitive (get_wg (cmb_variant), TRUE);
		else
			gtk_widget_set_sensitive (get_wg (cmb_variant), FALSE);

		g_free (current);

	}
	/* Set custom layouts in the variant combo */
	else
	{
		n = 0;
		if (g_str_equal (cmb_variant, "combobox_kbd_variant"))
		{
			gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), KEYB_EDIT);
			n++;
		}
		for (i = 0; i < layouts.n_cust; i++)
		{
			gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), layouts.cust[i].name);
			n++;
		}

		for (i = 0; i < n; i++)
		{
			gchar *variant;

			gtk_combo_box_set_active (cmb, i);
			variant = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));
			if (g_str_equal (variant, keyb.name))
			{
				g_free (variant);
				break;
			}
			g_free (variant);
		}

		if (i == n)
		{
			if (n > 1)
				gtk_combo_box_set_active (cmb, 1);
			else if (! gtk_widget_get_visible (get_wg ("window_keyboard")))
			{
				gtk_combo_box_set_active (cmb, 0);
				keyb_mode_edit ();
			}
		}

		if (layouts.n_cust > 0)
			gtk_widget_set_sensitive (get_wg (cmb_variant), TRUE);
		else
			gtk_widget_set_sensitive (get_wg (cmb_variant), FALSE);
	}
	g_free (country_txt);

	keyb_update_from_variant (cmb_country, cmb_variant);

	callbacks_shield_set (FALSE);
}

void
keyb_set_combo_kbd (gchar *cmb_country, gchar *cmb_variant)
{
	static gboolean init = FALSE;
	gchar *tmp;
	gint i, j;
	GtkComboBox *cmb;

	callbacks_shield_set (TRUE);
	
	if (! main_preferences_exist ("tutor", "keyboard"))
		main_preferences_set_string ("tutor", "keyboard", trans_get_default_keyboard ());

	if (init == FALSE)
	{ 
		tmp = main_preferences_get_string ("tutor", "keyboard");
		if (tmp == NULL)
			g_error ("Unexpected keyboard layout, NULL");
		keyb_init_name (tmp);
		keyb_set_chars ();
		init = TRUE;
		g_free (tmp);
	}

	keyb_set_keyboard_layouts (); // if already initialized, this sets only the custom layouts

	cmb = GTK_COMBO_BOX (get_wg (cmb_country));
	gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (cmb), 0);
	keyb.cmb_n = 0;
	for (i = 0; i < layouts.n_orig; i++)
	{
		j = i - 1;
		while (j >= 0)
		{
			if (g_str_equal (layouts.orig[i].country, layouts.orig[j].country))
				break;
			j--;
		}
		if (j < 0)
		{
			gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), layouts.orig[i].country);
			keyb.cmb_n++;
		}
	}
	gtk_combo_box_text_prepend_text (GTK_COMBO_BOX_TEXT (cmb), KEYB_CUSTOM);
	keyb.cmb_n++;

	keyb_update_combos (cmb_country, cmb_variant);

	callbacks_shield_set (FALSE);
}

void
keyb_update_combos (gchar *cmb_country, gchar *cmb_variant)
{
	gint i;
	GtkComboBox *cmb;

	callbacks_shield_set (TRUE);

	cmb = GTK_COMBO_BOX (get_wg (cmb_country));

	for (i = 0; i < layouts.n_orig; i++)
	{
		if (g_str_equal (keyb.name, layouts.orig[i].name))
			break;
	}
	if (i < layouts.n_orig)
	{
		gchar *country;
		gchar *current;

		/* Set original */
		for (i = 1; i < keyb.cmb_n; i++)
		{

			gtk_combo_box_set_active (cmb, i);
			country = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));
			current = keyb_get_country (keyb.name);
			if (g_str_equal (country, current))
			{
				g_free (country);
				g_free (current);
				break;
			}
			g_free (country);
			g_free (current);
		}
		if (i == keyb.cmb_n)
			gtk_combo_box_set_active (cmb, 0);
	}
	else 	/* Set custom */
		gtk_combo_box_set_active (cmb, 0);

	keyb_set_combo_kbd_variant (cmb_country, cmb_variant);

	callbacks_shield_set (FALSE);
}

void
keyb_intro_step_next ()
{
	if (keyb.intro_step < 6)
		keyb_intro_step (++keyb.intro_step);
}

void
keyb_intro_step_previous ()
{
	if (keyb.intro_step > 0)
		keyb_intro_step (--keyb.intro_step);
}

void
keyb_intro_step (gint step)
{
	gchar *intro00;
	GtkLabel *tit;
	GtkLabel *tx1;
	GtkLabel *tx2;
	GtkTextBuffer *buffer;
	GtkWidget *wg;

	static gchar *intro01 = NULL;
	static gchar *intro02 = NULL;
	static gchar *intro03 = NULL;
	static gchar *intro04 = NULL;
	static gchar *intro05 = NULL;
	static gchar *intro06 = NULL;
	static gchar *intro07 = NULL;
	static gchar *intro08 = NULL;
	static gchar *intro09 = NULL;
	static gchar *intro10 = NULL;

	if (intro01 == NULL)
	{
		intro01 = g_strdup (_("Correct positioning of the hands and fingers is very important to efficient typing. "
	"You will learn faster and type better if you follow the next recommendations."));
		intro02 = g_strdup (_(
	"The index-finger tips rest over each of the two keys which have a small raised mark, "
	"in the center of the keyboard."));
		intro03 = g_strdup (_(
	"These marks function as 'tactile hooks' for your fingers to remain at the correct position. "
	"This way, with a little experience, you will not need to look at the keyboard to see if your "
	"fingers are properly positioned."));
		intro04 = g_strdup (_(
	"The tips of the other fingers lie naturally beside the index ones, "
	"over the keys on the same row of the keyboard."));
		intro05 = g_strdup (_(
	"The outside edges of your thumbs rest over the space bar."));
		intro06 = g_strdup (_(
	"The part of the hands closest to the wrist (the base) rest over the table, "
	"outside the keyboard. Without this kind of support the arms would quickly tire."));
		intro07 = g_strdup (_(
	"This is referred to as the home position for the hands. "
	"From it the fingers move all over the keyboard, "
	"reaching all the keys as naturally and quickly as possible. "
	"To reach this goal one uses a specific relation between each key and finger. "
	"This relation will be learned gradually as you complete the basic course."));
		intro08 = g_strdup (_(
	"When learning the relation between fingers and keys, "
	"it is very important that you only move the finger which must press the key "
	"and allow all other fingers to remain in the home position."));
		intro09 = g_strdup (_(
	"After memorizing this relationship, you can relax the previous rule some, "
	"so that you can attain greater speed while typing."));
		intro10 = g_strdup (_(
	"You should be prepared to start training with the basic course. "
	"It will take effort and patience to be successful as a typist. "
	"We trust you have these and look forward to your success!"));
	}

	tit = GTK_LABEL (get_wg ("label_keyboard_title"));
	tx1 = GTK_LABEL (get_wg ("label_keyboard_text_1"));
	tx2 = GTK_LABEL (get_wg ("label_keyboard_text_2"));

	buffer = gtk_text_view_get_buffer (GTK_TEXT_VIEW (get_wg ("textview_keyboard")));

	intro00 = g_strdup_printf (_("Step %i"), step);
	switch (step)
	{
	case 0:	/* Recommendations */
		gtk_label_set_text (tx1, intro01);
		gtk_label_set_text (tx2, "");
		gtk_label_set_text (tit, _("To position the hands"));
        	gtk_text_buffer_set_text (buffer, intro01, -1);
		gtk_widget_grab_focus (get_wg ("button_keyboard_next"));
		keyb_set_sensitive (TRUE);
		hints_demo_fingers (0);
		break;
	case 1: /* Index fingers */
		gtk_label_set_text (tit, intro00);
		gtk_label_set_text (tx1, intro02);
		gtk_label_set_text (tx2, intro03);
        	gtk_text_buffer_set_text (buffer, intro02, -1);
        	gtk_text_buffer_insert_at_cursor (buffer, "\n", -1);
        	gtk_text_buffer_insert_at_cursor (buffer, intro03, -1);
		keyb_set_sensitive (FALSE);
		gtk_widget_set_sensitive (keyb.but[2][3], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][6], TRUE);
		hints_demo_fingers (1000);
		break;
	case 2: /* Fingers beside index */
		gtk_label_set_text (tit, intro00);
		gtk_label_set_text (tx1, intro04);
		gtk_label_set_text (tx2, "");
        	gtk_text_buffer_set_text (buffer, intro04, -1);
		keyb_set_sensitive (FALSE);
		gtk_widget_set_sensitive (keyb.but[2][0], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][1], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][2], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][7], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][8], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][9], TRUE);
		hints_demo_fingers (1000/3);
		break;
	case 3: /* Thumbs and wrists */
		gtk_label_set_text (tit, intro00);
		gtk_label_set_text (tx1, intro05);
		gtk_label_set_text (tx2, intro06);
        	gtk_text_buffer_set_text (buffer, intro05, -1);
        	gtk_text_buffer_insert_at_cursor (buffer, "\n", -1);
        	gtk_text_buffer_insert_at_cursor (buffer, intro06, -1);
		keyb_set_sensitive (FALSE);
		gtk_widget_set_sensitive (get_wg ("but_space"), TRUE);
		hints_demo_fingers (0);
		gtk_widget_grab_focus (get_wg ("but_space"));
		hints_update_from_button (GTK_BUTTON (get_wg ("but_space")));
		break;
	case 4: /* The home position */
		gtk_label_set_text (tit, intro00);
		gtk_label_set_text (tx1, intro07);
		gtk_label_set_text (tx2, "");
        	gtk_text_buffer_set_text (buffer, intro07, -1);
		keyb_set_sensitive (FALSE);
		gtk_widget_set_sensitive (keyb.but[2][0], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][1], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][2], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][3], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][6], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][7], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][8], TRUE);
		gtk_widget_set_sensitive (keyb.but[2][9], TRUE);
		hints_demo_fingers (1000/4);
		break;
	case 5: /* Reaching keys */
		gtk_label_set_text (tit, intro00);
		gtk_label_set_text (tx1, intro08);
		gtk_label_set_text (tx2, intro09);
        	gtk_text_buffer_set_text (buffer, intro08, -1);
        	gtk_text_buffer_insert_at_cursor (buffer, "\n", -1);
        	gtk_text_buffer_insert_at_cursor (buffer, intro09, -1);
		keyb_set_sensitive (TRUE);
		gtk_widget_set_sensitive (keyb.but[2][0], FALSE);
		gtk_widget_set_sensitive (keyb.but[2][1], FALSE);
		gtk_widget_set_sensitive (keyb.but[2][2], FALSE);
		gtk_widget_set_sensitive (keyb.but[2][3], FALSE);
		gtk_widget_set_sensitive (keyb.but[2][6], FALSE);
		gtk_widget_set_sensitive (keyb.but[2][7], FALSE);
		gtk_widget_set_sensitive (keyb.but[2][8], FALSE);
		gtk_widget_set_sensitive (keyb.but[2][9], FALSE);
		gtk_widget_set_sensitive (get_wg ("but_space"), FALSE);
		gtk_widget_set_sensitive (get_wg ("toggle_shift1"), FALSE);
		gtk_widget_set_sensitive (get_wg ("toggle_shift2"), FALSE);
		hints_demo_fingers (1000/5);
		break;
	case 6: /* Final words */
		gtk_label_set_text (tx1, intro10);
		gtk_label_set_text (tx2, "");
		gtk_label_set_text (tit, _("Go ahead!"));
        	gtk_text_buffer_set_text (buffer, intro10, -1);
		gtk_widget_grab_focus (get_wg ("button_keyboard_close"));
		keyb_set_sensitive (TRUE);
		hints_demo_fingers (0);
		break;
	default:
		gtk_label_set_text (tit, _("Relation between fingers and keys"));
		gtk_label_set_text (tx1, _("Click on any key to see which finger you must use:"));
		gtk_label_set_text (tx2, "");
		keyb_set_sensitive (TRUE);
		hints_demo_fingers (0);
	}
	g_free (intro00);

	/* Blind people want no fancy autonomous buttons jumping around */
	wg = get_wg ("checkbutton_speech");
	if (gtk_widget_get_visible (wg) && gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wg)))
		hints_demo_fingers (0);

	if (step == 0)
		gtk_widget_set_sensitive (get_wg ("button_keyboard_previous"), FALSE);
	else
		gtk_widget_set_sensitive (get_wg ("button_keyboard_previous"), TRUE);

	if (step == 6)
		gtk_widget_set_sensitive (get_wg ("button_keyboard_next"), FALSE);
	else
		gtk_widget_set_sensitive (get_wg ("button_keyboard_next"), TRUE);

	if (step >= 0 && step <= 6)
		keyb.intro_step = step;
	else
		keyb.intro_step = 0;
}

gchar *
keyb_mode_get_name ()
{
	gchar *country;
	gchar *variant;
	static gchar *kbname = NULL;

	if (kbname != NULL)
		g_free (kbname);

	country = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (get_wg ("combobox_kbd_country")));
	variant = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (get_wg ("combobox_kbd_variant")));
	kbname = g_strdup_printf ("%s - %s", country, variant);
	g_free (country);
	g_free (variant);

	return (kbname);
}

void
keyb_mode_intro ()
{
	gchar *tit;

	keyb_update_virtual_layout ();
	gtk_widget_hide (get_wg ("window_hints"));
	keyb_edit_none ();

	tit = g_strdup_printf ("%s - %s: %s", _("Introduction"), _("Keyboard"), keyb_mode_get_name ());
	gtk_window_set_title (get_win ("window_keyboard"), tit);
	g_free (tit);

	gtk_window_set_resizable (get_win ("window_keyboard"), TRUE);
	gtk_widget_set_size_request (get_wg ("window_keyboard"), -1, 420);
	gtk_widget_hide (get_wg ("label_keyboard_text_1"));
	gtk_widget_hide (get_wg ("button_kb_save"));
	gtk_widget_hide (get_wg ("button_keyboard_hands"));
	gtk_widget_hide (get_wg ("button_keyboard_cancel"));
	gtk_widget_hide (get_wg ("hbox_keyboard_selector"));
	gtk_widget_hide (get_wg ("hbox_keyboard_saveas"));
	gtk_widget_show (get_wg ("label_keyboard_spacer"));
	gtk_widget_show (get_wg ("scrolledwindow_keyboard"));
	gtk_widget_show (get_wg ("button_keyboard_close"));
	gtk_widget_show (get_wg ("button_keyboard_previous"));
	gtk_widget_show (get_wg ("button_keyboard_next"));
	gtk_widget_show (get_wg ("hbox_keyboard_hints"));

	hints_set_tips ();
	keyb_intro_step (0);

	gtk_widget_show (get_wg ("window_keyboard"));
}

void
keyb_mode_hint ()
{
	gchar *tit;

	keyb_update_virtual_layout ();
	gtk_widget_hide (get_wg ("window_hints"));
	keyb_edit_none ();

	tit = g_strdup_printf ("%s: %s", _("Keyboard"), keyb_mode_get_name ());
	gtk_window_set_title (get_win ("window_keyboard"), tit);
	g_free (tit);

	gtk_window_set_resizable (get_win ("window_keyboard"), FALSE);
	gtk_widget_set_size_request (get_wg ("window_keyboard"), -1, -1);
	gtk_widget_hide (get_wg ("label_keyboard_spacer"));
	gtk_widget_hide (get_wg ("scrolledwindow_keyboard"));
	gtk_widget_hide (get_wg ("button_kb_save"));
	gtk_widget_hide (get_wg ("button_keyboard_cancel"));
	gtk_widget_hide (get_wg ("button_keyboard_previous"));
	gtk_widget_hide (get_wg ("button_keyboard_next"));
	gtk_widget_hide (get_wg ("hbox_keyboard_selector"));
	gtk_widget_hide (get_wg ("hbox_keyboard_saveas"));
	gtk_widget_show (get_wg ("label_keyboard_text_1"));
	gtk_widget_show (get_wg ("button_keyboard_hands"));
	gtk_widget_show (get_wg ("button_keyboard_close"));
	gtk_widget_show (get_wg ("hbox_keyboard_hints"));

	hints_set_tips ();
	keyb_intro_step (-1);
	gtk_widget_grab_focus (get_wg ("button_keyboard_close"));

	gtk_widget_show (get_wg ("window_keyboard"));
}

void
keyb_mode_edit ()
{
	gchar *tmp;

	/* Save the current name as 'name_last' */
	tmp = g_strdup (keyb.name);
	keyb_set_name (tmp);
	g_free (tmp);

	keyb_set_modified_status (FALSE);
	if (layouts.n_cust == 0)
		gtk_widget_set_sensitive (get_wg ("button_kb_remove"), FALSE);
	else
		gtk_widget_set_sensitive (get_wg ("button_kb_remove"), TRUE);

	keyb_update_combos ("combobox_keyboard_country", "combobox_keyboard_variant");

	keyb_update_virtual_layout ();
	keyb_edit_none ();

	gtk_window_set_title (get_win ("window_keyboard"), _("Create or modify a custom keyboard layout"));
	gtk_window_set_resizable (get_win ("window_keyboard"), FALSE);
	gtk_widget_set_size_request (get_wg ("window_keyboard"), -1, -1);
	gtk_widget_hide (get_wg ("button_keyboard_hands"));
	gtk_widget_hide (get_wg ("button_keyboard_close"));
	gtk_widget_hide (get_wg ("button_keyboard_previous"));
	gtk_widget_hide (get_wg ("button_keyboard_next"));
	gtk_widget_hide (get_wg ("hbox_keyboard_hints"));
	gtk_widget_show (get_wg ("button_keyboard_cancel"));
	gtk_widget_show (get_wg ("hbox_keyboard_selector"));
	gtk_widget_show (get_wg ("hbox_keyboard_saveas"));
	gtk_widget_show (get_wg ("button_kb_save"));

	keyb_set_sensitive (TRUE);
	gtk_widget_set_sensitive (get_wg ("but_space"), FALSE);

	hints_set_tips ();
	gtk_widget_grab_focus (get_wg ("button_keyboard_cancel"));

	gtk_widget_show (get_wg ("window_keyboard"));
}

void
keyb_set_sensitive (gboolean state)
{
	gint i, j;
	gint j_max;

	for (i = 0; i < 4; i++)
	{
		j_max = KEY_LINE_LEN - (i == 0 ? 1 : (i == 1 ? 2 : 3));
		for (j = 0; j < j_max; j++)
			gtk_widget_set_sensitive (keyb.but[i][j], state);
	}
	gtk_widget_set_sensitive (get_wg ("but_space"), state);
	gtk_widget_set_sensitive (get_wg ("toggle_shift1"), state);
	gtk_widget_set_sensitive (get_wg ("toggle_shift2"), state);
}

gboolean
keyb_button_match (GtkButton * button)
{
	gint i, j;
	gint j_max;

	for (i = 0; i < 4; i++)
	{
		j_max = KEY_LINE_LEN - (i == 0 ? 1 : (i == 1 ? 2 : 3));
		for (j = 0; j < j_max; j++)
		{
			if (keyb.but[i][j] == GTK_WIDGET (button))
			{
				keyb.pos.i = i;
				keyb.pos.j = j;
				return TRUE;
			}
		}
	}
	return FALSE;
}

/**********************************************************************
 * Moves the entry widget to get a real key pressed and
 * writes the name of the virtual one.
 */
void
keyb_edit_button (GtkButton * button)
{
	if (! keyb_button_match (button))
		return;

	callbacks_shield_set (TRUE);

	gtk_widget_grab_focus (GTK_WIDGET (button));
	gtk_entry_set_text (GTK_ENTRY (keyb.entry),
		       	gtk_label_get_text (GTK_LABEL (keyb.lab[keyb.pos.i][keyb.pos.j])));
	gtk_fixed_move (GTK_FIXED (get_wg ("fixed_keyboard")), keyb.entry,
		       	2 + x0[keyb.pos.i] + KEY_DX * keyb.pos.j,
		       	2 + KEY_DY * keyb.pos.i);
	if (keyb.pos.i == 1 && keyb.pos.j == 12)
		gtk_widget_set_size_request (keyb.entry, 49, 28);
	else
		gtk_widget_set_size_request (keyb.entry, 28, 28);
	gtk_editable_select_region (GTK_EDITABLE (keyb.entry), 0, 1);
	gtk_widget_show (keyb.entry);
	gtk_widget_grab_focus (keyb.entry);

	callbacks_shield_set (FALSE);
}

void
keyb_edit_none (void)
{
	gtk_widget_hide (keyb.entry);
}

gboolean
keyb_edit_next (void)
{
	keyb.pos.j++;
	
	switch (keyb.pos.i)
	{
	case 0:
		if (keyb.pos.j > 13)
		{
			keyb.pos.i++;
			keyb.pos.j = 0;
		}
		break;
	case 1:
		if (keyb.pos.j > 12)
		{
			keyb.pos.i++;
			keyb.pos.j = 0;
		}
		break;
	default:
		if (keyb.pos.j > 11)
		{
			keyb.pos.i++;
			keyb.pos.j = 0;
		}
	}

	if (keyb.pos.i > 3)
		keyb.pos.i = 0;

	gtk_widget_hide (keyb.entry);
	if (gtk_widget_get_sensitive (keyb.but[keyb.pos.i][keyb.pos.j]))
	{
		gtk_widget_grab_focus (keyb.but[keyb.pos.i][keyb.pos.j]);
		return TRUE;
	}
	else
		return FALSE;
}

/**********************************************************************
 * Apply the key pressed to the virtual keyboard
 * and to the upper or lower character sets
 */
void
keyb_change_key (gunichar real_key)
{
	gint key_lin, key_col;
	gunichar str_char;
	gchar tmp_utf8[7];
	gboolean tog_state;
	GtkWidget *wg;

	key_lin = keyb.pos.i;
	key_col = keyb.pos.j;

	str_char = g_unichar_toupper (real_key);
	tmp_utf8[g_unichar_to_utf8 (str_char, tmp_utf8)] = '\0';
	gtk_label_set_text (GTK_LABEL (keyb.lab[key_lin][key_col]), tmp_utf8);

	wg = get_wg ("toggle_shift1");
	tog_state = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wg));
	if (tog_state)
	{
		keyb.upchars[key_lin][key_col] = str_char;
		if (str_char >= L'A' && str_char <= L'Z')
			keyb.lochars[key_lin][key_col] = g_unichar_tolower (str_char);
	}
	else
	{
		keyb.lochars[key_lin][key_col] = g_unichar_tolower (str_char);
		if (str_char >= L'A' && str_char <= L'Z')
			keyb.upchars[key_lin][key_col] = str_char;
	}

	keyb_set_modified_status (TRUE);
	gtk_widget_set_sensitive (get_wg ("button_kb_save"), TRUE);
	gtk_widget_set_sensitive (get_wg ("combobox_keyboard_country"), FALSE);
	gtk_widget_set_sensitive (get_wg ("combobox_keyboard_variant"), FALSE);
}

/*******************************************************************************
 * Get an utf8 string for the par symbol
 */
gchar *
keyb_get_utf8_paragraph_symbol ()
{
	static gchar parsym[7];
	static gboolean is_initialized = FALSE;

	if (is_initialized == FALSE)
	{
		is_initialized = TRUE;
		parsym[g_unichar_to_utf8 (UPSYM, parsym)] = '\0';
	}
	return (parsym);
}

/*******************************************************************************
 * Initialize the hints mapping array
 */
static gchar hints[4][KEY_LINE_LEN + 1];
static gboolean hints_is_initialized = FALSE;

void
hints_init ()
{
	gint i;
	gchar *tmp_name;
	gchar *tmp;
	FILE *fh;

	if (hints_is_initialized == TRUE)
		return;

	tmp_name = g_build_filename ("/etc/klavaro/fingers_position.txt", NULL);
	if (!g_file_test (tmp_name, G_FILE_TEST_IS_REGULAR))
	{
		g_free (tmp_name);
		tmp_name = g_build_filename (main_path_user (), "fingers_position.txt", NULL);
	}
	if (!g_file_test (tmp_name, G_FILE_TEST_IS_REGULAR))
	{
		g_free (tmp_name);
		tmp_name = g_build_filename (main_path_data (), "fingers_position.txt", NULL);
	}
	fh = (FILE *) g_fopen (tmp_name, "r");
	if (fh)
	{
		hints_is_initialized = TRUE;
		for (i = 0; i < 4; i++)
			tmp = fgets (hints[i], KEY_LINE_LEN + 1, fh);
		fclose (fh);
		hints_set_tips ();
		hints_set_colors ();
	}
	else
		g_warning ("couldn't open the file:\n %s", tmp_name);
	g_free (tmp_name);
}

gchar *
hints_string_from_charcode (gchar charcode)
{
	gchar *fingerhint = NULL;

	switch (charcode)
	{
	case '1':
		fingerhint = g_strdup (_("small finger"));
		break;
	case '2':
		fingerhint = g_strdup (_("ring finger"));
		break;
	case '3':
		fingerhint = g_strdup (_("middle finger"));
		break;
	case '4':
		fingerhint = g_strdup (_("index finger"));
		break;
	case '5':
		fingerhint = g_strdup (_("thumbs"));
		break;
	case '6':
		fingerhint = g_strdup (_("index finger"));
		break;
	case '7':
		fingerhint = g_strdup (_("middle finger"));
		break;
	case '8':
		fingerhint = g_strdup (_("ring finger"));
		break;
	case '9':
		fingerhint = g_strdup (_("small finger"));
		break;
	default:
		fingerhint = g_strdup ("???");
	}
	return (fingerhint);
}

gchar *
hints_color_from_charcode (gchar charcode)
{
	static gchar *hlp = NULL;

	g_free (hlp);

	switch (charcode)
	{
	case '1':
		if (main_preferences_exist ("colors", "key_1"))
			hlp = main_preferences_get_string ("colors", "key_1");
		else
			hlp = g_strdup (KEYB_BLUE);
		break;
	case '2':
		if (main_preferences_exist ("colors", "key_2"))
			hlp = main_preferences_get_string ("colors", "key_2");
		else
			hlp = g_strdup (KEYB_RED);
		break;
	case '3':
		if (main_preferences_exist ("colors", "key_3"))
			hlp = main_preferences_get_string ("colors", "key_3");
		else
			hlp = g_strdup (KEYB_GREEN);
		break;
	case '4':
		if (main_preferences_exist ("colors", "key_4"))
			hlp = main_preferences_get_string ("colors", "key_4");
		else
			hlp = g_strdup (KEYB_YELLOW);
		break;
	case '5':
		if (main_preferences_exist ("colors", "key_5"))
			hlp = main_preferences_get_string ("colors", "key_5");
		else
			hlp = g_strdup (KEYB_PURPLE);
		break;
	case '6':
		if (main_preferences_exist ("colors", "key_6"))
			hlp = main_preferences_get_string ("colors", "key_6");
		else
			hlp = g_strdup (KEYB_ORANGE);
		break;
	case '7':
		if (main_preferences_exist ("colors", "key_7"))
			hlp = main_preferences_get_string ("colors", "key_7");
		else
			hlp = g_strdup (KEYB_GREEN);
		break;
	case '8':
		if (main_preferences_exist ("colors", "key_8"))
			hlp = main_preferences_get_string ("colors", "key_8");
		else
			hlp = g_strdup (KEYB_RED);
		break;
	case '9':
		if (main_preferences_exist ("colors", "key_9"))
			hlp = main_preferences_get_string ("colors", "key_9");
		else
			hlp = g_strdup (KEYB_BLUE);
		break;
	default:
		hlp = g_strdup ("#AFAFAF");
	}
	return hlp;
}

void
hints_set_tips ()
{
	static gchar *editme = NULL;
	gint i, j;
	gint j_max;
	gchar *tmp;

	if (editme == NULL)
	       editme = g_strdup (_("Press and edit me"));

	if (hints_is_initialized == FALSE)
	{
		g_warning ("Not able to set keyboard tips without initializing the hints");
		return;
	}

	for (i = 0; i < 4; i++)
	{
		j_max = KEY_LINE_LEN - (i == 0 ? 1 : (i == 1 ? 2 : 3));
		for (j = 0; j < j_max; j++)
		{
			tmp = hints_string_from_charcode (hints[i][j]);
			if (! gtk_widget_get_visible (get_wg ("hbox_keyboard_hints")))
				gtk_widget_set_tooltip_text (keyb.but[i][j], editme);
			else
				gtk_widget_set_tooltip_text (keyb.but[i][j], tmp);
			g_free (tmp);
		}
	}
}

void
hints_set_colors ()
{
	gint i, j;
	gint j_max;
	GdkRGBA color;

	if (hints_is_initialized == FALSE)
	{
		g_warning ("Not able to set keyboard colors without initializing the hints");
		return;
	}

	for (i = 0; i < 4; i++)
	{
		j_max = KEY_LINE_LEN - (i == 0 ? 1 : (i == 1 ? 2 : 3));
		for (j = 0; j < j_max; j++)
		{
			gdk_rgba_parse (&color, hints_color_from_charcode (hints[i][j]));
			gtk_widget_override_background_color (keyb.but[i][j], GTK_STATE_FLAG_NORMAL, &color);
		}
	}
}

/* Update the image of the window_keyboard
 * Maps the button to the file which shows the finger associated with its key
 */
void
hints_update_from_button (GtkButton *button)
{
	gchar *pix_name;
	gchar ch;

	hints_init (); // if already initialized, do nothing

	if (keyb_button_match (button))
	{
		pix_name = g_strdup ("hands_0.png");
		ch = hints[keyb.pos.i][keyb.pos.j];
		if (ch >= '1' && ch <= '9')
			pix_name[6] = ch;
	}
	else if ( button == GTK_BUTTON (get_wg ("but_space")) )
		pix_name = g_strdup ("hands_5.png");
	else if ( button == GTK_BUTTON (get_wg ("toggle_shift1")) )
		pix_name = g_strdup ("hands_1.png");
	else if ( button == GTK_BUTTON (get_wg ("toggle_shift2")) )
		pix_name = g_strdup ("hands_9.png");
	else
		pix_name = g_strdup ("hands_0.png");

	set_pixmap ("pixmap_hints_fixed", pix_name);
	g_free (pix_name);
}

/* Update the image of the window_hints
 * Maps the character to the file which shows the finger associated with that key
 */
void
hints_update_from_char (gunichar character)
{
	gchar file_name[32];
	gint i, j;

	if (! gtk_widget_get_visible (get_wg ("window_hints")))
		return;

	strcpy (file_name, "hands_0.png");
	if (character == UPSYM)
		strcpy (file_name, "hands_9.png");
	else if (character == L' ')
		strcpy (file_name, "hands_5.png");
	else if (character != 0)
	{
		hints_init (); // if already initialized, do nothing

		for (i = 3; i >= 0; i--)
			for (j = 0; j < 15; j++)
				if (character == keyb.lochars[i][j])
				{
					file_name[6] = hints[i][j];
					set_pixmap ("pixmap_hints", file_name);
					return;
				}

		for (i = 3; i >= 0; i--)
			for (j = 0; j < 15; j++)
				if (character == keyb.upchars[i][j])
				{
					file_name[6] = hints[i][j];
					set_pixmap ("pixmap_hints", file_name);
					return;
				}
		file_name[6] = '0';
	}

	set_pixmap ("pixmap_hints", file_name);
}

gboolean
hints_demo_fingers_move (gpointer data)
{
	static int i = 0;

	if (data)
	{
		keyb.pos.i = 0;
		keyb.pos.j = 0;
	}

	for (i = 0; i < 500; i++)
		if (keyb_edit_next ())
			break;
	return TRUE;
}

void
hints_demo_fingers (guint msec)
{
	static GSource *source = NULL;
	guint id;

	if (source != NULL)
		g_source_destroy (source);
	source = NULL;
	hints_demo_fingers_move (&msec);

	if (msec != 0)
	{
		id = g_timeout_add (msec, hints_demo_fingers_move, NULL);
		source = g_main_context_find_source_by_id (NULL, id);
	}
}

gchar *
hints_finger_name_from_char (gunichar uch)
{
	gint i, j;

	if (uch == UPSYM || uch == L'\n' || uch == L'\r')
		return (hints_string_from_charcode ('9'));
	if (uch == L' ')
		return (hints_string_from_charcode ('5'));
	
	hints_init (); // if already initialized, do nothing

	for (i = 3; i >= 0; i--)
		for (j = 0; j < 15; j++)
			if (uch == keyb.lochars[i][j])
				return (hints_string_from_charcode (hints[i][j]));

	for (i = 3; i >= 0; i--)
		for (j = 0; j < 15; j++)
			if (uch == keyb.upchars[i][j])
				return (hints_string_from_charcode (hints[i][j]));

	return (g_strdup (" "));
}
