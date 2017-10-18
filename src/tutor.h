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

#ifndef TUTOR_H
# define TUTOR_H

/*
 * Special colors
 */
# define TUTOR_RED "#ff0044"
# define TUTOR_RED_LITE "#eeddbb"
# define TUTOR_GREEN "#228822"
# define TUTOR_YELLOW "#ffee00"
# define TUTOR_BROWN "#886600"
# define TUTOR_BLACK "#000000"
# define TUTOR_GRAY "#dddddd"
# define TUTOR_GRAY_LITE "#eeece8"
# define TUTOR_CREAM "#f8f4f2" /* "#f0f2ca" <== "#faf8f8" before */
# define TUTOR_WHITE "#fffefe"

typedef enum
{
	TT_BASIC,
	TT_ADAPT,
	TT_VELO,
	TT_FLUID
} TutorType;

typedef enum
{
	QUERY_INTRO,
	QUERY_START,
	QUERY_PROCESS_TOUCHS,
	QUERY_END
} TutorQuery;

#define MAX_ALPHABET_LEN 50
typedef struct CHAR_DISTRIBUTION
{
	struct CHARS
	{
		gunichar letter;
		guint count;
		gfloat freq;
	} ch[MAX_ALPHABET_LEN];
	guint size;
	guint total;
} Char_Distribution;

/*
 * Interface functions
 */
TutorType tutor_get_type (void);

gchar *tutor_get_type_name (void);

TutorQuery tutor_get_query (void);

void tutor_set_query (TutorQuery);

gint tutor_get_correcting (void);

void tutor_init_timers (void);

void tutor_init_goals (void);

gdouble tutor_goal_accuracy (void);

gdouble tutor_goal_speed (void);

gdouble tutor_goal_fluidity (void);

gdouble tutor_goal_level (guint n);

/*
 * Auxiliar functions
 */
void tutor_init (TutorType tutor_type);

void tutor_update (void);

void tutor_update_intro (void);

void tutor_update_start (void);

void tutor_process_touch (gunichar user_chr);

gboolean tutor_eval_forward (gunichar chr);

gboolean tutor_eval_forward_backward (gunichar chr);

void tutor_calc_stats (void);

gboolean tutor_char_distribution_approved (void);

void tutor_char_distribution_count (gchar * text, Char_Distribution * dist);

void tutor_draw_paragraph (gchar * text);

void tutor_load_list_other (gchar * file_name_end, GtkListStore * list);

void tutor_other_rename (const gchar *new_tx, const gchar *old_tx);

void tutor_message (gchar * mesg);

void tutor_beep (void);

gboolean tutor_delayed_finger_tip (gpointer unich);

void tutor_speak_string (gchar *string, gboolean wait);

void tutor_speak_char ();

void tutor_speak_word ();

#endif
