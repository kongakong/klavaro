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
 * Callbacks for widgets events
 */
#include <string.h>
#include <stdlib.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>
#include <gtkdatabox.h>

#include "auxiliar.h"
#include "main.h"
#include "translation.h"
#include "keyboard.h"
#include "tutor.h"
#include "cursor.h"
#include "plot.h"
#include "basic.h"
#include "velocity.h"
#include "fluidness.h"
#include "accuracy.h"
#include "top10.h"
#include "callbacks.h"

/*
 * Variables
 */
GtkClipboard *clipboard = NULL;
GtkClipboard *clipboard_2 = NULL;

extern gchar *KEYB_CUSTOM;
extern gchar *KEYB_EDIT;
extern gchar *OTHER_DEFAULT;

static gboolean callbacks_shield = FALSE;
static gboolean mesg_drag_drop = FALSE;
static gchar *other_renaming_undo = NULL;

void
callbacks_shield_set (gboolean state)
{
	callbacks_shield = state;
}

/**********************************************************************
 * 1 - Main menu commands
 **********************************************************************/
G_MODULE_EXPORT void
on_button_intro_clicked (GtkButton *button, gpointer user_data)
{
	keyb_mode_intro ();
}

G_MODULE_EXPORT void
on_button_basic_clicked (GtkButton *button, gpointer user_data)
{
	window_save ("main");
	tutor_init (TT_BASIC);
}

G_MODULE_EXPORT void
on_button_adapt_clicked (GtkButton *button, gpointer user_data)
{
	window_save ("main");
	tutor_init (TT_ADAPT);
}

G_MODULE_EXPORT void
on_button_velo_clicked (GtkButton *button, gpointer user_data)
{
	window_save ("main");
	tutor_init (TT_VELO);
}

G_MODULE_EXPORT void
on_button_fluid_clicked (GtkButton *button, gpointer user_data)
{
	window_save ("main");
	tutor_init (TT_FLUID);
}

G_MODULE_EXPORT void
on_button_help_clicked (GtkButton *button, gpointer user_data)
{
	gtk_widget_show (get_wg ("dialog_info"));
}

G_MODULE_EXPORT void
on_button_about_clicked (GtkButton *button, gpointer user_data)
{
	gtk_widget_show (get_wg ("aboutdialog_klavaro"));
}

G_MODULE_EXPORT void
on_aboutdialog_klavaro_close (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}

G_MODULE_EXPORT void
on_aboutdialog_klavaro_response (GtkDialog *dialog, gint response_id, gpointer user_data)
{
	gtk_widget_hide (GTK_WIDGET (dialog));
}

G_MODULE_EXPORT void
on_checkbutton_speech_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	if (callbacks_shield)
		return;
	main_preferences_set_boolean ("interface", "speech",
		       	gtk_toggle_button_get_active (togglebutton));
}

G_MODULE_EXPORT void
on_combobox_language_changed (GtkComboBox *cmb, gpointer user_data)
{
	gchar *tmp;
	gint active;

	if (callbacks_shield)
		return;
	tmp = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));
	trans_change_language (tmp);
	g_free (tmp);

	callbacks_shield_set (TRUE);
	active = gtk_combo_box_get_active (cmb);
	gtk_combo_box_set_active (GTK_COMBO_BOX (get_wg ("combobox_top10_language")), active);
	callbacks_shield_set (FALSE);

	on_combobox_stat_module_changed (GTK_COMBO_BOX (get_wg ("combobox_stat_module")), (gpointer) -1);
}

G_MODULE_EXPORT void
on_combobox_kbd_country_changed (GtkComboBox *cmb, gpointer user_data)
{
	if (callbacks_shield)
		return;
	if (keyb_get_name ()) accur_close ();
	keyb_set_combo_kbd_variant ("combobox_kbd_country", "combobox_kbd_variant");
	accur_init ();
	main_preferences_set_string ("tutor", "keyboard", keyb_get_name ());

	on_combobox_stat_module_changed (GTK_COMBO_BOX (get_wg ("combobox_stat_module")), (gpointer) -1);

	//g_message ("kbd_country_changed: %s", keyb_get_name ());
}

G_MODULE_EXPORT void
on_combobox_kbd_variant_changed (GtkComboBox *cmb, gpointer user_data)
{
	gchar *tmp;

	if (callbacks_shield)
		return;

	tmp = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));
	if (g_str_equal (tmp, KEYB_EDIT))
		keyb_mode_edit ();
	else
	{
		if (keyb_get_name ()) accur_close ();
		keyb_update_from_variant ("combobox_kbd_country", "combobox_kbd_variant");
		accur_init ();
		main_preferences_set_string ("tutor", "keyboard", keyb_get_name ());
	}
	g_free (tmp);

	on_combobox_stat_module_changed (GTK_COMBO_BOX (get_wg ("combobox_stat_module")), (gpointer) -1);

	//g_message ("kbd_variant_changed: %s", keyb_get_name ());
}

G_MODULE_EXPORT void
on_window_main_destroy (GtkWidget *obj, gpointer data)
{
	if (callbacks_shield)
		return;

	window_save ("main");
	main_preferences_set_boolean ("interface", "autostart", FALSE);

	main_window_pass_away ();
}

/**********************************************************************
 * 2 - Little nohelp-dialog
 **********************************************************************/
G_MODULE_EXPORT void
on_button_info_return_clicked (GtkButton *but, gpointer user_data)
{
	gtk_widget_hide (get_wg ("dialog_info"));
}

/**********************************************************************
 * 3 - Tutor window
 **********************************************************************/
#define CB_COLOR_TAG(TAG, FGCOLOR, BGCOLOR) \
	if (main_preferences_exist ("colors", TAG "_fg"))\
		color_fg = main_preferences_get_string ("colors", TAG "_fg");\
	else\
	{\
		color_fg = g_strdup (FGCOLOR);\
		main_preferences_set_string ("colors", TAG "_fg", color_fg);\
	}\
	if (main_preferences_exist ("colors", TAG "_bg"))\
		color_bg = main_preferences_get_string ("colors", TAG "_bg");\
	else\
	{\
		color_bg = g_strdup (BGCOLOR);\
		main_preferences_set_string ("colors", TAG "_bg", color_bg);\
	}\
	gtk_text_buffer_create_tag (buf, TAG,\
		       	"foreground", color_fg,\
		       	"background", color_bg,\
		       	"underline", PANGO_UNDERLINE_NONE, NULL);\
	g_free (color_bg);\
	g_free (color_fg);

