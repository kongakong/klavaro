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
 * Shared tutor window tasks
 */
#include <math.h>
#include <string.h>
#include <time.h>
#include <stdio.h>
#include <stdlib.h>
#include <locale.h>
#include <errno.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "main.h"
#include "auxiliar.h"
#include "callbacks.h"
#include "translation.h"
#include "keyboard.h"
#include "cursor.h"
#include "basic.h"
#include "adaptability.h"
#include "velocity.h"
#include "fluidness.h"
#include "accuracy.h"
#include "top10.h"
#include "tutor.h"

#define MAX_TOUCH_TICS 10000
struct
{
	TutorType type;
	TutorQuery query;
	GTimer *tmr;
	gdouble elapsed_time;
	gdouble touch_time[MAX_TOUCH_TICS + 1];
	guint ttidx;
	gint n_touchs;
	gint n_errors;
	gint retro_pos;
	gint correcting;
} tutor;

struct
{
	struct
	{
		double accuracy;
		double speed;
	} basic;
	struct
	{
		double accuracy;
		double speed;
		double accuracy_learning;
		double accuracy_improving;
		double accuracy_reaching;
	} adapt;
	struct
	{
		double accuracy;
		double speed;
		double speed_crawling;
		double speed_stepping;
		double speed_walking;
		double speed_jogging;
		double speed_running;
		double speed_professional;
		double speed_racer;
		double speed_flying;
	} velo;
	struct
	{
		double accuracy;
		double speed;
		double fluidity;
		double fluidity_stumbling;
		double speed_flying;
	} fluid;
} goal;

extern gchar *OTHER_DEFAULT;

/*******************************************************************************
 * Interface functions
 */
TutorType
tutor_get_type ()
{
	return (tutor.type);
}

gchar *
tutor_get_type_name ()
{
	static gchar type_name[4][6] = { "basic", "adapt", "velo", "fluid" };

	return (type_name[tutor.type]);
}

TutorQuery
tutor_get_query ()
{
	return (tutor.query);
}

void
tutor_set_query (TutorQuery query)
{
	tutor.query = query;
}

gint
tutor_get_correcting ()
{
	if (tutor.type != TT_FLUID)
		return (FALSE);
	return (tutor.correcting);
}

void
tutor_init_timers ()
{
	tutor.tmr = g_timer_new ();
}

