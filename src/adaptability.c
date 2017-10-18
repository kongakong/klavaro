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
 * Adaptability exercise
 */
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <gtk/gtk.h>

#include "auxiliar.h"
#include "main.h"
#include "keyboard.h"
#include "tutor.h"
#include "translation.h"
#include "accuracy.h"
#include "adaptability.h"

/**********************************************************************
 * Writes a random pattern of weird words in the exercise window
 */
void
adapt_draw_random_pattern ()
{
	gint i, j, k;
	gint tidx;
	gchar *hlp;
	gchar *utf8_text;
	gunichar text[WORDS * (MAX_WORD_LEN + 1) + 3];
	gunichar word[MAX_WORD_LEN + 1];
	gboolean special;
	gboolean word_ok = FALSE;

	hlp = main_preferences_get_string ("interface", "language");

	special = accur_error_total () >= ERROR_LIMIT ||
		       	accur_profi_aver_norm (0) >= PROFI_LIMIT;
	if (special)
	{
		gtk_widget_show (get_wg ("togglebutton_toomuch_errors"));
		special = gtk_toggle_button_get_active (GTK_TOGGLE_BUTTON (get_wg ("togglebutton_toomuch_errors")));
	}
	else
		gtk_widget_hide (get_wg ("togglebutton_toomuch_errors"));

	for (i = 0; i < LINES; i++)
	{			/* paragraphs per exercise */
		tidx = 0;
		for (j = 0; j < WORDS; j++)
		{		/* words per paragraph */
			if (special)
				word_ok = accur_create_word (word);

			if (!special || !word_ok)
			{
				if (rand () % 15)
					adapt_create_word (word);
				else
					adapt_create_number (word);
			}

			if (j == 0)
				word[0] = keyb_unichar_toupper (word[0]);
			else
				text[tidx++] = L' ';

			for (k = 0; word[k] != L'\0'; k++)
				text[tidx++] = word[k];
		}
		if (g_str_has_prefix (hlp, "ur"))
			text[tidx++] = URDU_STOP;
		if (g_str_has_prefix (hlp, "pa"))
			text[tidx++] = DEVANAGARI_STOP;
		else if (trans_lang_has_stopmark ())
			text[tidx++] = L'.';
		text[tidx++] = L'\n';
		text[tidx++] = L'\0';
		utf8_text = g_ucs4_to_utf8 (text, -1, NULL, NULL, NULL);
		tutor_draw_paragraph (utf8_text);
		g_free (utf8_text);
	}

	g_free (hlp);
}

/*
 * Creates a random weird word
 */
void
adapt_create_word (gunichar word[MAX_WORD_LEN + 1])
{
	gchar *hlp;
	gint i, n;
	gint vlen, clen, slen;
	gunichar vowels[20];
	gunichar consonants[4 * KEY_LINE_LEN];
	gunichar symbols[4 * KEY_LINE_LEN];

	vlen = keyb_get_vowels (vowels);
	clen = keyb_get_consonants (consonants);
	slen = keyb_get_symbols (symbols);

	n = rand () % (MAX_WORD_LEN - 1) + 1;
	for (i = 0; i < n; i++)
	{
		if ((rand () % 25))
		{
			/* Literal */
			if (i % 2)	/* vowel */
				if (rand () % 30)
					word[i] = vowels[rand () % vlen];
				else
					word[i] = consonants[rand () % clen];
			else if (rand () % 50)	/* consonant */
				word[i] = consonants[rand () % clen];
			else
				word[i] = vowels[rand () % vlen];
			if (i == 0 && !(rand () % 7))	/* capital */
				word[0] = keyb_unichar_toupper (word[0]);
		}
		else
		{
			/* Symbol */
			word[i] = symbols[rand () % slen];
			if (word[i] == L'\\' && i > 0)
				word[i] = L'-';
			if (word[i] == L'´' && i > 0)
				word[i] = L'`';
			if (rand () % 5 || word[i] == L'-' || word[i] == L'\\')
			{	/* space after symbol ==> end of word (most often) */
				word[i + 1] = L'\0';
				return;
			}
		}

		/* Avoid double diacritics */
		if (i > 0)
			if (keyb_is_diacritic (word[i - 1]) && keyb_is_diacritic (word[i]))
				word[i] = vowels[rand () % vlen];
	}
	/*
	 * Last char
	 */
	if (rand () % 20)
		word[n] = vowels[rand () % vlen];
	else
	{
		hlp = main_preferences_get_string ("interface", "language");
		if (g_str_has_prefix (hlp, "ur"))
			word[n] = URDU_COMMA;
		else if (trans_lang_has_stopmark ())
			word[n] = L',';
		g_free (hlp);
	}

	/*
	 * Null terminated unistring
	 */
	word[n + 1] = L'\0';
}

/*
 * Creates a random number
 */
void
adapt_create_number (gunichar ucs4_word[MAX_WORD_LEN + 1])
{
	gint i;
	gint alen;
	gboolean arabic;
	const gchar digits[11] = "0123456789";
	gunichar altnums[30];

	arabic = TRUE;
	alen = keyb_get_altnums (altnums);
	if (alen > 5)
		arabic = rand () % 7 ? FALSE : TRUE;
	for (i = 0; i < 4; i++)
	{
		if (arabic)
			ucs4_word[i] = digits[rand () % 10];
		else
			ucs4_word[i] = altnums[rand () % alen];
	}
	ucs4_word[4] = L'\0';
}

/**********************************************************************
 * Put on the screen the final comments
 */
void
adapt_comment (gdouble accuracy)
{
	gchar *tmp_str;
	GtkWidget *wg;
	GtkTextBuffer *buf;

	/*
	 * Comments
	 */
	if (accuracy < tutor_goal_level(0))
		tmp_str = g_strdup (":-(\n");
	else if (accuracy < tutor_goal_level(1))
		tmp_str = g_strdup_printf (_(" Your accuracy rate is below %.0f%%...\n"
					" Could you please try again to improve it?\n"), tutor_goal_level(1));
	else if (accuracy < tutor_goal_level(2))
		tmp_str = g_strdup_printf (_(" You are doing well. But...\n"
					" Could you make the accuracy reach %.0f%%?\n"), tutor_goal_level(2));
	else if (accuracy < tutor_goal_accuracy ())
		tmp_str = g_strdup_printf (_(" You are almost there,"
				" but your accuracy rate is still below %.0f%%.\n"
				" Try a few more times,"
				" or maybe you're getting upset, so go to another kind of exercise.\n"),
			      	tutor_goal_accuracy ());
	else
	{
		tmp_str = g_strdup_printf (_(" Very good!\n"
			      " You succeeded with an accuracy rate above %.0f%%.\n"
			      " Now it is time to increase your velocity.\n"
			      " Go to the 3rd exercise at the main menu.\n"),
			      tutor_goal_accuracy ());

		accur_terror_reset ();
		/*
		gtk_widget_hide (get_wg ("togglebutton_toomuch_errors"));
		*/
	}

	wg = get_wg ("text_tutor");
	buf = gtk_text_view_get_buffer (GTK_TEXT_VIEW (wg));
	gtk_text_buffer_insert_at_cursor (buf, tmp_str, strlen (tmp_str));
	g_free (tmp_str);
}