G_MODULE_EXPORT void
on_text_tutor_realize (GtkWidget * widget, gpointer user_data)
{
	gboolean beep;
	gchar *tmp;
	gchar *search;
	gchar *tmp_font;
	gchar *color_fg;
	gchar *color_bg;
	gchar *color_main_fg;
	gchar *color_main_bg;
	GtkWidget *wg;
	GtkTextBuffer *buf;
	PangoFontDescription *font_desc;
	GdkRGBA color;

	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (widget));

	/* Set main color for tutor text (char_untouched & CIA)
	 */
	if (main_preferences_exist ("colors", "char_untouched_bg"))
		color_main_bg = main_preferences_get_string ("colors", "char_untouched_bg");
	else
		color_main_bg = g_strdup (TUTOR_CREAM);
	if (main_preferences_exist ("colors", "char_untouched_fg"))
		color_main_fg = main_preferences_get_string ("colors", "char_untouched_fg");
	else
		color_main_fg = g_strdup (TUTOR_BLACK);

	/*
	 * Colors of text on the tutor window (note: ordering here matters, the first tag created is in the bottom!)
	 */
	gtk_text_buffer_create_tag (buf, "char_keep_wrap",
		       	"background", color_main_bg,
		       	"foreground", color_main_fg,
		       	"underline", PANGO_UNDERLINE_NONE, NULL);

	gtk_text_buffer_create_tag (buf, "char_keep_wrap2",
		       	"background", color_main_bg,
		       	"foreground", color_main_fg,
		       	"underline", PANGO_UNDERLINE_NONE, NULL);

	CB_COLOR_TAG ("char_untouched",	TUTOR_BLACK,	TUTOR_CREAM);
	CB_COLOR_TAG ("char_wrong", 	TUTOR_RED,	TUTOR_RED_LITE);
	CB_COLOR_TAG ("char_correct",	TUTOR_GREEN,	TUTOR_CREAM);
	CB_COLOR_TAG ("char_retouched",	TUTOR_BROWN,	TUTOR_GRAY);
	CB_COLOR_TAG ("cursor_blink",	TUTOR_BLACK,	TUTOR_YELLOW);
	CB_COLOR_TAG ("cursor_unblink",	TUTOR_BLACK,	TUTOR_CREAM);
	CB_COLOR_TAG ("text_intro",	TUTOR_BLACK,	TUTOR_WHITE);

	/* Tutor font */
	tmp_font = main_preferences_get_string ("tutor", "lesson_font");
	if (tmp_font == NULL)
	{
		tmp_font = g_strdup (LESSON_FONT);
		main_preferences_set_string ("tutor", "lesson_font", tmp_font);
	}
	gtk_text_buffer_create_tag (buf, "lesson_font", "font", tmp_font, NULL);
	gtk_font_button_set_font_name (GTK_FONT_BUTTON (get_wg ("fontbutton_tutor")), tmp_font);

	/* Change default font throughout the widget */
	font_desc = pango_font_description_from_string (tmp_font);
	g_free (tmp_font);
	gtk_widget_override_font (widget, font_desc);
	pango_font_description_free (font_desc);

	/* Change default background color throughout the widget */
	gdk_rgba_parse (&color, color_main_bg);
	gtk_widget_override_background_color (widget, GTK_STATE_FLAG_INSENSITIVE, &color);

	/* Change default text color throughout the widget */
	gdk_rgba_parse (&color, color_main_fg);
	gtk_widget_override_color (widget, GTK_STATE_FLAG_INSENSITIVE, &color);

	/* Turns on/off the beeps according to last time
	 */
	if (main_preferences_exist ("tutor", "tutor_beep"))
		beep = main_preferences_get_boolean ("tutor", "tutor_beep");
	else
		beep = TRUE;
	wg = get_wg ("togglebutton_tutor_beep");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wg), beep);
	main_preferences_set_boolean ("tutor", "tutor_beep", beep);


	g_free (color_main_bg);
	g_free (color_main_fg);

	tutor_init_goals ();
}

static void
cb_quick_restart ()
{
	if (tutor_get_query () == QUERY_END)
		return;

	tutor_set_query (QUERY_INTRO);
	tutor_process_touch (L'\0');
}

G_MODULE_EXPORT void
on_button_tutor_other_clicked (GtkButton * button, gpointer user_data)
{
	GtkWidget *wg;
	GtkListStore *list;

	gtk_widget_show (get_wg ("popup_other"));

	wg = get_wg ("treeview_other");
	list = GTK_LIST_STORE ( gtk_tree_view_get_model (GTK_TREE_VIEW (wg)) );
	if (tutor_get_type () == TT_VELO)
		tutor_load_list_other (".words", list);
	else if (tutor_get_type () == TT_FLUID)
		tutor_load_list_other (".paragraphs", list);

	g_free (other_renaming_undo);
	other_renaming_undo = g_strdup ("*");
}

G_MODULE_EXPORT void
on_button_tutor_stat_clicked (GtkButton * button, gpointer user_data)
{
	gint aux;

	gtk_combo_box_set_active (GTK_COMBO_BOX (get_wg ("combobox_stat_module")), tutor_get_type ());
	if (tutor_get_type () == TT_BASIC)
	{
		callbacks_shield_set (TRUE);
		aux = basic_get_lesson () - (basic_get_lesson_increased () ? 1 : 0);
		aux += (aux == 0) ? 1 : 0;
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (get_wg ("spinbutton_stat_lesson")), aux);
		callbacks_shield_set (FALSE);
	}

	plot_draw_chart (gtk_combo_box_get_active (GTK_COMBO_BOX (get_wg ("combobox_stat_type"))) + 1);
	gtk_widget_show (get_wg ("window_stat"));
}

G_MODULE_EXPORT void
on_spinbutton_lesson_value_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
	gint tmp_int;

	if (callbacks_shield)
		return;

	if (tutor_get_type () == TT_BASIC)
	{
		basic_set_lesson_increased (FALSE);
		basic_set_lesson (gtk_spin_button_get_value_as_int (spinbutton));
		basic_init_char_set ();
	}
	else if (tutor_get_type () == TT_FLUID)
	{
		tmp_int = gtk_spin_button_get_value_as_int (spinbutton);
		main_preferences_set_int ("tutor", "fluid_paragraphs", tmp_int);
	}
	tutor_set_query (QUERY_INTRO);
	tutor_process_touch (L'\0');
}

G_MODULE_EXPORT void
on_togglebutton_edit_basic_lesson_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	gchar *tmp_name;
	GtkWidget *wg;

	wg = get_wg ("entry_custom_basic_lesson");
	if (gtk_toggle_button_get_active (togglebutton))
	{
		callbacks_shield_set (TRUE);
		gtk_widget_show (wg);
		gtk_widget_grab_focus (wg);

		tmp_name = g_ucs4_to_utf8 (basic_get_char_set (), -1, NULL, NULL, NULL);
		gtk_entry_set_text (GTK_ENTRY (wg), g_strstrip (tmp_name));
		gtk_editable_set_position (GTK_EDITABLE (wg), -1);
		g_free (tmp_name);

		gtk_widget_set_sensitive (get_wg ("spinbutton_lesson"), FALSE);
		gtk_label_set_text (GTK_LABEL (get_wg ("label_heading")), _("Keys:"));
	}
	else
	{
		gtk_widget_hide (wg);

		tmp_name = g_strdup (gtk_entry_get_text (GTK_ENTRY (wg)));
		if (tutor_get_type () == TT_BASIC)
		{
			basic_save_lesson (tmp_name);

			basic_init_char_set ();
			tutor_set_query (QUERY_INTRO);
			tutor_process_touch (L'\0');

			gtk_widget_set_sensitive (get_wg ("spinbutton_lesson"), TRUE);
		}
		g_free (tmp_name);

		callbacks_shield_set (FALSE);
		gtk_widget_grab_focus (get_wg ("entry_mesg"));
	}
}