#define GOAL_GSET(MODULE, GOAL, DEFAULT_VAL) \
	if (main_preferences_exist ("goals", #MODULE "_" #GOAL))\
		goal.MODULE.GOAL = (gdouble) main_preferences_get_int ("goals", #MODULE "_" #GOAL);\
	else\
	{\
		goal.MODULE.GOAL = DEFAULT_VAL;\
		main_preferences_set_int ("goals", #MODULE "_" #GOAL, DEFAULT_VAL);\
	}
#define LEVEL_GSET(MODULE, GOAL, DEFAULT_VAL) \
	if (main_preferences_exist ("levels", #MODULE "_" #GOAL))\
		goal.MODULE.GOAL = (gdouble) main_preferences_get_int ("levels", #MODULE "_" #GOAL);\
	else\
	{\
		goal.MODULE.GOAL = DEFAULT_VAL;\
		main_preferences_set_int ("levels", #MODULE "_" #GOAL, DEFAULT_VAL);\
	}
void
tutor_init_goals ()
{
	GOAL_GSET (basic, accuracy, 95);
	GOAL_GSET (basic, speed, 10);
	GOAL_GSET (adapt, accuracy, 98);
	GOAL_GSET (adapt, speed, 10);
	GOAL_GSET (velo, accuracy, 95);
	GOAL_GSET (velo, speed, 50);
	GOAL_GSET (fluid, accuracy, 97);
	GOAL_GSET (fluid, speed, 50);
	GOAL_GSET (fluid, fluidity, 70);

	LEVEL_GSET (adapt, accuracy_learning, 50);
	LEVEL_GSET (adapt, accuracy_improving, 90);
	LEVEL_GSET (adapt, accuracy_reaching, 95);

	LEVEL_GSET (velo, speed_crawling, 10);
	LEVEL_GSET (velo, speed_stepping, 20);
	LEVEL_GSET (velo, speed_walking, 30);
	LEVEL_GSET (velo, speed_jogging, 40);
	LEVEL_GSET (velo, speed_running, 60);
	LEVEL_GSET (velo, speed_professional, 70);
	LEVEL_GSET (velo, speed_racer, 80);
	LEVEL_GSET (velo, speed_flying, 90);

	LEVEL_GSET (fluid, fluidity_stumbling, 60);
	LEVEL_GSET (fluid, speed_flying, 90);

	/*
	g_printf ("basic accur: %.0f\n", goal.basic.accuracy);
	g_printf ("basic speed: %.0f\n", goal.basic.speed);
	g_printf ("adapt accur: %.0f\n", goal.adapt.accuracy);
	g_printf ("adapt speed: %.0f\n", goal.adapt.speed);
	g_printf ("velo accur: %.0f\n", goal.velo.accuracy);
	g_printf ("velo speed: %.0f\n", goal.velo.speed);
	g_printf ("fluid accur: %.0f\n", goal.fluid.accuracy);
	g_printf ("fluid speed: %.0f\n", goal.fluid.speed);
	g_printf ("fluid fluidity: %.0f\n", goal.fluid.fluidity);

	g_printf ("adapt learning: %.0f\n", goal.adapt.accuracy_learning);
	g_printf ("adapt improving: %.0f\n", goal.adapt.accuracy_improving);
	g_printf ("adapt reaching: %.0f\n", goal.adapt.accuracy_reaching);

	g_printf ("velo crawling: %.0f\n", goal.velo.speed_crawling);
	g_printf ("velo stepping: %.0f\n", goal.velo.speed_stepping);
	g_printf ("velo walking: %.0f\n", goal.velo.speed_walking);
	g_printf ("velo jogging: %.0f\n", goal.velo.speed_jogging);
	g_printf ("velo running: %.0f\n", goal.velo.speed_running);
	g_printf ("velo professional: %.0f\n", goal.velo.speed_professional);
	g_printf ("velo racer: %.0f\n", goal.velo.speed_racer);
	g_printf ("velo flying: %.0f\n", goal.velo.speed_flying);

	g_printf ("fluid stumbling: %.0f\n", goal.fluid.fluidity_stumbling);
	g_printf ("fluid flying: %.0f\n", goal.fluid.speed_flying);
	 */
}

gdouble
tutor_goal_accuracy ()
{
	switch (tutor.type)
	{
		case TT_BASIC: return goal.basic.accuracy;
		case TT_ADAPT: return goal.adapt.accuracy;
		case TT_VELO: return goal.velo.accuracy;
		case TT_FLUID: return goal.fluid.accuracy;
	}
	return -1.0;
}

gdouble
tutor_goal_speed ()
{
	switch (tutor.type)
	{
		case TT_BASIC: return goal.basic.speed;
		case TT_ADAPT: return goal.adapt.speed;
		case TT_VELO: return goal.velo.speed;
		case TT_FLUID: return goal.fluid.speed;
	}
	return -1.0;
}

gdouble
tutor_goal_fluidity ()
{
	switch (tutor.type)
	{
		case TT_BASIC:
		case TT_ADAPT:
		case TT_VELO: return 0.0;
		case TT_FLUID: return goal.fluid.fluidity;
	}
	return -1.0;
}

gdouble
tutor_goal_level (guint n)
{
	switch (tutor.type)
	{
		case TT_ADAPT: if (n > 2) return -1.0; return (&goal.adapt.accuracy_learning) [n];
		case TT_VELO: if (n > 7) return -2.0; return (&goal.velo.speed_crawling) [n];
		case TT_FLUID: if (n > 1) return -3.0; return (&goal.fluid.fluidity_stumbling) [n];
	}
	return -4.0;
}

/**********************************************************************
 * Initialize the course 
 */
void
tutor_init (TutorType tt_type)
{
	gchar *tmp_title = NULL;
	gchar *tmp_name = NULL;
	GtkWidget *wg;

	gtk_widget_hide (get_wg ("window_main"));
	gtk_widget_hide (get_wg ("window_keyboard"));
	gtk_widget_hide (get_wg ("dialog_info"));
	gtk_widget_hide (get_wg ("aboutdialog_klavaro"));
	gtk_widget_hide (get_wg ("togglebutton_toomuch_errors"));
	gtk_widget_show (get_wg ("window_tutor"));
	gtk_widget_grab_focus (get_wg ("entry_mesg"));

	tutor.type = tt_type;
	cursor_set_blink (FALSE);

	/******************************
	 * Set the layout for each exercise type
	 */
	gtk_widget_hide (get_wg ("button_tutor_top10"));
	gtk_widget_hide (get_wg ("entry_custom_basic_lesson"));
	if (tutor.type == TT_BASIC || tutor.type == TT_FLUID)
	{
		gtk_widget_show (get_wg ("label_lesson"));
		gtk_widget_show (get_wg ("spinbutton_lesson"));
		gtk_widget_show (get_wg ("vseparator_tutor_2"));
		if (tutor.type == TT_BASIC)
		{
			gtk_widget_show (get_wg ("togglebutton_edit_basic_lesson"));
			gtk_widget_hide (get_wg ("button_tutor_top10"));
			gtk_widget_show (get_wg ("button_tutor_show_keyb"));
			gtk_label_set_text (GTK_LABEL (get_wg ("label_lesson")), _("Lesson:"));
			callbacks_shield_set (TRUE);
			gtk_spin_button_set_range (GTK_SPIN_BUTTON (get_wg ("spinbutton_lesson")), 1, MAX_BASIC_LESSONS);
			callbacks_shield_set (FALSE);
		}
		else
		{
			gtk_widget_hide (get_wg ("togglebutton_edit_basic_lesson"));
			gtk_widget_show (get_wg ("button_tutor_top10"));
			gtk_widget_hide (get_wg ("button_tutor_show_keyb"));
			gtk_label_set_text (GTK_LABEL (get_wg ("label_lesson")), _("Paragraphs:"));
			callbacks_shield_set (TRUE);
			gtk_spin_button_set_range (GTK_SPIN_BUTTON (get_wg ("spinbutton_lesson")), 0, 10);
			callbacks_shield_set (FALSE);
		}
	}
	else
	{
		gtk_widget_hide (get_wg ("label_lesson"));
		gtk_widget_hide (get_wg ("spinbutton_lesson"));
		gtk_widget_hide (get_wg ("vseparator_tutor_2"));
		gtk_widget_hide (get_wg ("togglebutton_edit_basic_lesson"));
		gtk_widget_hide (get_wg ("button_tutor_show_keyb"));
		gtk_widget_hide (get_wg ("button_tutor_top10"));
	}

	if (tutor.type == TT_BASIC || tutor.type == TT_ADAPT)
	{
		gtk_widget_hide (get_wg ("button_tutor_other"));
		gtk_widget_hide (get_wg ("vseparator_tutor_1"));
	}
	else
	{
		gtk_widget_show (get_wg ("button_tutor_other"));
		gtk_widget_show (get_wg ("vseparator_tutor_1"));
	}

	/******************************
	 * Set decoration texts and tips
	 */
	switch (tutor.type)
	{
	case TT_BASIC:
		tmp_title = g_strdup (_("Klavaro - Basic Course"));
		tmp_name = g_strdup ("");
		break;

	case TT_ADAPT:
		tmp_title = g_strdup (_("Klavaro - Adaptability"));
		tmp_name =
			g_strdup (_
				  ("Adaptability exercises: automating the fingers"
				   " responses, typing over all the keyboard."));
		break;

	case TT_VELO:
		tmp_title = g_strdup (_("Klavaro - Velocity"));
		tmp_name = g_strdup (_("Velocity exercises: accelerate typing real words."));
		break;

	case TT_FLUID:
		tmp_title = g_strdup (_("Klavaro - Fluidness"));
		tmp_name =
			g_strdup (_("Fluidness exercises: accuracy typing good sense paragraphs."));
		break;
	}
	gtk_window_set_title (get_win ("window_tutor"), tmp_title);
	wg = get_wg ("label_heading");
	gtk_label_set_text (GTK_LABEL (wg), tmp_name);
	g_free (tmp_title);
	g_free (tmp_name);

	/******************************
	 * Set tooltips of tutor entry (drag and drop)
	 */
	if (tutor.type == TT_VELO || tutor.type == TT_FLUID)
		gtk_widget_set_tooltip_text (get_wg ("entry_mesg"), _("Drag and drop text here to practice with it."));
	else
		gtk_widget_set_tooltip_text (get_wg ("entry_mesg"), "");

	/******************************
	 * Set specific variables
	 */
	tutor.query = QUERY_INTRO;
	if (tutor.type == TT_BASIC)
	{
		basic_init ();
		if (basic_get_lesson () > 1)
		{
			tutor_process_touch ('\0');
			return;
		}
	}
	else if (tutor.type == TT_VELO)
	{
		velo_init ();
	}
	else if (tutor.type == TT_FLUID)
	{
		fluid_init ();
	}
	tutor_update ();
}


/**********************************************************************
 * Update what is shown in the tutor window. 
 */
void
tutor_update ()
{
	switch (tutor.query)
	{
	case QUERY_INTRO:
		tutor_update_intro ();
		break;

	case QUERY_START:
		tutor_update_start ();
		break;

	case QUERY_PROCESS_TOUCHS:
		break;

	case QUERY_END:
		tutor_message (_("End of exercise. Press [Enter] to start another."));
		break;
	}
}

void
tutor_update_intro ()
{
	gchar *tmp_name;
	gchar *text;
	gchar *color_bg;
	GdkRGBA color;
	GtkWidget *wg;
	GtkLabel *wg_label;
	GtkTextView *wg_text;
	GtkAdjustment *scroll;
	GtkTextIter start;
	GtkTextIter end;

	if (tutor.type == TT_BASIC)
	{
		callbacks_shield_set (TRUE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON
					   (get_wg ("spinbutton_lesson")),
					   basic_get_lesson ());
		callbacks_shield_set (FALSE);

		wg_label = GTK_LABEL (get_wg ("label_heading"));
		gtk_label_set_text (wg_label, _("Learning the key positions."));
	}

	tutor_message (_("Press any key to start the exercise. "));

	tmp_name = g_strconcat ("_", tutor_get_type_name (), "_intro.txt", NULL);
	text = trans_read_text (tmp_name);
	g_free (tmp_name);

	wg_text = GTK_TEXT_VIEW (get_wg ("text_tutor"));
	gtk_text_buffer_set_text (gtk_text_view_get_buffer (wg_text), text, -1);
	g_free (text);

	gtk_text_buffer_get_bounds (gtk_text_view_get_buffer (wg_text), &start, &end);
	gtk_text_buffer_apply_tag_by_name (gtk_text_view_get_buffer (wg_text), "lesson_font", &start, &end);
	gtk_text_buffer_apply_tag_by_name (gtk_text_view_get_buffer (wg_text), "text_intro", &start, &end);

	if (main_preferences_exist ("colors", "text_intro_bg"))
		color_bg = main_preferences_get_string ("colors", "text_intro_bg");
	else
		color_bg = g_strdup (TUTOR_WHITE);
	gdk_rgba_parse (&color, color_bg);
	gtk_widget_override_background_color (get_wg ("text_tutor"), GTK_STATE_FLAG_INSENSITIVE, &color);
	g_free (color_bg);

	wg = get_wg ("scrolledwindow_tutor_main");
	scroll = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (wg));
	gtk_adjustment_set_value (scroll, 0);

	callbacks_shield_set (TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (get_wg ("togglebutton_tutor_intro")), TRUE);
	callbacks_shield_set (FALSE);
}

void
tutor_update_start ()
{
	gchar *tmp_name;
	gchar *text;
	gchar *color_bg;
	GdkRGBA color;
	GtkWidget *wg;
	GtkTextBuffer *buf;
	GtkTextIter start;
	GtkTextIter end;
	GtkAdjustment *scroll;

	/*
	 * Delete all the text on tutor window
	 */
	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));
	gtk_text_buffer_set_text (buf, "", -1);

	if (tutor.type == TT_BASIC)
	{
		callbacks_shield_set (TRUE);
		gtk_spin_button_set_value (GTK_SPIN_BUTTON (get_wg ("spinbutton_lesson")),
					   basic_get_lesson ());
		callbacks_shield_set (FALSE);

		tmp_name = g_ucs4_to_utf8 (basic_get_char_set (), -1, NULL, NULL, NULL);
		text = g_strdup_printf ("%s %s", _("Keys:"), tmp_name);
		g_free (tmp_name);
		wg = get_wg ("label_heading");
		gtk_label_set_text (GTK_LABEL (wg), text);
		g_free (text);

		basic_draw_lesson ();
	}

	switch (tutor.type)
	{
	case TT_BASIC:
		break;
	case TT_ADAPT:
		adapt_draw_random_pattern ();
		break;
	case TT_VELO:
		velo_draw_random_words ();
		break;
	case TT_FLUID:
		fluid_draw_random_paragraphs ();
	}

	/*
	 * Apply tutor background color and font to the text
	 */
	if (main_preferences_exist ("colors", "char_untouched_bg"))
		color_bg = main_preferences_get_string ("colors", "char_untouched_bg");
	else
		color_bg = g_strdup (TUTOR_CREAM);
	gdk_rgba_parse (&color, color_bg);
	gtk_widget_override_background_color (get_wg ("text_tutor"), GTK_STATE_FLAG_INSENSITIVE, &color);
	g_free (color_bg);

	gtk_text_buffer_get_bounds (buf, &start, &end);
	gtk_text_iter_backward_char (&end);
	gtk_text_buffer_apply_tag_by_name (buf, "lesson_font", &start, &end);

	/* Trying to minimize automatic wrapping because of cursor blinking:
	*/
	end = start;
	while (gtk_text_iter_forward_word_end (&end))
	{
		gtk_text_buffer_apply_tag_by_name (buf, "char_keep_wrap", &start, &end);
		start = end;
		if (! gtk_text_iter_forward_char (&end))
			break;
		gtk_text_buffer_apply_tag_by_name (buf, "char_keep_wrap2", &start, &end);
		start = end;
	}
	
	if (tutor.type == TT_FLUID)
		tmp_name = g_strconcat (_("Start typing when you are ready. "), " ",
			       _("Use backspace to correct errors."), " ", NULL);
	else
		tmp_name = g_strdup (_("Start typing when you are ready. "));

	tutor_message (tmp_name);
	g_free (tmp_name);

	wg = get_wg ("scrolledwindow_tutor_main");
	scroll = gtk_scrolled_window_get_vadjustment (GTK_SCROLLED_WINDOW (wg));
	gtk_adjustment_set_value (scroll, 0);

	callbacks_shield_set (TRUE);
	gtk_toggle_button_set_active (GTK_TOGGLE_BUTTON (get_wg ("togglebutton_tutor_intro")), FALSE);
	callbacks_shield_set (FALSE);
}


/**********************************************************************
 * Respond to each touch of the user, according to the tutor.query mode
 */
void
tutor_process_touch (gunichar user_chr)
{
	gboolean go_on;
	gchar *u8ch;
	GtkTextView *wg_text;
	GtkTextBuffer *wg_buffer;
	GtkTextIter start;

	wg_text = GTK_TEXT_VIEW (get_wg ("text_tutor"));
	wg_buffer = gtk_text_view_get_buffer (wg_text);

	switch (tutor.query)
	{
	case QUERY_PROCESS_TOUCHS:
		break;

	case QUERY_INTRO:
		tutor.query = QUERY_START;
		tutor_update ();
		tutor.n_touchs = 0;
		tutor.n_errors = 0;
		tutor.retro_pos = 0;
		tutor.correcting = 0;
		tutor.ttidx = 0;
		gtk_text_buffer_get_start_iter (wg_buffer, &start);
		gtk_text_buffer_place_cursor (wg_buffer, &start);
		cursor_set_blink (TRUE);
		cursor_on (NULL);
		accur_sort ();

		switch (tutor.type)
		{
		case TT_BASIC:
			hints_update_from_char (cursor_get_char ());
			tutor_speak_char ();
			return;
		case TT_ADAPT:
			tutor_speak_char ();
			return;
		case TT_VELO:
			tutor_speak_word ();
			return;
		case TT_FLUID:
			return;
		}
		return;

	case QUERY_START:
		tutor.query = QUERY_PROCESS_TOUCHS;
		callbacks_shield_set (TRUE);
		u8ch = g_malloc0 (7);
		if (g_unichar_to_utf8 (user_chr, u8ch) > 0)
			tutor_message (u8ch);
		else
			tutor_message ("");
		g_free (u8ch);
		callbacks_shield_set (FALSE);

		g_timer_start (tutor.tmr);
		if (tutor.type == TT_FLUID)
			tutor_eval_forward_backward (user_chr);
		else
			tutor_eval_forward (user_chr);

		switch (tutor.type)
		{
		case TT_BASIC:
			hints_update_from_char (cursor_get_char ());
			tutor_speak_char ();
			return;
		case TT_ADAPT:
			tutor_speak_char ();
			return;
		case TT_VELO:
			tutor_speak_word ();
			return;
		case TT_FLUID:
			return;
		}
		return;

	case QUERY_END:
		if (user_chr == UPSYM)
		{
			basic_set_lesson_increased (FALSE);
			tutor.query = QUERY_INTRO;
			tutor_process_touch (L'\0');
		}
		else if (user_chr == (gunichar) 8 && tutor.type == TT_BASIC)
		{
			basic_set_lesson (basic_get_lesson () - 1);
			basic_init_char_set ();
			basic_set_lesson_increased (FALSE);
			tutor.query = QUERY_INTRO;
			tutor_process_touch (L'\0');
		}
		else
		{
			tutor_beep ();
			tutor_update ();
		}
		return;
	}

	/* It is time to analise the correctness of typing.
	 */
	if (tutor.type == TT_FLUID)
		go_on = tutor_eval_forward_backward (user_chr);
	else
		go_on = tutor_eval_forward (user_chr);

	if (go_on == FALSE)
	{
		cursor_set_blink (FALSE);
		cursor_off (NULL);
		tutor.elapsed_time = g_timer_elapsed (tutor.tmr, NULL);

		tutor_calc_stats ();
		tutor.query = QUERY_END;
		tutor_update ();
		tutor_beep ();
	}
	else
	{
		switch (tutor.type)
		{
		case TT_BASIC:
			cursor_on (NULL);
			hints_update_from_char (cursor_get_char ());
			tutor_speak_char ();
			return;
		case TT_ADAPT:
			cursor_on (NULL);
			tutor_speak_char ();
			return;
		case TT_VELO:
			cursor_on (NULL);
			tutor_speak_word ();
			return;
		case TT_FLUID:
			cursor_off (NULL);
			return;
		}
	}
}

/**********************************************************************
 * Advances the cursor one position and test for correctness,
 * in the shared tutor window.
 * Updates the variables:
 * cursor_pos, n_touchs and n_errors.
 */
gboolean
tutor_eval_forward (gunichar user_chr)
{
	gunichar real_chr;

	if (user_chr == L'\b' || user_chr == L'\t')
	{
		tutor_beep ();
		return (TRUE);
	}

	gsize n_touchs = g_unichar_fully_decompose (user_chr, FALSE, NULL, 0);
	tutor.n_touchs += (int) n_touchs;

	real_chr = cursor_get_char ();

	// Minimizing the line breaking bug:
	if (user_chr == UPSYM && real_chr == L' ')
		user_chr = L' ';

	/*
	 * Compare the user char with the real char and set the color
	 */
	if (user_chr == real_chr)
	{
		if (tutor.ttidx < MAX_TOUCH_TICS)
		{
			tutor.touch_time[tutor.ttidx] =
				g_timer_elapsed (tutor.tmr, NULL) - tutor.touch_time[tutor.ttidx];
			tutor.ttidx++;
			tutor.touch_time[tutor.ttidx] = g_timer_elapsed (tutor.tmr, NULL);
			if (tutor.type != TT_BASIC)
				accur_correct (real_chr, tutor.touch_time[tutor.ttidx-1]);
		}

		cursor_paint_char ("char_correct");
	}
	else
	{
		tutor.touch_time[tutor.ttidx] = g_timer_elapsed (tutor.tmr, NULL);
		if (tutor.type != TT_BASIC)
			accur_wrong (real_chr);

		cursor_paint_char ("char_wrong");
		tutor.n_errors += (int) n_touchs;
		tutor_beep ();
	}


	/*
	 * Go forward and test end of text
	 */
	if (cursor_advance (1) != 1)
		return (FALSE);

	/*
	 * Test end of text
	 */
	if (cursor_get_char () == L'\n')
		if (cursor_advance (1) != 1)
			return (FALSE);

	return (TRUE);
}

/**********************************************************************
 * Like the previous, but allows to go back and forth.
 */
gboolean
tutor_eval_forward_backward (gunichar user_chr)
{
	gunichar real_chr;

	/*
	 * Work on backspaces
	 * L'\t' means an hyper <Ctrl> + <Backspace>
	 */
	if (user_chr == L'\b' || user_chr == L'\t')
	{
		tutor.touch_time[tutor.ttidx] = g_timer_elapsed (tutor.tmr, NULL);

		/*
		 * Test for end of errors to be corrected
		 */
		if (tutor.retro_pos == 0)
		{
			tutor_beep ();
			return (TRUE);
		}

		/*
		 * Go backwards and test for begin of the text
		 */
		if (cursor_advance (-1) != -1)
		{
			tutor_beep ();
			return (TRUE);
		}

		/*
		 * Test for start of line
		 */
		if (cursor_get_char () == L'\n')
			if (cursor_advance (-1) != -1)
			{
				tutor_beep ();
				return (TRUE);
			}

		/*
		 * Reinitialize the color (no visible effect at all...)
		 */
		cursor_paint_char ("char_untouched");

		/*
		 * Update state
		 */
		tutor.retro_pos--;
		tutor.correcting++;

		if (user_chr == L'\t')
			tutor_eval_forward_backward (L'\t');
		return (TRUE);
	}

	real_chr = cursor_get_char ();

	// Minimizing the line-breaking bug:
	if (user_chr == UPSYM && real_chr == L' ')
		user_chr = L' ';

	/*
	 * Compare the user char with the real char and set the color
	 */
	if (user_chr == real_chr && tutor.retro_pos == 0)
	{
	        gsize n_touchs = g_unichar_fully_decompose (user_chr, FALSE, NULL, 0);
	        tutor.n_touchs += (int) n_touchs;
		if (tutor.ttidx < MAX_TOUCH_TICS)
		{
			tutor.touch_time[tutor.ttidx] =
				g_timer_elapsed (tutor.tmr, NULL) - tutor.touch_time[tutor.ttidx];
			tutor.ttidx++;
			tutor.touch_time[tutor.ttidx] = g_timer_elapsed (tutor.tmr, NULL);
		}
		if (tutor.correcting != 0)
		{
			cursor_paint_char ("char_retouched");
			tutor.n_errors += (int) n_touchs;
		}
		else
		{
			cursor_paint_char ("char_correct");
			accur_correct (real_chr, tutor.touch_time[tutor.ttidx-1]);
		}
	}
	else
	{
		tutor.touch_time[tutor.ttidx] = g_timer_elapsed (tutor.tmr, NULL);
		cursor_paint_char ("char_wrong");
		tutor.retro_pos++;
		if (tutor.retro_pos == 1)
			accur_wrong (real_chr);
		tutor_beep ();
	}

	if (tutor.correcting > 0)
		tutor.correcting--;

	/*
	 * Go forward and test end of text
	 */
	if (cursor_advance (1) != 1)
	{
		if (tutor.retro_pos == 0)
			return (FALSE);
		tutor.retro_pos--;
	}

	/*
	 * Test end of paragraph
	 */

	if (cursor_get_char () == L'\n')
		if (cursor_advance (1) != 1 && tutor.retro_pos == 0)
			return (FALSE);

	return (TRUE);
}


/**********************************************************************
 * Calculate the final results
 */
void
tutor_calc_stats ()
{
	guint i = 0;
	gint minutes;
	gint seconds;
	gboolean may_log = TRUE;
	gdouble accuracy;
	gdouble touchs_per_second;
	gdouble velocity;
	gdouble fluidness;
	gdouble standard_deviation = 0;
	gdouble sum;
	gdouble average = 0;
	gdouble sample;
	gchar *contest_ps = NULL;
	gchar *tmp_locale;
	gchar *tmp_str = NULL;
	gchar *tmp_str2 = NULL;
	gchar *tmp_name;
	gchar *tmp;
	FILE *fh;
	time_t tmp_time;
	struct tm *ltime;
	GtkWidget *wg;
	GtkTextBuffer *buf;
	GtkTextIter start;
	GtkTextIter end;
	Statistics stat;

	/*
	 * Calculate statistics
	 */
	minutes = ((gulong) tutor.elapsed_time) / 60;
	seconds = ((gulong) tutor.elapsed_time) % 60;

	accuracy = 100 * (1.0 - (gfloat) tutor.n_errors / tutor.n_touchs);
	touchs_per_second = (gdouble) (tutor.n_touchs - tutor.n_errors) / tutor.elapsed_time;
	velocity = 12 * touchs_per_second; // touched: new_WPM = 1.2 old_WPM

	if (tutor.type == TT_FLUID)
	{
		/*
		 * "Magic" fluidness calculation
		 */
		sum = 0;
		for (i = 2; i < tutor.ttidx; i++)
		{
			if (tutor.touch_time[i] <= 0)
				tutor.touch_time[i] = 1.0e-8;
			sample = sqrt (1 / tutor.touch_time[i]);
			sum += sample;
		}
		if (i == 2)
			i++;
		average = sum / (i - 2);

		sum = 0;
		for (i = 2; i < tutor.ttidx; i++)
		{
			sample = sqrt (1 / tutor.touch_time[i]);
			sum += (sample - average) * (sample - average);
		}
		if (i < 4)
			i = 4;
		standard_deviation = sqrt (sum / (i - 3));

		if (average <= 0)
			average = 1.0e-9;
		fluidness = 100 * (1 - standard_deviation / average);
		if (fluidness < 2)
			fluidness = 2;
	}
	else
		fluidness = 0;
	stat.score = 0;

	/* Verify if logging is allowed 
	 */
	may_log = TRUE;
	if (tutor.type == TT_FLUID)
		if (tutor.n_touchs < MIN_CHARS_TO_LOG)
		{
			gdk_beep ();
			gdk_beep ();
			contest_ps =
				g_strdup_printf (_
						 ("ps.: logging not performed for this session: "
						  "the number of typed characters (%i) must be greater than %i."),
						 tutor.n_touchs, MIN_CHARS_TO_LOG);
			may_log = FALSE;
		}
	if (may_log)
	{
		/*
		 * Changing to "C" locale: remember to copy the previous value!
		 */
		tmp_locale = g_strdup (setlocale (LC_NUMERIC, NULL));
		if (tmp_locale != NULL)
			setlocale (LC_NUMERIC, "C");

		/*
		 * Logging
		 */
		tmp_name =
			g_strconcat (main_path_stats (), G_DIR_SEPARATOR_S "stat_", tutor_get_type_name (), ".txt",
				     NULL);
		assert_user_dir ();
		if (!(fh = (FILE *) g_fopen (tmp_name, "r")))
		{
			fh = (FILE *) g_fopen (tmp_name, "w");
			if (tutor.type == TT_BASIC)
				fprintf (fh, "Accuracy\tVelocity\tFluidness\tDate\tHour\tLesson\tKeyboard\n");
			else if (tutor.type == TT_ADAPT)
				fprintf (fh, "Accuracy\tVelocity\tFluidness\tDate\tHour\tKeyboard\tLanguage\n");
			else
				fprintf (fh, "Accuracy\tVelocity\tFluidness\tDate\tHour\tLesson\tLanguage\n");
		}
		else
		{
			fclose (fh);
			fh = (FILE *) g_fopen (tmp_name, "a");
		}
		if (fh)
		{
			tmp_time = time (NULL);
			ltime = localtime (&tmp_time);
			fprintf (fh,
				 "%.2f\t%.2f\t%.2f\t%i-%2.2i-%2.2i\t%2.2i:%2.2i\t",
				 accuracy, velocity, fluidness,
				 (ltime->tm_year) + 1900, (ltime->tm_mon) + 1,
				 (ltime->tm_mday), (ltime->tm_hour), (ltime->tm_min));
			switch (tutor.type)
			{
			case TT_BASIC:
				fprintf (fh, "%2.2i\t", basic_get_lesson ());
				break;
			case TT_ADAPT:
				tmp = g_strdup (keyb_get_name ());
				for (i=0; tmp[i]; i++)
					tmp[i] = (tmp[i] == ' ') ? '_' : tmp[i];
				fprintf (fh, "%s\t", tmp);
				g_free (tmp);
				break;
			case TT_VELO:
				tmp = g_strdup (velo_get_dict_name ());
				for (i=0; tmp[i]; i++)
					tmp[i] = (tmp[i] == ' ') ? '_' : tmp[i];
				fprintf (fh, "%s\t", tmp);
				g_free (tmp);
				break;
			case TT_FLUID:
				tmp = g_strdup (fluid_get_paragraph_name ());
				for (i=0; tmp[i]; i++)
					tmp[i] = (tmp[i] == ' ') ? '_' : tmp[i];
				fprintf (fh, "%s\t", tmp);
				g_free (tmp);
				break;
			}

			if (tutor.type == TT_BASIC)
			{
				tmp = g_strdup (keyb_get_name ());
				for (i=0; tmp[i]; i++)
					tmp[i] = (tmp[i] == ' ') ? '_' : tmp[i];
				fprintf (fh, "%s\n", tmp );
				g_free (tmp);
			}
			else
				fprintf (fh, "%s\n", trans_get_current_language ());

			fclose (fh);
		}
		else
			g_message ("not able to log on this file:\n %s", tmp_name);
		g_free (tmp_name);

		if (tutor.type == TT_FLUID)
		{
			/* Log the fluidness results of the last session
			 */
			tmp_name = g_build_filename (main_path_stats (), "deviation_fluid.txt", NULL);
			if ((fh = (FILE *) g_fopen (tmp_name, "w")))
			{
				g_message ("writing further fluidness results at:\n %s", tmp_name);
				fprintf (fh,
					 "(i)\tdt(i)\tsqrt(1/dt(i))\tAverage:\t%g\tStd. dev.:\t%g\n",
					 average, standard_deviation);
				for (i = 1; i < tutor.ttidx; i++)
					fprintf (fh, "%i\t%g\t%g\n", i, tutor.touch_time[i],
						 sqrt (1 /
						       (tutor.touch_time[i] >
							0 ? tutor.touch_time[i] : 1.0e-9)));
				fclose (fh);
			}
			else
				g_message ("not able to log on this file:\n %s", tmp_name);
			g_free (tmp_name);

			/* Add results to Top 10
			 */
			tmp_name = main_preferences_get_string ("interface", "language");
			stat.lang[0] = ((tmp_name[0] == 'C') ? 'e' : tmp_name[0]);
			stat.lang[1] = ((tmp_name[0] == 'C') ? 'n' : tmp_name[1]);
			stat.genv = (UNIX_OK ? 'x' : 'w');
			stat.when = time (NULL);
			stat.nchars = tutor.n_touchs;
			stat.accur = accuracy;
			stat.velo = velocity;
			stat.fluid = fluidness;
			stat.score = top10_calc_score (&stat);

			g_free (tmp_name);
			tmp = main_preferences_get_string ("tutor", "keyboard");
			tmp_name = g_strdup_printf ("%s [%s]", g_get_real_name (), tmp);
			g_free (tmp);
			stat.name_len = strlen (tmp_name);
			if (stat.name_len > MAX_NAME_LEN)
				stat.name_len = MAX_NAME_LEN;
			strncpy (stat.name, tmp_name, stat.name_len + 1);
			g_free (tmp_name);

			top10_read_stats (LOCAL, -1);
			if (tutor_char_distribution_approved ())
			{
				if (top10_compare_insert_stat (&stat, LOCAL))
				{
					contest_ps =
						g_strdup (_
							  ("ps.: you have entered the Top 10 list, great!"));
					top10_write_stats (LOCAL, -1);
					//if (main_preferences_get_boolean ("game", "autopublish") && UNIX_OK)
					if (main_preferences_get_boolean ("game", "autopublish"))
					{
						top10_show_stats (LOCAL);
						top10_show_stats (GLOBAL);
						top10_global_publish (NULL);
					}
				}
			}
			else
				contest_ps = g_strdup (_("ps.: the text you just typed doesn't seem to be similar"
						   " to ordinary texts in the language currently selected:"
						   " we can't account for it in the 'Top 10' contest."));

			/* Anyway, log also the scoring
			 */
			tmp_name =
				g_build_filename (main_path_stats (), "scores_fluid.txt", NULL);
			assert_user_dir ();
			if (!g_file_test (tmp_name, G_FILE_TEST_IS_REGULAR))
			{
				fh = (FILE *) g_fopen (tmp_name, "w");
				fprintf (fh, "Score\tDate\tTime\tNumber of chars\tLanguage\n");
			}
			else
				fh = (FILE *) g_fopen (tmp_name, "a");
			if (fh)
			{
				ltime = localtime (&stat.when);
				fprintf (fh,
					 "%3.4f\t%i-%2.2i-%2.2i\t%2.2i:%2.2i\t%i\t%s\n",
					 stat.score, (ltime->tm_year) + 1900, (ltime->tm_mon) + 1,
					 (ltime->tm_mday), (ltime->tm_hour), (ltime->tm_min),
					 stat.nchars, trans_get_current_language ());
				fclose (fh);
			}
			else
				g_message ("not able to log on this file:\n %s", tmp_name);
			g_free (tmp_name);
		}

		/*
		 * Coming back to the right locale
		 */
		if (tmp_locale != NULL)
		{
			setlocale (LC_NUMERIC, tmp_locale);
			g_free (tmp_locale);
		}
	}

	/*
	 * Print statistics
	 */
	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));

	/* Begin the accuracy */
	;
	tmp_str = g_strconcat ("\n", _("STATISTICS"), "\n",
			       _("Elapsed time:"), " %i ",
			       dngettext (PACKAGE, "minute and", "minutes and", minutes),
			       " %i ", dngettext (PACKAGE, "second", "seconds", seconds),
			       "\n", _("Error ratio:"), " %i/%i\n", _("Accuracy:"), " ", NULL);

	tmp_str2 = g_strdup_printf (tmp_str, minutes, seconds, tutor.n_errors, tutor.n_touchs);
	g_free (tmp_str);
	gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

	/* Paint the accuracy */
	g_free (tmp_str2);
	tmp_str2 = g_strdup_printf ("%.1f%%", accuracy);
	gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

	gtk_text_buffer_get_end_iter (buf, &start);
	gtk_text_buffer_get_end_iter (buf, &end);
	gtk_text_iter_backward_cursor_positions (&start, strlen (tmp_str2));
	if (accuracy > tutor_goal_accuracy ())
		gtk_text_buffer_apply_tag_by_name (buf, "char_correct", &start, &end);
	else
		gtk_text_buffer_apply_tag_by_name (buf, "char_wrong", &start, &end);

	// Finish the accuracy
	g_free (tmp_str2);
	tmp_str2 = g_strdup_printf ("\t\t%s %.0f%%\n", _("Goal:"), tutor_goal_accuracy ());
	gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

	if (tutor.type == TT_VELO || tutor.type == TT_FLUID)
	{
		// Begin the CPS
		g_free (tmp_str2);
		tmp_str2 = g_strdup_printf ("%s ", _("Characters per second:"));
		gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

		// Paint the CPS
		g_free (tmp_str2);
		tmp_str2 = g_strdup_printf ("%.2f", velocity / 12);
		gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

		gtk_text_buffer_get_end_iter (buf, &start);
		gtk_text_buffer_get_end_iter (buf, &end);
		gtk_text_iter_backward_cursor_positions (&start, strlen (tmp_str2));
		if (velocity > tutor_goal_speed ())
			gtk_text_buffer_apply_tag_by_name (buf, "char_correct", &start, &end);
		else
			gtk_text_buffer_apply_tag_by_name (buf, "char_wrong", &start, &end);

		// Finish the CPS
		g_free (tmp_str2);
		tmp_str2 = g_strdup_printf ("\t\t%s %.1f %s\n", _("Goal:"), tutor_goal_speed () / 12, _("(CPS)"));
		gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

		// Begin the WPM
		g_free (tmp_str2);
		tmp_str2 = g_strdup_printf ("%s ", _("Words per minute:"));
		gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

		// Paint the WPM
		g_free (tmp_str2);
		tmp_str2 = g_strdup_printf ("%.1f", velocity);
		gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

		gtk_text_buffer_get_end_iter (buf, &start);
		gtk_text_buffer_get_end_iter (buf, &end);
		gtk_text_iter_backward_cursor_positions (&start, strlen (tmp_str2));
		if (velocity > tutor_goal_speed ())
			gtk_text_buffer_apply_tag_by_name (buf, "char_correct", &start, &end);
		else
			gtk_text_buffer_apply_tag_by_name (buf, "char_wrong", &start, &end);

		// Finish the WPM
		g_free (tmp_str2);
		tmp_str2 = g_strdup_printf ("\t\t%s %.0f %s\n", _("Goal:"), tutor_goal_speed (), _("(WPM)"));
		gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

		if (tutor.type == TT_FLUID)
		{
			// Begin the fluidity
			g_free (tmp_str2);
			tmp_str2 = g_strdup_printf ("%s ", _("Fluidness:"));
			gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

			// Paint the fluidity
			g_free (tmp_str2);
			tmp_str2 = g_strdup_printf ("%.1f%%", fluidness);
			gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

			gtk_text_buffer_get_end_iter (buf, &start);
			gtk_text_buffer_get_end_iter (buf, &end);
			gtk_text_iter_backward_cursor_positions (&start, strlen (tmp_str2));
			if (fluidness > tutor_goal_fluidity ())
				gtk_text_buffer_apply_tag_by_name (buf, "char_correct", &start, &end);
			else
				gtk_text_buffer_apply_tag_by_name (buf, "char_wrong", &start, &end);

			// Finish the fluidity and scores
			g_free (tmp_str2);
			tmp_str2 = g_strdup_printf ("\t\t%s %.0f%%\n%s: %f\n",
				       	_("Goal:"), tutor_goal_fluidity (),
				       	_("Score"), stat.score);
			gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));
		}
	}

	// Begin the comments
	g_free (tmp_str2);
	tmp_str2 = g_strdup_printf ("\n%s\n", _("Comments:"));
	gtk_text_buffer_insert_at_cursor (buf, tmp_str2, strlen (tmp_str2));

	switch (tutor.type)
	{
	case TT_BASIC:
		basic_comment (accuracy);
		break;
	case TT_ADAPT:
		adapt_comment (accuracy);
		break;
	case TT_VELO:
		velo_comment (accuracy, velocity);
		break;
	case TT_FLUID:
		fluid_comment (accuracy, velocity, fluidness);

		if (contest_ps != NULL)
		{
			//if (UNIX_OK)
			{
				gtk_text_buffer_insert_at_cursor (buf, "\n", 1);
				gtk_text_buffer_insert_at_cursor (buf, contest_ps, strlen (contest_ps));
			}
			g_free (contest_ps);
		}
		break;
	}
	g_free (tmp_str2);

	gtk_text_view_scroll_mark_onscreen (GTK_TEXT_VIEW (wg), gtk_text_buffer_get_insert (buf));
}

/**********************************************************************
 * Ensure the user is not trying to type with weird texts in the fluidness contest
 */
#define DECEIVENESS_LIMIT 0.195 // 0.135
gboolean
tutor_char_distribution_approved ()
{
	guint i, j;
	gfloat deceiveness;
	gchar *tmp_code;
	gchar *tmp_name;

	struct MODEL
	{
		gchar *text;
		Char_Distribution dist;
	} model;

	struct EXAM
	{
		gchar *text;
		Char_Distribution dist;
	} exam;

	GtkWidget *wg;
	GtkTextBuffer *buf;
	GtkTextIter start;
	GtkTextIter end;

	/* Get model text
	 */
	tmp_code = main_preferences_get_string ("interface", "language");
	tmp_name = g_strconcat (main_path_data (), G_DIR_SEPARATOR_S, tmp_code, ".paragraphs", NULL);
	g_free (tmp_code);
	if (!g_file_get_contents (tmp_name, &model.text, NULL, NULL))
	{
		g_free (tmp_name);
		tmp_name = trans_lang_get_similar_file_name (".paragraphs");
		if (!g_file_get_contents (tmp_name, &model.text, NULL, NULL))
		{
			g_message ("Can't read file:\n %s\n So, not logging your score.", tmp_name);
			g_free (tmp_name);
			return FALSE;
		}
	}

	/* Get text under examination
	 */
	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));
	gtk_text_buffer_get_bounds (buf, &start, &end);
	exam.text = gtk_text_buffer_get_text (buf, &start, &end, FALSE);

	/* Get char distributions
	 */
	tutor_char_distribution_count (model.text, &model.dist);
	tutor_char_distribution_count (exam.text, &exam.dist);

	/* Compare both distributions
	 */
	deceiveness = 0;
	for (i = 0; i < 9 && deceiveness < 1.0e+6; i++)
	{
		for (j = 0; j < exam.dist.size; j++)
			if (model.dist.ch[i].letter == exam.dist.ch[j].letter)
			{
				deceiveness +=
					powf ((exam.dist.ch[j].freq - model.dist.ch[i].freq), 2);
				break;
			}
		if (j == exam.dist.size)
		{
			deceiveness += 1.0e+7;
			break;
		}
	}
	deceiveness = sqrtf (deceiveness / 9);

	g_print ("Corpus file: %s\n", tmp_name);
	if (deceiveness < DECEIVENESS_LIMIT)
		g_print ("\tDeviation: %.3f. OK, it is less than %.3f.\n", deceiveness, DECEIVENESS_LIMIT);
	else
		g_print ("\tDeviation: %.3f! It should be less than %.3f.\n", deceiveness, DECEIVENESS_LIMIT);

	g_free (tmp_name);
	g_free (model.text);
	g_free (exam.text);
	return (deceiveness < DECEIVENESS_LIMIT);
}

