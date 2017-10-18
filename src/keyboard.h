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

#define KEYB_AUTO_SAVE "tmp_auto"

#define MAX_KEYBOARDS 200
#define KEY_LINE_LEN (14 + 1)	/* 14 keys + 1 NULL char */

#define UPSYM ((gunichar) 182)
#define URDU_COMMA ((gunichar) 0x060C)
#define URDU_STOP ((gunichar) 0x06D4)
#define DEVANAGARI_STOP ((gunichar) 0x0964)

/* Pretty colors
 */
#define KEYB_GREEN "#aaeebb"
#define KEYB_RED "#eeaaaa"
#define KEYB_BLUE "#bbbbff"
#define KEYB_YELLOW "#eeee88"
#define KEYB_ORANGE "#ffcc77" /* "#ffdd88" */
#define KEYB_PURPLE "#ccaacc"
#define KEYB_BLACK "#000000"

typedef struct _KEYBLAYOUT
{
	gchar *name;
	gchar *country;
	gchar *variant;
} KeybLayout;

/*
 * Interface
 */
gchar *keyb_get_name (void);

gchar * keyb_get_name_last (void);

void keyb_set_name (const gchar * name);

void keyb_init_name (const gchar * name);

gunichar keyb_get_lochars (gint i, gint j);

gunichar keyb_get_upchars (gint i, gint j);

gboolean keyb_get_modified_status (void);

void keyb_set_modified_status (gboolean new_status);

/*
 * Auxiliar
 */
void keyb_create_virtual_keys (void);

void keyb_set_chars (void);

gboolean keyb_is_vowel (gunichar chr);

gboolean keyb_is_diacritic (gunichar chr);

gboolean keyb_is_inset (gunichar chr);

gint keyb_get_vowels (gunichar * vows);

gint keyb_get_consonants (gunichar * consonants);

gint keyb_get_symbols (gunichar * symbols);

gint keyb_get_altnums (gunichar * altnums);

gunichar keyb_unichar_toupper (gunichar uchar);

void keyb_save_new_layout (void);

void keyb_remove_user_layout (void);

void keyb_update_virtual_layout (void);

gchar * keyb_get_country (const gchar *kbd);

gchar * keyb_get_variant (const gchar *kbd);

void keyb_set_keyboard_layouts (void);

void keyb_update_from_variant (gchar *cmb_country, gchar *cmb_variant);

void keyb_set_combo_kbd_variant (gchar *cmb_country, gchar *cmb_variant);

void keyb_set_combo_kbd (gchar *cmb_country, gchar *cmb_variant);

void keyb_update_combos (gchar *cmb_country, gchar *cmb_variant);

void keyb_intro_step_next (void);

void keyb_intro_step_previous (void);

void keyb_intro_step (gint step);

gchar * keyb_mode_get_name (void);

void keyb_mode_intro (void);

void keyb_mode_hint (void);

void keyb_mode_edit (void);

void keyb_set_sensitive (gboolean state);

gboolean keyb_button_match (GtkButton * button);

void keyb_edit_button (GtkButton * button);

void keyb_edit_none (void);

gboolean keyb_edit_next (void);

void keyb_change_key (gunichar real_key);

gchar *keyb_get_utf8_paragraph_symbol (void);

gboolean keyb_force_edit_tab (gpointer data);

/*
 * Hints
 */
void hints_init (void);

gchar * hints_string_from_charcode (gchar charcode);

void hints_set_tips (void);

void hints_set_colors (void);

void hints_get_from_char (gchar *file_name, gunichar character);

void hints_update_from_char (gunichar character);

void hints_update_from_button (GtkButton * button);

gboolean hints_demo_fingers_move (gpointer data);

void hints_demo_fingers (guint msec);

gchar * hints_finger_name_from_char (gunichar uch);