G_MODULE_EXPORT void
on_togglebutton_toomuch_errors_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	cb_quick_restart ();
}

G_MODULE_EXPORT void
on_entry_custom_basic_lesson_activate (GtkEntry * entry, gpointer user_data)
{
	GtkWidget *wg;

	wg = get_wg ("togglebutton_edit_basic_lesson");
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (wg), FALSE);
}

G_MODULE_EXPORT void
on_button_tutor_top10_clicked (GtkButton * button, gpointer user_data)
{
	GtkComboBox *cmb;

	cmb = GTK_COMBO_BOX (get_wg ("combobox_top10"));

	if (gtk_combo_box_get_active (cmb) == -1)
		gtk_combo_box_set_active (cmb, 0);
			
	top10_message (NULL);

	if (gtk_combo_box_get_active (GTK_COMBO_BOX (get_wg ("combobox_top10"))) == 0)
		top10_show_stats (LOCAL);
	else
		top10_show_stats (GLOBAL);

	gtk_widget_show (get_wg ("window_top10"));
}

G_MODULE_EXPORT void
on_button_tutor_show_keyb_clicked (GtkButton * button, gpointer user_data)
{
	if (gtk_widget_get_visible (get_wg ("window_hints")))
		window_save ("hints");
	keyb_mode_hint ();
}

G_MODULE_EXPORT void
on_fontbutton_tutor_font_set (GtkFontButton * fbut, gpointer user_data)
{
	GtkWidget *wg;
	GtkTextBuffer *buf;
	GtkTextTagTable *ttab;
	GtkTextIter start;
	GtkTextIter end;
	gchar *tmp_font;

	tmp_font = g_strdup (gtk_font_button_get_font_name (fbut));
	if (tmp_font == NULL)
		tmp_font = g_strdup (LESSON_FONT);
	if (strlen (tmp_font) == 0)
		tmp_font = g_strdup (LESSON_FONT);
	main_preferences_set_string ("tutor", "lesson_font", tmp_font);

	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));
	ttab = gtk_text_buffer_get_tag_table (buf);
	gtk_text_buffer_get_bounds (buf, &start, &end);

	gtk_text_tag_table_remove (ttab, gtk_text_tag_table_lookup (ttab, "lesson_font"));
	gtk_text_buffer_create_tag (buf, "lesson_font", "font", tmp_font, NULL);
	gtk_text_buffer_apply_tag_by_name (buf, "lesson_font", &start, &end);

	g_free (tmp_font);
}

G_MODULE_EXPORT void
on_togglebutton_tutor_beep_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean beep;

	beep = gtk_toggle_button_get_active (togglebutton);
	main_preferences_set_boolean ("tutor", "tutor_beep", beep);
}

G_MODULE_EXPORT void
on_togglebutton_tutor_intro_toggled (GtkToggleButton *button, gpointer user_data)
{
	if (callbacks_shield)
		return;

	tutor_set_query (QUERY_INTRO);
	if (gtk_toggle_button_get_active (button))
		tutor_update ();
	else
		tutor_process_touch (UPSYM);
}

G_MODULE_EXPORT void
on_button_tutor_back_clicked (GtkButton *button, gpointer user_data)
{
	window_save ("tutor");
	if (gtk_widget_get_visible (get_wg ("window_hints")))
		window_save ("hints");
	if (gtk_widget_get_visible (get_wg ("window_top10")))
		window_save ("top10");
	if (gtk_widget_get_visible (get_wg ("window_stat")))
		window_save ("stat");
	gtk_widget_hide (get_wg ("window_tutor"));
	gtk_widget_hide (get_wg ("window_hints"));
	gtk_widget_hide (get_wg ("window_top10"));
	gtk_widget_hide (get_wg ("window_stat"));
	gtk_widget_hide (get_wg ("filechooser_tutor"));
	gtk_widget_show (get_wg ("window_main"));
}

G_MODULE_EXPORT void
on_window_tutor_destroy (GtkWidget * object, gpointer user_data)
{
	if (callbacks_shield)
		return;
	window_save ("tutor");
	main_preferences_set_boolean ("interface", "autostart", TRUE);
	main_preferences_set_int ("interface", "exercise", tutor_get_type ());
	callbacks_shield_set (TRUE);
	main_window_pass_away ();
}

G_MODULE_EXPORT void
on_button_tutor_close_clicked (GtkButton *button, gpointer user_data)
{
	on_window_tutor_destroy (NULL, NULL);
}

G_MODULE_EXPORT void
on_button_tutor_restart_clicked (GtkButton * button, gpointer user_data)
{
	cb_quick_restart ();
}

G_MODULE_EXPORT void
on_eventbox_tutor_restart_grab_focus (GtkWidget * widget, gpointer user_data)
{
	cb_quick_restart ();
}

G_MODULE_EXPORT void
on_entry_mesg_icon_release (GtkEntry *entry, GtkEntryIconPosition pos, GdkEvent *event, gpointer user_data)
{
	cb_quick_restart ();
}

/* Managing the keyboard/messages entry of the tutor window
 */

G_MODULE_EXPORT gboolean
on_entry_mesg_focus_out_event (GtkWidget * widget, GdkEventFocus * event, gpointer user_data)
{
	if (callbacks_shield)
		return (FALSE);

	gtk_widget_grab_focus (widget);
	return (FALSE);
}


G_MODULE_EXPORT void
on_entry_mesg_grab_focus (GtkWidget * widget, gpointer user_data)
{
	if (callbacks_shield)
		return;

	gtk_editable_select_region (GTK_EDITABLE (widget), 0, 0);
	gtk_editable_set_position (GTK_EDITABLE (widget), -1);
}


G_MODULE_EXPORT void
on_entry_mesg_activate (GtkEntry * entry, gpointer user_data)
{
	gchar *tmp1;
	gchar *tmp2;

	if (callbacks_shield)
		return;

	tmp1 = g_strdup (gtk_entry_get_text (entry));
	tmp2 = g_strconcat (tmp1, keyb_get_utf8_paragraph_symbol (), NULL);
	callbacks_shield_set (TRUE);
	gtk_entry_set_text (entry, tmp2);
	callbacks_shield_set (FALSE);
	g_free (tmp1);
	g_free (tmp2);

	gtk_editable_set_position (GTK_EDITABLE (entry), -1);

	tutor_process_touch (UPSYM);
}


G_MODULE_EXPORT void
on_entry_mesg_delete_text (GtkEditable * editable, gint start_pos, gint end_pos, gpointer user_data)
{
	if (callbacks_shield)
		return;

	if (end_pos - start_pos == 1)
		tutor_process_touch (L'\b');
	else
		tutor_process_touch (L'\t');
}