/**********************************************************************
 * Count relative frequency of letters in text
 */
void
tutor_char_distribution_count (gchar * text, Char_Distribution * dist)
{
	gchar *pt;
	gunichar ch;
	gsize i, j;

	pt = text;

	dist->size = 0;
	dist->total = 0;
	while ((ch = g_utf8_get_char (pt)) != L'\0')
	{
		/* Only count letters
		 */
		if (!g_unichar_isalpha (ch))
		{
			pt = g_utf8_next_char (pt);
			continue;
		}
		ch = g_unichar_tolower (ch);

		/* Verify if ch was already counted
		 */
		for (i = 0; i < dist->size; i++)
		{
			if (ch == dist->ch[i].letter)
			{
				dist->ch[i].count++;
				dist->total++;
				break;
			}
		}

		/* If ch was not counted yet, start to do it
		 */
		if (i == dist->size && i < MAX_ALPHABET_LEN)
		{
			dist->ch[dist->size].letter = ch;
			dist->ch[dist->size].count = 1;
			dist->total++;
			dist->size++;
		}

		pt = g_utf8_next_char (pt);
	}

	/* Sort the list
	 */
	for (i = 1; i < dist->size; i++)
	{
		gunichar aletter;
		guint acount;

		if (dist->ch[i].count > dist->ch[i - 1].count)
			for (j = i; j > 0; j--)
			{
				if (dist->ch[j].count <= dist->ch[j - 1].count)
					break;

				aletter = dist->ch[j - 1].letter;
				dist->ch[j - 1].letter = dist->ch[j].letter;
				dist->ch[j].letter = aletter;

				acount = dist->ch[j - 1].count;
				dist->ch[j - 1].count = dist->ch[j].count;
				dist->ch[j].count = acount;
			}
	}

	/* Write the relative frequency
	 */
	for (i = 0; i < dist->size; i++)
		dist->ch[i].freq = ((gfloat) dist->ch[i].count) / ((gfloat) dist->ch[0].count);

	/*
	   for (i = 0; i < dist->size; i++)
	   g_message ("Char: %x, count: %u, freq:%g", dist->ch[i].letter, dist->ch[i].count, dist->ch[i].freq);
	   g_message ("Total: %u  / Size: %u ------------------------------", dist->total, dist->size);
	 */
}

