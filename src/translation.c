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
 * Set of functions to deal with internationalization (translation).
 */
#include <stdio.h>
#include <string.h>
#include <locale.h>
#include <glib.h>
#include <glib/gstdio.h>
#include <gtk/gtk.h>

#include "auxiliar.h"
#include "main.h"
#include "callbacks.h"
#include "keyboard.h"
#include "velocity.h"
#include "fluidness.h"
#include "translation.h"

/**********************************************************************
 * Variables
 */
static Lang_Name_Code *lang;
static gint lang_num = 0;

/**********************************************************************
 * Get country name from its "ISO code" (eo and xx are just languages...)
 */
const gchar *
trans_code_to_country (gchar *code)
{
#	define COUNTRY_N 50
	gsize i;
	gchar *dummy = NULL;
	static gchar map[COUNTRY_N][2][64] = {
		{"xx","Esperantio"},
		{"ad","Andorra"},
		{"ar","العالم العربي"},
		{"be","België"},
		{"bg","България"},
		{"br","Brasil"},
		{"ca","Canada"},
		{"ch","Schweiz / Suisse"},
		{"cn","中华人民共和国"},
		{"cz","Česká republika"}, /* 10 */
		{"dk","Danmark"},
		{"de","Deutschland"},
		{"eo","Esperantujo"},
		{"es","España"},
		{"eu","Euskal Herria"},
		{"fi","Suomi"},
		{"fr","France"},
		{"gr","Ελλάδα"},
		{"il","ישראל"},
		{"hr","Hrvatska"}, /* 20 */
		{"hu","Magyarország"},
		{"in","India"},
		{"it","Italia"},
		{"jp","日本 (Nippon)"},
		{"kk","Қазақстан"},
		{"kr","대한민국"}, /* Korea */
		{"no","Norge"},
		{"pl","Polska"},
		{"pk","پاکستان"},
		{"pt","Portugal"}, /* 30 */
		{"rs","Србија"}, /* Serbia */
		{"ru","Россия"},
		{"sl","Slovenija"},
		{"se","Sverige"},
		{"tr","Türkiye"},
		{"ua","Україна"},
		{"uk","United Kingdom"},
		{"us","USA"},
		{"",""},
		{"",""}, /* 40 */
		{"",""},
		{"",""},
		{"",""},
		{"",""},
		{"",""},
		{"",""},
		{"",""},
		{"",""},
		{"",""},
		{"",""}
	};

	for (i = 0; i < COUNTRY_N; i++)
		if (g_str_equal (code, map[i][0]))
			return (map[i][1]);
	dummy = g_strdup_printf ("(%s)", code);
	return (dummy);
}

/**********************************************************************
 * Get a 'reazonable' value for keyboard, that is, QWERTY... :-(
 */
gchar *
trans_get_default_keyboard ()
{
	gint i;
	gchar *tmp;

	if (lang_num == 0)
	{
		g_warning ("Internal error: trying to use language data not initialized!");
		return (NULL);
	}

	tmp = main_preferences_get_string ("interface", "language");
	for (i = 0; i < lang_num; i++)
		if (g_str_equal (lang[i].code, tmp))
		{
			g_free (tmp);
			return (lang[i].kbd);
		}

	g_free (tmp);
	return (NULL);
}

/**********************************************************************
 * Initialize 'lang', 'code' and 'kbd' accordingly to 'language_set',
 * which was defined in "languages.h".
 */