G_MODULE_EXPORT void
on_entry_mesg_insert_text (GtkEditable * editable,
			   gchar * new_text, gint new_text_length, gpointer position,
			   gpointer user_data)
{
	gchar *test;
	gchar *text;

	if (callbacks_shield)
		return;

	if (mesg_drag_drop)
	{
		mesg_drag_drop = FALSE;

		callbacks_shield_set (TRUE);
		gtk_editable_delete_text (editable, 0, -1);
		callbacks_shield_set (FALSE);

		if (g_str_has_prefix (new_text, "file://"))
			test = new_text + 7;
		else
			test = new_text;

		if (g_file_test (test, G_FILE_TEST_IS_REGULAR))
			g_file_get_contents (test, &text, NULL, NULL);
		else
			text = g_strdup (new_text);

		if (tutor_get_type () == TT_VELO)
			velo_text_write_to_file (text, TRUE);
		else if (tutor_get_type () == TT_FLUID)
			fluid_text_write_to_file (text);

		g_free (text);
		return;
	}

	tutor_process_touch (g_utf8_get_char_validated (new_text, new_text_length));
}

/* For languages using input method - by Hodong Kim */
G_MODULE_EXPORT void
on_entry_mesg_preedit_changed (GtkEntry * entry,
			       gchar * preedit,
			       gpointer user_data)
{
  gunichar real_char = cursor_get_char ();
  gunichar preedit_char = g_utf8_get_char (preedit);

  if (real_char == preedit_char)
  {
    /* This trick sends 'focus change' to GtkEntry.
       gtk_entry_focus_in() sets priv->need_im_reset = TRUE,
       therefore gtk_entry_reset_im_context always executes gtk_im_context_reset,
       as a result, pre-edit text is committed
       */
    GdkEvent *event = gdk_event_new (GDK_FOCUS_CHANGE);

    event->focus_change.type = GDK_FOCUS_CHANGE;
    event->focus_change.in = TRUE;
    event->focus_change.window = gtk_widget_get_window (GTK_WIDGET (entry));
    if (event->focus_change.window != NULL)
      g_object_ref (event->focus_change.window);

    gtk_widget_send_focus_change (GTK_WIDGET (entry), event);
    gdk_event_free (event);

    gtk_entry_reset_im_context (entry);
  }
}

/* Tutor drag and drop handling
 */
G_MODULE_EXPORT gboolean
on_entry_mesg_drag_drop (GtkWidget * widget, GdkDragContext * drag_context,
			 gint x, gint y, guint time, gpointer user_data)
{
	callbacks_shield_set (TRUE);
	gtk_entry_set_text (GTK_ENTRY (widget), "");
	callbacks_shield_set (FALSE);
	mesg_drag_drop = TRUE;
	return FALSE;
}

/**********************************************************************
 * 4 - Top 10 management
 **********************************************************************/
G_MODULE_EXPORT void
on_combobox_top10_changed (GtkComboBox *cmb, gpointer user_data)
{
	if (callbacks_shield)
		return;

	top10_message (NULL);

	if (gtk_combo_box_get_active (cmb) == 0)
		top10_show_stats (LOCAL);
	else
		top10_show_stats (GLOBAL);
}

G_MODULE_EXPORT void
on_combobox_top10_language_changed (GtkComboBox *cmb, gpointer user_data)
{
	gchar *tmp;
	gint active;

	if (callbacks_shield)
		return;
	tmp = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));
	trans_change_language (tmp);
	g_free (tmp);

	callbacks_shield_set (TRUE);
	active = gtk_combo_box_get_active (cmb);
	gtk_combo_box_set_active (GTK_COMBO_BOX (get_wg ("combobox_language")), active);
	callbacks_shield_set (FALSE);

	if (tutor_get_type () == TT_FLUID)
		tutor_init (tutor_get_type ());

	on_combobox_top10_changed (GTK_COMBO_BOX (get_wg ("combobox_top10_language")), NULL);
}

G_MODULE_EXPORT void
on_button_top10_go_www_clicked         (GtkButton       *button,
                                        gpointer         user_data)
{
	gchar *url;
	url = gtk_widget_get_tooltip_text (get_wg ("button_top10_go_www"));
	gtk_show_uri (NULL, url, GDK_CURRENT_TIME, NULL);
	g_free (url);
}

G_MODULE_EXPORT void
on_button_top10_publish_clicked (GtkButton * button, gpointer user_data)
{
	GtkWidget *wg;

	top10_message (NULL);

	main_preferences_save ();
	wg = get_wg ("image_top10_publish");
	gtk_image_set_from_icon_name (GTK_IMAGE (wg), "gtk-yes", GTK_ICON_SIZE_BUTTON);

	top10_message (_("Connecting..."));

	g_idle_add ((GSourceFunc) top10_global_publish, NULL);
}

G_MODULE_EXPORT void
on_button_top10_update_clicked (GtkButton * button, gpointer user_data)
{
	GtkWidget *wg;

	top10_message (NULL);

	main_preferences_save ();
	wg = get_wg ("image_top10_update");
	gtk_image_set_from_icon_name (GTK_IMAGE (wg), "gtk-yes", GTK_ICON_SIZE_BUTTON);

	top10_message (_("Connecting..."));

	g_idle_add ((GSourceFunc) top10_global_update, NULL);
}

G_MODULE_EXPORT void
on_button_top10_expand_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (GTK_WIDGET (button));
	gtk_widget_show (get_wg ("button_top10_noexpand"));
	gtk_widget_show (get_wg ("scrolledwindow_top10_2"));
	gtk_widget_set_size_request (get_wg ("window_top10"), 630, 400);
}

G_MODULE_EXPORT void
on_button_top10_noexpand_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_hide (get_wg ("scrolledwindow_top10_2"));
	gtk_widget_show (get_wg ("button_top10_expand"));
	gtk_widget_hide (GTK_WIDGET (button));
	gtk_widget_set_size_request (get_wg ("window_top10"), 350, 400);
}

G_MODULE_EXPORT void
on_button_top10_close_clicked (GtkButton * button, gpointer user_data)
{
	top10_message (NULL);
	window_save ("top10");
	gtk_widget_hide (get_wg ("window_top10"));
}

/**********************************************************************
 * 5 - Keyboard window
 **********************************************************************/

G_MODULE_EXPORT void
on_combobox_keyboard_country_changed (GtkComboBox *cmb, gpointer user_data)
{
	gchar *tmp;

	if (callbacks_shield)
		return;

	keyb_set_combo_kbd_variant ("combobox_keyboard_country", "combobox_keyboard_variant");
	tmp = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (cmb));
	if (g_str_equal (tmp, KEYB_CUSTOM) &&
		gtk_combo_box_get_active ( GTK_COMBO_BOX (get_wg ("combobox_keyboard_variant")) ) > -1 )
	{
			gtk_widget_set_sensitive (get_wg ("button_kb_remove"), TRUE);
	}
	else
		gtk_widget_set_sensitive (get_wg ("button_kb_remove"), FALSE);
	g_free (tmp);
}