/**********************************************************************
 * Formats and draws one paragraph at the tutor window
 */
void
tutor_draw_paragraph (gchar * utf8_text)
{
	static gchar *tmp1 = NULL;
	static gchar *tmp2 = NULL;
	gchar *ptr;
	GtkWidget *wg;
	GtkTextBuffer *buf;

	g_free (tmp1);
	g_free (tmp2);

	if (g_utf8_strrchr (utf8_text, -1, L'\n') == NULL)
	{
		g_message ("paragraph not terminated by carriage return: adding one.");
		tmp1 = g_strconcat (utf8_text, "\n", NULL);
	}
	else
		tmp1 = g_strdup (utf8_text);

	ptr = g_utf8_strrchr (tmp1, -1, L'\n');
	if (ptr)
		*ptr = '\0';
	else
		g_error ("draw_paragraph () ==> string error");

	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));

	tmp2 = g_strconcat (tmp1, keyb_get_utf8_paragraph_symbol (), "\n", NULL);

	gtk_text_buffer_insert_at_cursor (buf, tmp2, -1);
}

/**********************************************************************
 * Load the list of files to include in the set of "other exercises"
 */
void
tutor_load_list_other (gchar * file_name_end, GtkListStore * list)
{
	gchar *tmp_str;
	gchar *dentry;
	GDir *dir;
	GtkTreeIter iter;
	static gchar *defstr = NULL;

	if (defstr == NULL)
		defstr = g_strdup (OTHER_DEFAULT);

	gtk_list_store_clear (list);
	gtk_list_store_append (list, &iter);
	gtk_list_store_set (list, &iter, 0, defstr, -1);

	assert_user_dir ();
	dir = g_dir_open (main_path_user (), 0, NULL);
	while ((dentry = g_strdup (g_dir_read_name (dir))) != NULL)
	{
		if (strlen (dentry) < 5)
		{
			g_free (dentry);
			continue;
		}
		if (!(tmp_str = strrchr (dentry, '.')))
		{
			g_free (dentry);
			continue;
		}
		if (! g_str_equal (file_name_end, tmp_str))
		{
			g_free (dentry);
			continue;
		}

		*(strrchr (dentry, '.')) = '\0';
		gtk_list_store_append (list, &iter);
		gtk_list_store_set (list, &iter, 0, dentry, -1);
		g_free (dentry);
	}
	g_dir_close (dir);

	gtk_widget_set_sensitive (get_wg ("button_other_remove"), FALSE);
	gtk_widget_set_sensitive (get_wg ("label_other_rename"), FALSE);
	gtk_widget_set_sensitive (get_wg ("entry_other_rename"), FALSE);
	gtk_widget_set_sensitive (get_wg ("button_other_apply"), FALSE);
}