void
trans_init_lang_name_code ()
{
	static gboolean init = FALSE;
	gint i;
	gchar *tmp;
	gchar **temp_lang = NULL;
	const gchar languages_set[] = LANG_SET;

	if (init)
	{
		g_warning ("Not initializing again the language data");
		return;
	}
	init = TRUE;

	/* Number of configured languages
	 */
	temp_lang = g_strsplit (languages_set, "\n", -1);
	i = 0;
	while (temp_lang[i] != NULL)
		i++;
	g_assert (i > 0);
	lang = g_new (Lang_Name_Code, i);
	lang_num = i;

	for (i = 0; i < lang_num; i++)
	{
		gchar *end;
		gchar *begin;

		tmp = g_strdup (temp_lang[i]);
		/* Initialize 'lang'
		 */
		end = strchr (tmp, '(');
		if (end)
		       	end -= ( ((end - tmp) > 1) ? 1 : 0 );
		else
			g_error ("Internal lang error: found nothing like '(LL...'");
		lang[i].name = g_strndup (tmp, end - tmp);

		/* Initialize 'code'
		 */
		begin = strchr (end, '(') + 1;
		end = strchr (begin, ')');
		if (end == NULL)
			g_error ("Internal lang error: found nothing like '(LL)'");
		lang[i].code = g_strndup (begin, end - begin);

		/* Initialize 'cd'
		 */
		if (lang[i].code[0] == 'C')
			strcpy (lang[i].cd, "en");
		else
			strncpy (lang[i].cd, lang[i].code, 2);
		lang[i].cd[2] = '\0';

		/* Initialize 'kbd'
		 */
		begin = strchr (end, '[') + 1;
		end = strchr (begin, ']');
		if (end == NULL)
			g_error ("Internal lang error: found nothing like '[yy_zz]'");
		lang[i].kbd = g_strndup (begin, end - begin);

		//g_printf ("%s : %s : %s : %s\n", lang[i].name, lang[i].code, lang[i].cd, lang[i].kbd);
	}
	g_strfreev (temp_lang);
}

gchar *
trans_get_code (gint i)
{
	if (i >= lang_num || i < 0)
		return (NULL);
	return (lang[i].cd);
}

gboolean
trans_lang_is_available (gchar * langcode)
{
	gint i;

	for (i = 0; i < lang_num; i++)
		if (g_str_equal (lang[i].code, langcode))
			break;
	return (i == lang_num ? FALSE : TRUE);
}

/**********************************************************************
 * Define if we may put a stop mark at the end of "phrases".
 */
gboolean
trans_lang_has_stopmark ()
{
	gboolean stopmark;
	gchar *hlp;

	hlp = main_preferences_get_string ("interface", "language");
	stopmark = g_str_has_prefix (hlp, "ur") ||
		   g_str_has_prefix (hlp, "ar") ||
		   g_str_has_prefix (hlp, "bn") ||
		   g_str_has_prefix (hlp, "pa");
	g_free (hlp);

	return (!stopmark);
}

/**********************************************************************
 * Private auxiliar function
 */
static gboolean
trans_lang_get_similar (gchar * test)
{
	gint i;
	gchar aux_code_2[3];

	if (g_str_equal (test, "C"))
		return TRUE;

	strncpy (aux_code_2, test, 2);
	aux_code_2[2] = '\0';

	for (i = 0; i < lang_num; i++)
	{
		if (strstr (lang[i].code, aux_code_2))
		{
			g_free (test);
			test = g_strdup (lang[i].code);
			break;
		}
	}
	if (i == lang_num && g_str_has_prefix (test, "en"))
	{
		g_free (test);
		test = g_strdup ("C");
		return (TRUE);
	}
	return (i == lang_num ? FALSE : TRUE);
}

/**********************************************************************
 * Get the current locale and change it if necessary
 */
void
trans_init_language_env ()
{
	gchar *tmp_code;
	gboolean lang_ok;
	gint i;

	/*
	 * If the language is already set in preferences, just use it
	 */
	lang_ok = FALSE;
	if (main_preferences_exist ("interface", "language"))
	{
		lang_ok = TRUE;
		tmp_code = main_preferences_get_string ("interface", "language");
		if (trans_lang_is_available (tmp_code) == FALSE)
		{
			tmp_code[2] = '\0';
			if (trans_lang_is_available (tmp_code) == FALSE)
			{
				lang_ok = FALSE;
				main_preferences_remove ("interface", "language");
			}
			else
				main_preferences_set_string ("interface", "language", tmp_code);
		}
	}

	if (lang_ok == FALSE)
	{
		/* 
		 * Read the current locale
		 */
#ifdef G_OS_UNIX
		i = 0;
		while ((tmp_code = g_strdup (g_get_language_names ()[i])))
		{
			if (tmp_code[0] == 'C')
			{
				lang_ok = (i == 0 ? TRUE : FALSE);
				break;
			}
			lang_ok = trans_lang_is_available (tmp_code);
			if (lang_ok == TRUE)
				break;
			g_free (tmp_code);
			lang_ok = FALSE;
			i++;
		}
		if (lang_ok == FALSE)
		{
			i = 0;
			while ((tmp_code = g_strdup (g_get_language_names ()[i])))
			{
				if (tmp_code[0] == 'C')
				{
					lang_ok = (i == 0 ? TRUE : FALSE);
					break;
				}
				lang_ok = trans_lang_get_similar (tmp_code);
				if (lang_ok == TRUE)
					break;
				g_free (tmp_code);
				lang_ok = FALSE;
				i++;
			}
		}
#else
		tmp_code = g_win32_getlocale ();
		lang_ok = trans_lang_is_available (tmp_code);
		if (lang_ok == FALSE)
			lang_ok = trans_lang_get_similar (tmp_code);
#endif
	}
	if (tmp_code == NULL)
		tmp_code = g_strdup ("xx");

	/* If even a similar is not available...
	 */
	if (lang_ok == FALSE)
	{
		g_message ("as your locale (%s) isn't available, "
			       "we are using \"C\"", tmp_code);
		g_free (tmp_code);
		tmp_code = g_strdup ("C");
	}

#ifdef G_OS_WIN32
	g_setenv ("LANGUAGE", tmp_code, TRUE);
#endif
	main_preferences_set_string ("interface", "language", tmp_code);
	g_free (tmp_code);
}