G_MODULE_EXPORT void
on_combobox_keyboard_variant_changed (GtkComboBox *cmb, gpointer user_data)
{
	gchar *tmp;

	if (callbacks_shield)
		return;

	keyb_update_from_variant ("combobox_keyboard_country", "combobox_keyboard_variant");

	tmp = gtk_combo_box_text_get_active_text (GTK_COMBO_BOX_TEXT (get_wg ("combobox_keyboard_country")));
	if (g_str_equal (tmp, KEYB_CUSTOM) &&
		gtk_combo_box_get_active (cmb) > -1 )

		gtk_widget_set_sensitive (get_wg ("button_kb_remove"), TRUE);
	else
		gtk_widget_set_sensitive (get_wg ("button_kb_remove"), FALSE);
	g_free (tmp);
}

G_MODULE_EXPORT void
on_button_kb_remove_clicked (GtkButton * button, gpointer user_data)
{
	gchar *tmp_path = NULL;

	tmp_path = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, keyb_get_name (), ".kbd", NULL);
	if (! g_file_test (tmp_path, G_FILE_TEST_IS_REGULAR))
	{
		gdk_beep ();
		g_message ("no valid keyboard to remove...");
		g_free (tmp_path);
		return;
	}
	g_free (tmp_path);

	gtk_label_set_text (GTK_LABEL (get_wg ("label_confirm_action")), "REMOVE");
	gtk_widget_show (get_wg ("dialog_confirm"));
}

G_MODULE_EXPORT void
on_button_kb_save_clicked (GtkButton * button, gpointer user_data)
{
	gchar *tmp;
	gchar *tmp_path;

	tmp = gtk_editable_get_chars (GTK_EDITABLE (get_wg ("entry_keyboard")), 0, -1);
	if (strlen (tmp) == 0)
	{
		g_free (tmp);
		tmp = g_strdup (KEYB_AUTO_SAVE);
	}
	tmp_path = g_strconcat (main_path_user (), G_DIR_SEPARATOR_S, tmp, ".kbd", NULL);
	keyb_set_name (tmp);
	g_free (tmp);

	gtk_label_set_text (GTK_LABEL (get_wg ("label_confirm_action")), "OVERWRITE");
	if (g_file_test (tmp_path, G_FILE_TEST_IS_REGULAR))
		gtk_widget_show (get_wg ("dialog_confirm"));
	else
		on_button_confirm_yes_clicked (GTK_BUTTON (get_wg ("button_confirm_yes")), NULL);
	g_free (tmp_path);
}

G_MODULE_EXPORT void
on_button_keyboard_previous_clicked (GtkButton *but, gpointer user_data)
{
	keyb_intro_step_previous ();
}

G_MODULE_EXPORT void
on_button_keyboard_next_clicked (GtkButton *but, gpointer user_data)
{
	keyb_intro_step_next ();
}

G_MODULE_EXPORT void
on_button_keyboard_close_clicked (GtkButton *but, gpointer user_data)
{
	hints_demo_fingers (0);
	gtk_widget_hide (get_wg ("window_keyboard"));
}

G_MODULE_EXPORT void
on_button_keyboard_hands_clicked (GtkButton *but, gpointer user_data)
{
	if (gtk_widget_get_visible (get_wg ("window_tutor")))
		if (gtk_widget_get_visible (get_wg ("hbox_keyboard_hints")))
		{
			gtk_widget_show (get_wg ("window_hints"));
			hints_update_from_char (cursor_get_char ());
		}

	hints_demo_fingers (0);
	gtk_widget_hide (get_wg ("window_keyboard"));
}

G_MODULE_EXPORT void
on_button_keyboard_cancel_clicked (GtkButton * button, gpointer user_data)
{
	gchar *tmp;

	tmp = main_preferences_get_string ("tutor", "keyboard");
	keyb_set_name (tmp);
	keyb_set_chars ();
	keyb_update_combos ("combobox_kbd_country", "combobox_kbd_variant");
	gtk_widget_hide (get_wg ("window_keyboard"));
	g_free (tmp);
}

/* Editing the keyboard.
 */
G_MODULE_EXPORT void
on_toggle_shift1_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean tog_state;
	GtkWidget *tog;

	tog = get_wg ("toggle_shift2");
	tog_state = gtk_toggle_button_get_active (togglebutton);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tog), tog_state);
	keyb_update_virtual_layout ();
	keyb_edit_none ();

	if (gtk_widget_get_visible (get_wg ("hbox_keyboard_hints")))
		hints_update_from_button (GTK_BUTTON (togglebutton));
}

G_MODULE_EXPORT void
on_toggle_shift2_toggled (GtkToggleButton * togglebutton, gpointer user_data)
{
	gboolean tog_state;
	GtkWidget *tog;

	tog = get_wg ("toggle_shift1");
	tog_state = gtk_toggle_button_get_active (togglebutton);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (tog), tog_state);
	keyb_update_virtual_layout ();
	keyb_edit_none ();

	if (gtk_widget_get_visible (get_wg ("hbox_keyboard_hints")))
		hints_update_from_button (GTK_BUTTON (togglebutton));
}

G_MODULE_EXPORT void
on_toggle_shift1_grab_focus (GtkToggleButton *tg, gpointer user_data)
{
	if (callbacks_shield)
		return;
	keyb_edit_none ();
}

G_MODULE_EXPORT void
on_toggle_shift2_grab_focus (GtkToggleButton *tg, gpointer user_data)
{
	if (callbacks_shield)
		return;
	keyb_edit_none ();
}

/* All the other keys
 */
G_MODULE_EXPORT void
on_virtual_key_grab_focus (GtkWidget *wg, gpointer user_data)
{
	if (callbacks_shield)
		return;

	if ( gtk_widget_get_visible (get_wg ("hbox_keyboard_hints")) )
		hints_update_from_button (GTK_BUTTON (wg));
	else
		keyb_edit_button (GTK_BUTTON (wg));
}

G_MODULE_EXPORT void
on_virtual_key_clicked (GtkButton * button, gpointer user_data)
{
	if (callbacks_shield)
		return;

	if ( gtk_widget_get_visible (get_wg ("hbox_keyboard_hints")) )
		hints_update_from_button (button);
	else
		keyb_edit_button (button);
}

G_MODULE_EXPORT void
on_virtual_key_changed (GtkEditable *edit, gpointer user_data)
{
	gunichar ch;

	if (callbacks_shield)
		return;

	ch = g_utf8_get_char_validated (gtk_entry_get_text (GTK_ENTRY (edit)), -1);
	if (ch == (gunichar)-2 || ch == (gunichar)-1 || ch == 0)
		ch = L' ';

	keyb_change_key (ch);
}

/* Confirm-dialog shared tasks
 */