void
tutor_other_rename (const gchar *new_tx, const gchar *old_tx)
{
	if (! g_str_equal (new_tx, old_tx) && 
	    ! g_str_equal (new_tx, OTHER_DEFAULT) &&
	    ! g_str_equal (new_tx, "") &&
	    g_strrstr (old_tx, "*") == NULL )
	{
		gchar *old_name;
		gchar *new_name;
		gchar *old_file;
		gchar *new_file;

		if (tutor.type == TT_VELO)
		{
			old_name = g_strconcat (old_tx, ".words", NULL);
			new_name = g_strconcat (new_tx, ".words", NULL);
		}
		else
		{
			old_name = g_strconcat (old_tx, ".paragraphs", NULL);
			new_name = g_strconcat (new_tx, ".paragraphs", NULL);
		}
		old_file = g_build_filename (main_path_user (), old_name, NULL);
		new_file = g_build_filename (main_path_user (), new_name, NULL);

		if (g_file_test (new_file, G_FILE_TEST_IS_REGULAR))
		{
			g_message ("File already exists, not renaming.\n\t%s\n", new_file);
			gdk_beep ();
		}
		else
		{
			g_printf ("Renaming from:\n\t%s\nTo:\n\t%s\n", old_file, new_file);
			if (g_rename (old_file, new_file))
			{
				g_printf ("Fail: %s\n", strerror (errno));
			}
			else
				g_printf ("Success!\n");
		}
		g_free (old_name);
		g_free (new_name);
		g_free (old_file);
		g_free (new_file);
	}
}