/**********************************************************************
 * Inserts the list of available languages in the 'combo_language'.
 */
void
trans_set_combo_language ()
{
	static gboolean recur = FALSE;
	gint i;
	gint i_env;
	gchar *tmp_code;
	gchar *langcode;
	GtkComboBox *cmb;

	callbacks_shield_set (TRUE);

	if (recur)
		cmb = GTK_COMBO_BOX (get_wg ("combobox_top10_language"));
	else
		cmb = GTK_COMBO_BOX (get_wg ("combobox_language"));

	tmp_code = main_preferences_get_string ("interface", "language");
	if (tmp_code == NULL || g_str_equal ("en_US", tmp_code))
	{
		g_message ("Using \"C\" as language code.");
		main_preferences_set_string ("interface", "language", "C");
		if (tmp_code)
			g_free (tmp_code);
		tmp_code = g_strdup ("C");
	}
	gtk_combo_box_text_remove (GTK_COMBO_BOX_TEXT (cmb), 0);
	for (i = 0, i_env = -1; i < lang_num; i++)
	{
		langcode = g_strdup_printf ("%s (%s)", lang[i].name, lang[i].code);
		gtk_combo_box_text_append_text (GTK_COMBO_BOX_TEXT (cmb), langcode);
		if (g_str_equal (lang[i].code, tmp_code))
			i_env = i;
		else if (g_str_has_prefix (tmp_code, lang[i].code))
		{
			i_env = i;
			main_preferences_set_string ("interface", "language", lang[i].code);
		}
		g_free (langcode);
	}
	if (i_env == -1)
	{
		g_warning ("set_combo_language() ==> the locale \"%s\" is not available!", tmp_code);
		g_message ("Using \"C\" as language code.");
		g_free (tmp_code);
		tmp_code = g_strdup ("C");
		main_preferences_set_string ("interface", "language", "C");
		gtk_combo_box_set_active (cmb, 8);
	}
	else
		gtk_combo_box_set_active (cmb, i_env);

	callbacks_shield_set (FALSE);

	if (! recur)
	{
		recur = TRUE;
		trans_set_combo_language ();
		return;
	}
	recur = FALSE;

	if (g_str_has_prefix (tmp_code, "en") || g_str_equal (tmp_code, "C"))
		gtk_widget_show (get_wg ("checkbutton_speech"));
	else
		gtk_widget_hide (get_wg ("checkbutton_speech"));

	g_free (tmp_code);
}

/**********************************************************************
 * Get the current language name, mapped from the preference's key code
 */
gchar *
trans_get_current_language ()
{
	gchar *tmp_code;
	gint i;

	if (lang_num == 0)
	{
		g_warning ("Internal error: trying to use language data not initialized!");
		return (NULL);
	}

	tmp_code = main_preferences_get_string ("interface", "language");
	for (i = 0; i < lang_num; i++)
		if (g_str_equal (lang[i].code, tmp_code))
		{
			g_free (tmp_code);
			return (lang[i].name);
		}
	g_free (tmp_code);
	return ("??");
}

/**********************************************************************
 * Update the current language used accordingly to that selected in the
 * 'combo_language'
 */