G_MODULE_EXPORT void
on_dialog_confirm_show (GtkWidget * widget, gpointer user_data)
{
	gchar *action;
	gchar *msg;
	GtkLabel *wg_label;

	wg_label = GTK_LABEL (get_wg ("label_confirm_action"));
	action = (gchar *) gtk_label_get_text (wg_label);

	wg_label = GTK_LABEL (get_wg ("label_confirm"));

	if (g_str_equal (action, "OVERWRITE"))
	{
		gtk_window_set_title (GTK_WINDOW (widget), _("Overwrite user layout"));
		msg = g_strdup_printf ("%s\n\n %s",
				_("This will OVERWRITE an existent keyboard layout."),
			       	keyb_get_name ());
		gtk_label_set_text (wg_label, msg);
		g_free (msg);
	}
	else if (g_str_equal (action, "REMOVE"))
	{
		gtk_window_set_title (GTK_WINDOW (widget), _("Remove user layout"));
		msg = g_strdup_printf ("%s\n\n %s",
				_("This will REMOVE an existent keyboard layout."),
			       	keyb_get_name ()); 
		gtk_label_set_text (wg_label, msg);
		g_free (msg);
	}
	else if (g_str_equal (action, "RESET"))
	{
		gtk_window_set_title (GTK_WINDOW (widget), _("Reset progress data"));
		gtk_label_set_text (wg_label, _("This will DELETE all the progress data shown in the charts."));
	}
	else
	{
		g_warning ("No valid action for this confirm-dialog: %s", action);
		gtk_widget_hide (widget);
	}
}

G_MODULE_EXPORT void
on_button_confirm_yes_clicked (GtkButton * button, gpointer user_data)
{
	gchar *file;
	gchar *action;
	GtkWidget *wg;

	wg = get_wg ("label_confirm_action");
	action = (gchar *) gtk_label_get_text (GTK_LABEL (wg));

	if (g_str_equal (action, "OVERWRITE"))
	{
		keyb_save_new_layout ();
		keyb_set_keyboard_layouts ();
		keyb_update_combos ("combobox_kbd_country", "combobox_kbd_variant");
		gtk_widget_hide (get_wg ("window_keyboard"));
	}

	else if (g_str_equal (action, "REMOVE"))
	{
		keyb_remove_user_layout ();
	}

	else if (g_str_equal (action, "RESET"))
	{
		file = g_build_filename (main_path_stats (), "stat_basic.txt", NULL);
		g_unlink (file);
		g_free (file);
		file = g_build_filename (main_path_stats (), "stat_adapt.txt", NULL);
		g_unlink (file);
		g_free (file);
		file = g_build_filename (main_path_stats (), "stat_velo.txt", NULL);
		g_unlink (file);
		g_free (file);
		file = g_build_filename (main_path_stats (), "stat_fluid.txt", NULL);
		g_unlink (file);
		g_free (file);
		file = g_build_filename (main_path_stats (), "scores_fluid.txt", NULL);
		g_unlink (file);
		g_free (file);
		accur_reset ();

		basic_set_lesson (1);

		on_combobox_stat_type_changed (NULL, NULL);
	}

	else
		g_warning ("No valid action selected for 'yes' confirm-button: %s", action);

	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

G_MODULE_EXPORT void
on_button_confirm_no_clicked (GtkButton * button, gpointer user_data)
{
	gchar *action;
	GtkWidget *wg;

	wg = get_wg ("label_confirm_action");
	action = (gchar *) gtk_label_get_text (GTK_LABEL (wg));

	if (g_str_equal (action, "OVERWRITE"))
		g_printf ("Not overwriting file %s.kbd\n", keyb_get_name ());

	else if (g_str_equal (action, "REMOVE"))
		g_printf ("Not removing file %s.kbd\n", keyb_get_name ());

	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

/*******************************************************************************
 * Window hints management
 */
G_MODULE_EXPORT void
on_window_hints_check_resize (GtkContainer *cont, gpointer user_data)
{
	static int i;
	static GTimer *wait = NULL;
	gint dx, dy;

	//if (callbacks_shield)
		return;

	if (wait == NULL)
		wait = g_timer_new ();

	if (g_timer_elapsed (wait, NULL) > 0.2)
	{
		g_timer_start (wait);
		gtk_window_get_size (get_win ("window_hints"), &dx, &dy);
		g_printf ("hints_check_resize: %04i x %04i (%i)\n", dx, dy, i++);
	}
}

G_MODULE_EXPORT void
on_button_hints_close_clicked (GtkButton *but, gpointer user_data)
{
	window_save ("hints");
	gtk_widget_hide (get_wg ("window_hints"));
}

/**********************************************************************
 * 6 - Charts window
 **********************************************************************/

G_MODULE_EXPORT void
on_combobox_stat_module_changed (GtkComboBox *cmb, gpointer user_data)
{
	static gchar *stat_title = NULL;
	gchar *win_title;
	gint i;
	gchar *tmp;
	GtkWindow *win;

	if (callbacks_shield == TRUE)
		return;

	if (gtk_combo_box_get_active (cmb) < 0)
		return;

	win = GTK_WINDOW (get_wg ("window_stat"));
	if (stat_title == NULL)
	{
		plot_initialize ();
		stat_title = g_strdup (gtk_window_get_title (win));
	}

	if (user_data == NULL)
		tutor_init (gtk_combo_box_get_active (cmb));

	callbacks_shield_set (TRUE);
	for (i = 0; i < 4; i++)
		gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (get_wg ("combobox_stat_type")), 0);
	tmp = g_strdup_printf ("%s (%%)", _("Accuracy"));
	gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (get_wg ("combobox_stat_type")), 0, tmp);
	g_free (tmp);
	tmp = g_strdup_printf ("%s %s", _("Speed"), _("(WPM)"));
	gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (get_wg ("combobox_stat_type")), 1, tmp);
	g_free (tmp);

	switch (gtk_combo_box_get_active (cmb))
	{
	case 0:
		win_title = g_strdup_printf ("%s (%s)", stat_title, keyb_mode_get_name ());
		break;
	case 1:
		win_title = g_strdup_printf ("%s (%s)", stat_title, keyb_mode_get_name ());
		tmp = g_strdup_printf ("%s", _("Errors"));
		gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (get_wg ("combobox_stat_type")), 2, tmp);
		g_free (tmp);
		tmp = g_strdup_printf ("%s", _("Touch times (s)"));
		gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (get_wg ("combobox_stat_type")), 3, tmp);
		g_free (tmp);
		break;
	case 2:
		win_title = g_strdup_printf ("%s (%s)", stat_title, trans_get_current_language ());
		break;
	case 3:
		win_title = g_strdup_printf ("%s (%s)", stat_title, trans_get_current_language ());
		tmp = g_strdup_printf ("%s (%%)", _("Fluidity"));
		gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (get_wg ("combobox_stat_type")), 2, tmp);
		g_free (tmp);
		tmp = g_strdup_printf ("%s (0-10)", _("Score"));
		gtk_combo_box_text_insert_text (GTK_COMBO_BOX_TEXT (get_wg ("combobox_stat_type")), 3, tmp);
		g_free (tmp);
		break;
	default:
		win_title = g_strdup (stat_title);
	}

	gtk_combo_box_set_active (GTK_COMBO_BOX (get_wg ("combobox_stat_type")), 0);
	callbacks_shield_set (FALSE);

	gtk_window_set_title (win, win_title);
	g_free (win_title);
	plot_draw_chart (1);
}