/**********************************************************************
 * Put 'mesg' in the message entry line of the shared tutor window
 */
void
tutor_message (gchar * mesg)
{
	gint pos = 0;
	GtkWidget *wg;

	if (mesg == NULL)
	{
		g_message ("tutor_message() --> not showing NULL message!");
		return;
	}

	wg = get_wg ("entry_mesg");
	callbacks_shield_set (TRUE);
	gtk_editable_delete_text (GTK_EDITABLE (wg), 0, -1);
	gtk_editable_insert_text (GTK_EDITABLE (wg), g_strdup (mesg), strlen (mesg), &pos);
	gtk_editable_set_position (GTK_EDITABLE (wg), -1);
	callbacks_shield_set (FALSE);
}

/**********************************************************************
 * Beeps (or not) at the user, in the tutor window
 */
void
tutor_beep ()
{
	GtkWidget *wg;

	wg = get_wg ("togglebutton_tutor_beep");
	if (gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wg)))
		gdk_beep ();
}

/**********************************************************************
 * Speak some phrase
 */ 
void
tutor_speak_string (gchar *string, gboolean wait)
{
	gchar *tmp_code;
	gchar *command;
	static gboolean espeak_OK = TRUE;
	static GtkWidget *wg = NULL;

	if (wg == NULL)
		wg = get_wg ("checkbutton_speech");

	if (espeak_OK == FALSE)
		return;
	if (!gtk_widget_get_visible (wg) )
		return;
	if (!gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (wg)) )
		return;

	/* Translators: your language code (first 2 letters of your po-file)*/
	tmp_code = g_strdup (_("en"));
	tmp_code[2] = '\0';
	if (wait)
	{
		command = g_strdup_printf ("espeak -v%s -k1 \"%s\"", tmp_code, string);
#ifdef G_OS_UNIX
		espeak_OK = g_spawn_command_line_sync (command, NULL, NULL, NULL, NULL);
#else
		espeak_OK = ! system (command);
#endif
	}
	else
	{
#ifdef G_OS_UNIX
		if (g_utf8_strlen (string, -1) == 1 || tutor.type == TT_VELO)
			command = g_strdup_printf ("espeak -v%s -k1 --punct '%s'", tmp_code, string);
		else
			command = g_strdup_printf ("espeak -v%s -k1 \"%s\"", tmp_code, string);
		espeak_OK = g_spawn_command_line_async (command, NULL);
#else
		if (g_utf8_strlen (string, -1) == 1 || tutor.type == TT_VELO)
			command = g_strdup_printf ("espeak -v%s -k1 --punct \"%s\"", tmp_code, string);
		else
			command = g_strdup_printf ("espeak -v%s -k1 \"%s\"", tmp_code, string);
		espeak_OK = ! system (command);
#endif
	}
	if (espeak_OK == FALSE)
		g_message ("Espeak not installed, so we'll say nothing:\n %s", command);
	g_free (tmp_code);
	g_free (command);
}