void
trans_change_language (gchar *language)
{
	gint i;
	gchar *tmp_code;

	/* Keep decreasing order of scanning, for not missing "English UK", for instance. */
	for (i = lang_num-1; i >= 0; i--)
		if (g_str_has_prefix (language, lang[i].name))
			break;

	if (i == lang_num)
	{
		g_warning ("change_language() --> couldn't find the language: %s", language);
		return;
	}

	main_preferences_set_string ("interface", "language", lang[i].code);

	velo_reset_dict ();
	fluid_reset_paragraph ();

	/* Check if the interface language is the same of the selected in the combo,
	 * so that it may be spoken
	 */
	if (lang[i].code[0] == 'C')
		tmp_code = g_strdup ("en");
	else
		tmp_code = g_strdup (lang[i].code);
	if (tmp_code[0] == _("en")[0] && tmp_code[1] == _("en")[1])
		gtk_widget_show (get_wg ("checkbutton_speech"));
	else
		gtk_widget_hide (get_wg ("checkbutton_speech"));
	g_free (tmp_code);
}

/**********************************************************************
 * Find a file whose language prefix is similar to the current one
 */
FILE *
trans_lang_get_similar_file (const gchar * file_end)
{
	gint i;
	gchar *tmp_code;
	gchar *tmp_path = NULL;
	FILE *fh = NULL;

	tmp_code = main_preferences_get_string ("interface", "language");
	for (i = 0; i < lang_num && fh == NULL; i++)
	{
		if (g_str_equal (lang[i].code, tmp_code))
			continue;
		if (lang[i].code[0] == tmp_code[0] && lang[i].code[1] == tmp_code[1])
		{
			g_free (tmp_path);
			tmp_path =
				g_strconcat (main_path_data (), G_DIR_SEPARATOR_S, lang[i].code, file_end, NULL);
			fh = (FILE *) g_fopen (tmp_path, "r");
		}
	}
	g_free (tmp_code);
	g_free (tmp_path);
	i--;
	if (fh)
		g_message ("applying similar file: %s%s", lang[i].code, file_end);
	return (fh);
}

/**********************************************************************
 * Find a file whose language prefix is similar to the current one, returnig
 * its name
 */
gchar *
trans_lang_get_similar_file_name (const gchar * file_end)
{
	gint i;
	gchar *tmp_code;
	gchar *tmp_path = NULL;

	tmp_code = main_preferences_get_string ("interface", "language");
	for (i = 0; i < lang_num; i++)
	{
		if (g_str_equal (lang[i].code, tmp_code))
			continue;
		if (lang[i].code[0] == tmp_code[0] && lang[i].code[1] == tmp_code[1])
		{
			g_free (tmp_path);
			tmp_path =
				g_strconcat (main_path_data (), G_DIR_SEPARATOR_S, lang[i].code, file_end, NULL);
			if (g_file_test (tmp_path, G_FILE_TEST_IS_REGULAR))
				break;
		}
	}
	if (tmp_path == NULL)
		tmp_path = g_strconcat (main_path_data (), G_DIR_SEPARATOR_S, "C", file_end, NULL);
	g_free (tmp_code);
	return (tmp_path);
}

/**********************************************************************
 * Reads the text of a file to be presented at some dialog,
 * accordingly to the environment language.
 * 'text': new buffer where the text is copied into.
 * 'file_end': how the file name ends
 */