G_MODULE_EXPORT void
on_combobox_stat_type_changed (GtkComboBox *cmb, gpointer user_data)
{
	static gboolean init = TRUE;
	gint active;

	if (callbacks_shield == TRUE)
		return;

	if (init)
	{
		plot_initialize ();
		init = FALSE;
	}

	active = gtk_combo_box_get_active ( GTK_COMBO_BOX (get_wg ("combobox_stat_type")) );
	if (active < 0)
		return;

	/* Adapt module (1) has special charts
	 */
	if (gtk_combo_box_get_active (GTK_COMBO_BOX (get_wg ("combobox_stat_module"))) == 1)
		plot_draw_chart (active + (active < 2 ? 1 : 4));
	else
		plot_draw_chart (active + 1);
}

G_MODULE_EXPORT void
on_spinbutton_stat_lesson_value_changed (GtkSpinButton * spinbutton, gpointer user_data)
{
	if (callbacks_shield)
		return;

	plot_draw_chart (gtk_combo_box_get_active (GTK_COMBO_BOX (get_wg ("combobox_stat_type"))) + 1);
}

G_MODULE_EXPORT void
on_button_stat_reset_clicked (GtkButton * button, gpointer user_data)
{
	gtk_label_set_text (GTK_LABEL (get_wg ("label_confirm_action")), "RESET");
	gtk_widget_show (get_wg ("dialog_confirm"));
}

G_MODULE_EXPORT void
on_button_stat_close_clicked (GtkButton * button, gpointer user_data)
{
	callbacks_shield_set (TRUE);
	gtk_combo_box_set_active (GTK_COMBO_BOX ( get_wg ("combobox_stat_type")), 0 );
	callbacks_shield_set (FALSE);
	window_save ("stat");
	gtk_widget_hide (gtk_widget_get_toplevel (GTK_WIDGET (button)));
}

G_MODULE_EXPORT void
on_databox_hovered (GtkDatabox *dbox, GdkEventMotion *event, gpointer user_data)
{
	plot_pointer_update (event->x);
}

/**********************************************************************
 * 7 - Other texts popup
 **********************************************************************/

G_MODULE_EXPORT void
on_treeview_other_realize (GtkWidget * widget, gpointer user_data)
{
	GtkListStore *list;
	GtkCellRenderer *renderer;
	GtkTreeViewColumn *column;

	list = gtk_list_store_new (1, G_TYPE_STRING);
	gtk_tree_view_set_model (GTK_TREE_VIEW (widget), GTK_TREE_MODEL (list));
	renderer = gtk_cell_renderer_text_new ();
	column = gtk_tree_view_column_new_with_attributes (":-)", renderer, "text", 0, NULL);
	gtk_tree_view_append_column (GTK_TREE_VIEW (widget), column);
}

G_MODULE_EXPORT void
on_button_other_new_clicked (GtkButton * button, gpointer user_data)
{
	gtk_widget_show (GTK_WIDGET (get_wg ("filechooser_tutor")));
}

G_MODULE_EXPORT void
on_button_other_cancel_clicked (GtkButton * button, gpointer user_data)
{
	g_free (other_renaming_undo);
	other_renaming_undo = g_strdup ("*");
	gtk_widget_hide (get_wg ("popup_other"));
}

G_MODULE_EXPORT void
on_button_other_apply_clicked (GtkButton *button, gpointer user_data)
{
	gchar *tmp_name;
	GtkTreeSelection *sel;
	GtkTreeModel *store;
	GtkTreeIter iter;

	gtk_widget_hide (get_wg ("popup_other"));

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (get_wg ("treeview_other")));
	if (gtk_tree_selection_get_selected (sel, &store, &iter) == FALSE)
	{
		tutor_set_query (QUERY_INTRO);
		tutor_process_touch ('\0');
		return;
	}

	tutor_other_rename (gtk_entry_get_text (GTK_ENTRY (get_wg ("entry_other_rename"))), other_renaming_undo);
	g_free (other_renaming_undo);
	other_renaming_undo = g_strdup ("*");

	gtk_tree_model_get (store, &iter, 0, &tmp_name, -1);

	if (tutor_get_type () == TT_VELO)
		velo_init_dict (tmp_name);
	else if (tutor_get_type () == TT_FLUID)
		fluid_init_paragraph_list (tmp_name);

	tutor_set_query (QUERY_INTRO);
	tutor_process_touch ('\0');
}

G_MODULE_EXPORT void
on_button_other_paste_clicked (GtkButton * button, gpointer user_data)
{
	gchar *text;
	GtkWidget *wg;
	GtkListStore *list;

	if (clipboard == NULL)
		clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
	if (clipboard_2 == NULL)
		clipboard_2 = gtk_clipboard_get (GDK_SELECTION_PRIMARY);

	text = gtk_clipboard_wait_for_text (clipboard);
	if (text == NULL)
		text = gtk_clipboard_wait_for_text (clipboard_2);
	if (text == NULL)
	{
		gdk_beep ();
		g_message ("No text in the Clipboard");
		return;
	}

	wg = get_wg ("treeview_other");
	list = GTK_LIST_STORE ( gtk_tree_view_get_model (GTK_TREE_VIEW (wg)) );
	if (tutor_get_type () == TT_VELO)
	{
		velo_text_write_to_file (text, TRUE);
		tutor_load_list_other (".words", list);
	}

	else if (tutor_get_type () == TT_FLUID)
	{
		fluid_text_write_to_file (text);
		tutor_load_list_other (".paragraphs", list);
	}
	g_free (text);

	g_free (other_renaming_undo);
	other_renaming_undo = g_strdup ("*");
	gtk_widget_hide (get_wg ("popup_other"));
}

G_MODULE_EXPORT void
on_button_other_remove_clicked (GtkButton * button, gpointer user_data)
{
	gchar *tmp_str;
	gchar *tmp_name;
	GtkTreeSelection *sel;
	GtkTreeModel *store;
	GtkTreeIter iter;

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (get_wg ("treeview_other")));
	if (gtk_tree_selection_get_selected (sel, &store, &iter) == FALSE)
	{
		gdk_beep ();
		return;
	}

	gtk_tree_model_get (store, &iter, 0, &tmp_str, -1);

	if (g_str_equal (tmp_str, OTHER_DEFAULT))
	{
		gdk_beep ();
		g_free (tmp_str);
		return;
	}

	tmp_name = g_build_filename (main_path_user (), tmp_str, NULL);
	g_free (tmp_str);

	if (tutor_get_type () == TT_VELO)
		tmp_str = g_strconcat (tmp_name, ".words", NULL);
	else if (tutor_get_type () == TT_FLUID)
		tmp_str = g_strconcat (tmp_name, ".paragraphs", NULL);
	g_unlink (tmp_str);
	g_free (tmp_str);
	g_free (tmp_name);

	gtk_list_store_remove (GTK_LIST_STORE (store), &iter);
	gtk_widget_set_sensitive (get_wg ("button_other_remove"), FALSE);
	gtk_widget_set_sensitive (get_wg ("label_other_rename"), FALSE);
	gtk_widget_set_sensitive (get_wg ("entry_other_rename"), FALSE);
}