/**********************************************************************
 * Control delayed tips for the finger to be used
 */ 
gboolean
tutor_delayed_finger_tip (gpointer unich)
{
	gchar *finger;
	gunichar *uch = (gunichar*) unich;
	static gint counter = 0;

	if (unich == NULL)
	{
		counter++;
		return FALSE;
	}
	counter--;

	if (counter > 0)
		return FALSE;

	finger = hints_finger_name_from_char (*uch);
	tutor_speak_string (finger, TRUE);
	g_free (finger);

	return FALSE;
}

/**********************************************************************
 * Speak the current character to be typed
 */
void
tutor_speak_char ()
{
	gchar ut8[100];
	static gunichar uch;

	if (tutor.type == TT_BASIC)
	{
		g_timeout_add (3000, (GSourceFunc) tutor_delayed_finger_tip, (gpointer) &uch);
		tutor_delayed_finger_tip (NULL);
	}

	uch = cursor_get_char ();
	switch (uch)
	{
	case L' ':
		strcpy (ut8, _("space"));
		break;
	case L'y':
	case L'Y':
		/* Translators: the name of letter Y */
		strcpy (ut8, _("wye"));
		break;
	case UPSYM:
		/* Translators: the name of the Return key */
		strcpy (ut8, _("enter"));
		break;
	case L'%':
		strcpy (ut8, "%");
		break;
	case L'\'':
		strcpy (ut8, _("apostrophe"));
		break;
	case L'\"':
		/* Translators: double quote symbol: " */
		strcpy (ut8, _("quote"));
		break;
	case L'&':
		/* Translators: ampersand symbol: & */
		strcpy (ut8, _("ampersand"));
		break;
	default:
		ut8[g_unichar_to_utf8 (uch, ut8)] = '\0';
	}

	tutor_speak_string (ut8, FALSE);

}

/**********************************************************************
 * Speak the next word to be typed
 */
void
tutor_speak_word ()
{
	gunichar uch[100];
	gchar *ut8;
	gint i;

	if (cursor_advance (-1) != -1)
		uch[0] = L' ';
	else
	{
		uch[0] = cursor_get_char ();
		cursor_advance (1);
	}
	if (uch[0] == L' ' || uch[0] == UPSYM || uch[0] == L'\n' || uch[0] == L'\r')
	{
		for (i = 0; i < 100; i++)
		{
			uch[i] = cursor_get_char ();
			if (uch[i] == L' ' || uch[i] == UPSYM || uch[i] == L'\n' || uch[i] == L'\r')
				break;
			cursor_advance (1);
		} 
		cursor_advance (-i);
	}
	else
		return;

	ut8 = g_ucs4_to_utf8 (uch, i, NULL, NULL, NULL);
	if (ut8)
		tutor_speak_string (ut8, FALSE);
	g_free (ut8);
}