gchar *
trans_read_text (const gchar * file_end)
{
	gchar *buf;
	gchar *tmp_name = NULL;
	gchar *tmp_path = NULL;
	gchar *tmp_code;
	FILE *fh;

	gchar *basic1 = _(
"The basic course focuses on having you read the characters presented to you on screen "
"and typing the corresponding keys. Remember to keep your hands correctly oriented "
"on the home row of the keyboard at all times (see introduction on main menu).");
	gchar *basic2 = _(
"The key set used in each series will be shown in the above message line. "
"The [Space], [Shift] and [Enter] keys may not show up there but are used very often.");
	gchar *basic3 = _(
"The message line below follows and echoes your key presses. "
"If required, it changes and displays instructions for actions required from you.");

	gchar *adapt1 = _(
"Here you may practice and improve your memorization of all keys. "
"There will be sentences presented with nonsense words which mix some numbers and symbols.");
	gchar *adapt2 = _(
"In order to keep the lesson contents language and keyboard independent, "
"accented letter combinations will probably not appear. For real word sentences, "
"please use the fourth option of the main menu (about fluidness).");
	gchar *adapt3 = _(
"After each exercise there will be a brief statistics panel "
"reviewing your performance along with some relevant comments.");

	gchar *velo1 = _(
"This exercise is very similar to the second one, for adaptability. "
"The difference is that here you'll type real words.");
	gchar *velo2 = _(
"The default language is the actual one of the interface. "
"But you may select any other texts with words you would like to use. "
"Press the 'Other' option above and add files containing those texts.");
	gchar *velo3 = _(
"With this exercise the focus is on speed. "
"So, you are supposed to type really fast and I will only flatter you when you deserve it!");

	gchar *fluid1 = _(
"We will now use complete sentences and paragraphs which make logical sense. "
"This may distract you while you type if you try to understand what you are entering. "
"The previous exercises were aimed at getting you to type without interpreting and analyzing the content.");
	gchar *fluid2 = _(
"We do not mean to imply that the typists must behave like a robot, without understanding what they type. "
"We do aim to develop the skill of typing, making it an automatic reflex akin to the acts of walking, talking, etc. "
"After reaching this goal, the act of typing will become automatic and require little concentration. "
"Then you will be able to pay attention to the real meaning of the text.");
	gchar *fluid3 = _("These exercises are longer. Each exercise consists of three paragraphs and "
"the emphasis is placed on correctness and rhythm, with a minimum speed requirement. "
"Here you will be required to use the backspace key to correct any mistakes. "
"In other words, only input without error will be accepted.");

	if (g_str_equal (file_end, "_basic_intro.txt"))
		return (g_strdup_printf ("%s\n%s\n%s", basic1, basic2, basic3));

	if (g_str_equal (file_end, "_adapt_intro.txt"))
		return (g_strdup_printf ("%s\n%s\n%s", adapt1, adapt2, adapt3));

	if (g_str_equal (file_end, "_velo_intro.txt"))
		return (g_strdup_printf ("%s\n%s\n%s", velo1, velo2, velo3));

	if (g_str_equal (file_end, "_fluid_intro.txt"))
		return (g_strdup_printf ("%s\n%s\n%s", fluid1, fluid2, fluid3));

	/* Use text files
	 */
	tmp_code = main_preferences_get_string ("interface", "language");
	tmp_name = g_strconcat (tmp_code, file_end, NULL);

	/* Try at HOME
	 */
	tmp_path = g_build_filename (main_path_user (), tmp_name, NULL);
	fh = (FILE *) g_fopen (tmp_path, "r");

	/* Try at PACKAGE_DATA
	 */
	if (fh == NULL)
	{
		g_free (tmp_path);
		tmp_path = g_build_filename (main_path_data (), tmp_name, NULL);
		fh = (FILE *) g_fopen (tmp_path, "r");
	}

	/*
	 * Try other "flavors" of the same language
	 */
	if (fh == NULL && strlen (tmp_code) > 1)
		fh = trans_lang_get_similar_file (file_end);

	/*
	 * Default to C
	 */
	if (fh == NULL && ! g_str_equal (tmp_code, "C"))
	{
		g_message ("trans_read_text() --> couldn't open the data file: %s\n"
			 " So, we have to apply the default one: C%s", tmp_name, file_end);
		main_preferences_set_string ("interface", "language", "C");
		buf = trans_read_text (file_end);
		main_preferences_set_string ("interface", "language", tmp_code);
		g_free (tmp_code);
		g_free (tmp_path);
		g_free (tmp_name);
		return buf;
	}

	if (fh == NULL)
		g_error ("trans_read_text() --> couldn't open the data file:\n %s", tmp_name);

	g_free (tmp_code);
	g_free (tmp_path);
	g_free (tmp_name);

	/*
	 * Process the file
	 */
	gsize bufsize = 16;
	gsize pos = 0;
	buf = g_malloc (bufsize);
	while (1)
	{
		gsize max = bufsize - pos - 1; // -1 for terminating zero
		gsize ct = fread (buf + pos, 1, max, fh);
		if (ct == 0)
			break;
		pos += ct;
		if (ct == max)
		{
			bufsize *= 2;
			buf = g_realloc (buf, bufsize);
		}
	}
	buf[pos] = 0;
	fclose (fh);
	return buf;
}