G_MODULE_EXPORT void
on_entry_other_rename_changed (GtkEditable *editable, gint start_pos, gint end_pos, gpointer user_data)
{
	gchar *str;
	GtkTreeSelection *sel;
	GtkTreeModel *store;
	GtkTreeIter iter;

	if (callbacks_shield)
		return;

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (get_wg ("treeview_other")));
	if (gtk_tree_selection_get_selected (sel, &store, &iter) == FALSE)
	{
		gtk_widget_set_sensitive (get_wg ("label_other_rename"), FALSE);
		gtk_widget_set_sensitive (get_wg ("entry_other_rename"), FALSE);
		return;
	}
	str = (gchar*) gtk_entry_get_text (GTK_ENTRY (get_wg ("entry_other_rename")));
	if (strlen (str) > 0)
		gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, str, -1);
	else
		gtk_list_store_set (GTK_LIST_STORE (store), &iter, 0, other_renaming_undo, -1);
}

G_MODULE_EXPORT void
on_entry_other_rename_activate (GtkEntry *entry, gpointer user_data)
{
	on_button_other_apply_clicked (NULL, NULL);
}

G_MODULE_EXPORT void
on_treeview_other_cursor_changed (GtkTreeView *treeview, gpointer user_data)
{
	gchar *tmp_str;
	GtkTreeSelection *sel;
	GtkTreeModel *store;
	GtkTreeIter iter;
	GtkEntry *entry;

	sel = gtk_tree_view_get_selection (GTK_TREE_VIEW (get_wg ("treeview_other")));
	if (gtk_tree_selection_get_selected (sel, &store, &iter) == FALSE)
	{
		gtk_widget_set_sensitive (get_wg ("button_other_apply"), FALSE);
		return;
	}
	gtk_widget_set_sensitive (get_wg ("button_other_apply"), TRUE);

	entry = GTK_ENTRY (get_wg ("entry_other_rename"));

	if (other_renaming_undo == NULL)
		other_renaming_undo = g_strdup ("*");
	else
		tutor_other_rename (gtk_entry_get_text (entry), other_renaming_undo);

	callbacks_shield_set (TRUE);
	g_free (other_renaming_undo);
	gtk_tree_model_get (store, &iter, 0, &tmp_str, -1);
	if (g_str_equal (tmp_str, OTHER_DEFAULT))
	{
		gtk_entry_set_text (entry, "");
		gtk_widget_set_sensitive (get_wg ("button_other_remove"), FALSE);
		gtk_widget_set_sensitive (get_wg ("label_other_rename"), FALSE);
		gtk_widget_set_sensitive (get_wg ("entry_other_rename"), FALSE);
		other_renaming_undo = g_strdup ("*");
	}
	else
	{
		gtk_widget_set_sensitive (get_wg ("button_other_remove"), TRUE);
		gtk_widget_set_sensitive (get_wg ("label_other_rename"), TRUE);
		gtk_widget_set_sensitive (get_wg ("entry_other_rename"), TRUE);
		gtk_entry_set_text (entry, tmp_str);
		other_renaming_undo = g_strdup (tmp_str);
	}
	callbacks_shield_set (FALSE);

}

G_MODULE_EXPORT void
on_treeview_other_row_activated (GtkTreeView *treeview, GtkTreePath *path, GtkTreeViewColumn *column, gpointer user_data)
{
	on_button_other_apply_clicked (NULL, NULL);
}

G_MODULE_EXPORT void
on_button_filechooser_open_clicked (GtkButton *button, gpointer user_data)
{
	gchar *tmp_path;
	GtkWidget *wg;
	GtkListStore *list;

	tmp_path = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (get_wg ("filechooser_tutor")));
	if (tutor_get_type () == TT_VELO)
		velo_create_dict (tmp_path, TRUE);
	else if (tutor_get_type () == TT_FLUID)
		fluid_copy_text_file (tmp_path);
	g_free (tmp_path);

	wg = get_wg ("treeview_other");
	list = GTK_LIST_STORE ( gtk_tree_view_get_model (GTK_TREE_VIEW (wg)) );
	if (tutor_get_type () == TT_VELO)
		tutor_load_list_other (".words", list);
	else if (tutor_get_type () == TT_FLUID)
		tutor_load_list_other (".paragraphs", list);

	g_free (other_renaming_undo);
	other_renaming_undo = g_strdup ("*");
	gtk_widget_hide (get_wg ("popup_other"));
	gtk_widget_hide (get_wg ("filechooser_tutor"));
}

G_MODULE_EXPORT void
on_button_filechooser_cancel_clicked (GtkButton *button, gpointer user_data)
{
	gtk_widget_hide (get_wg ("filechooser_tutor"));
}

/**********************************************************************
 * 8 - Mangling with the windows positions 
 **********************************************************************/

G_MODULE_EXPORT void
on_window_main_show (GtkWidget *wg, gpointer user_data)
{
	window_restore ("main");
}

G_MODULE_EXPORT void
on_window_tutor_show (GtkWidget *wg, gpointer user_data)
{
	window_restore ("tutor");
}

G_MODULE_EXPORT void
on_window_hints_show (GtkWidget *wg, gpointer user_data)
{
	window_restore ("hints");
}

G_MODULE_EXPORT void
on_window_top10_show (GtkWidget *wg, gpointer user_data)
{
	window_restore ("top10");
}

G_MODULE_EXPORT void
on_window_stat_show (GtkWidget *wg, gpointer user_data)
{
	window_restore ("stat");
}

void
window_restore (gchar *who)
{
	gint x, y;
	gchar *str;

	str = g_strconcat (who, "_X", NULL);
	if (main_preferences_exist ("windows", str))
		x = main_preferences_get_int ("windows", str);
	else
	{
		g_free (str);
		return;
	}
	g_free (str);

	str = g_strconcat (who, "_Y", NULL);
	if (main_preferences_exist ("windows", str))
		y = main_preferences_get_int ("windows", str);
	else
	{
		g_free (str);
		return;
	}
	g_free (str);

	str = g_strconcat ("window_", who, NULL);
	gtk_window_move (get_win (str), x, y);
	g_free (str);
}

void
window_save (gchar *who)
{
	gint x, y;
	gchar *str;

	str = g_strconcat ("window_", who, NULL);
	gtk_window_get_position (get_win (str), &x, &y);
	g_free (str);

	str = g_strconcat (who, "_X", NULL);
	main_preferences_set_int ("windows", str, x);
	g_free (str);

	str = g_strconcat (who, "_Y", NULL);
	main_preferences_set_int ("windows", str, y);
	g_free (str);
}
