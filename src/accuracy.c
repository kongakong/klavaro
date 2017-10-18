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

#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include <glib.h>
#include <glib/gstdio.h>

#include "main.h"
#include "auxiliar.h"
#include "keyboard.h"
#include "adaptability.h"
#include "accuracy.h"

/* Touch errors
 */
static struct
{
	gunichar uchr;
	gulong wrong;
	gulong correct;
} terror[MAX_CHARS_EVALUATED];
static gint terror_n = 0;

/* Touch times
 */
static struct
{
	gunichar uchr;
	gdouble dt[MAX_TT_SAVED];
	gint idx;
} ttime[MAX_CHARS_EVALUATED];
static gint ttime_n = 0;

/* Simple reset
 */
void
accur_terror_reset ()
{
	memset (&terror, 0, sizeof (terror));
	terror_n = 0;
}

void
accur_ttime_reset ()
{
	memset (&ttime, 0, sizeof (ttime));
	ttime_n = 0;
}

void
accur_reset ()
{
	accur_terror_reset ();
	accur_ttime_reset ();
}

gint
accur_terror_n_get (void)
{
	return (terror_n);
}

gint
accur_ttime_n_get (void)
{
	return (ttime_n);
}

gchar *
accur_terror_char_utf8 (gint i)
{
	gchar *utf8;
	gint n;

	if (i < 0 || i >= terror_n)
		return (g_strdup (" "));

	utf8 = g_malloc (UTF8_BUFFER);
	n = g_unichar_to_utf8 (terror[i].uchr, utf8);
	if (n < 1)
		return (g_strdup (" "));
	utf8[n] = '\0';

	return (utf8);
}

gchar *
accur_ttime_char_utf8 (gint i)
{
	gchar *utf8;
	gint n;

	if (i < 0 || i >= ttime_n)
		return (g_strdup (" "));

	utf8 = g_malloc (UTF8_BUFFER);
	n = g_unichar_to_utf8 (ttime[i].uchr, utf8);
	if (n < 1)
		return (g_strdup (" "));
	utf8[n] = '\0';

	return (utf8);
}

gulong
accur_wrong_get (gint i)
{
	if (i < 0 || i >= terror_n)
		return -1;

	return (terror[i].wrong);
}

/**********************************************************************
 * Open the accuracy accumulators
 */
void
accur_init ()
{
	const gchar delim[] = "\t\n\r()";
	gint i, j;
	gchar *tmp;
	gchar *kb_name;
	gchar *data;
	gboolean success;
	gunichar uchr;
	gulong wrong;
	gulong correct;
	gdouble dt;

	accur_reset ();

	kb_name = g_strdup (keyb_get_name ());
	for (i=0; kb_name[i]; i++)
		kb_name[i] = (kb_name[i] == ' ') ? '_' : kb_name[i];

	/*
	 * First, the accuracy log
	 */
	tmp = g_strconcat (main_path_stats (), DIRSEP_S, ACCUR_LOG_FILE, "_", kb_name, NULL);
	success = g_file_get_contents (tmp, &data, NULL, NULL);
	if (!success)
	{
		g_message ("Empty accuracy log: %s", tmp);
		g_free (tmp);
	}
	else
	{
		g_free (tmp);
		tmp = strtok (data, delim);
		for (i = 0; i < MAX_CHARS_EVALUATED; i++)
		{
			if (tmp == NULL)
				break;
			uchr = g_utf8_get_char_validated (tmp, -1);
			if (uchr == (gunichar)-1 || uchr == (gunichar)-2) 
				break;

			tmp = strtok (NULL, delim);
			if (tmp == NULL)
				break;
			wrong = strtoul (tmp, NULL, 10);

			tmp = strtok (NULL, delim);
			if (tmp == NULL)
				break;
			correct = strtoul (tmp, NULL, 10);
			
			if (wrong > 0 && correct <= ERROR_INERTIA)
			{
				terror[i].uchr = uchr;
				terror[i].wrong = wrong;
				terror[i].correct = correct;
				terror_n = i + 1;
			}
			else
				break;

			tmp = strtok (NULL, delim);
		}
		g_free (data);
	}

	/*
	 * Second, the proficiency log
	 */
	tmp = g_strconcat (main_path_stats (), DIRSEP_S, PROFI_LOG_FILE, "_", kb_name, NULL);
	success = g_file_get_contents (tmp, &data, NULL, NULL);
	if (!success)
	{
		g_message ("Empty proficiency log: %s", tmp);
		g_free (tmp);
	}
	else
	{
		g_free (tmp);
		tmp = strtok (data, delim);
		for (i = 0; i < MAX_CHARS_EVALUATED; i++)
		{
			if (tmp == NULL)
				break;
			uchr = g_utf8_get_char_validated (tmp, -1);
			if (uchr == (gunichar)-1 || uchr == (gunichar)-2) 
				break;

			tmp = strtok (NULL, delim);
			if (tmp == NULL)
				break;
			dt = strtod (tmp, NULL);

			if (dt > 0)
			{
				ttime[i].uchr = uchr;
				for (j = 0; j < 10; j++)
					ttime[i].dt[j] = dt;
				ttime[i].idx = 11;
				ttime_n = i + 1;
			}
			else
				break;

			/* Dummy normalized value
			 */
			tmp = strtok (NULL, delim);
			if (tmp == NULL)
				break;

			tmp = strtok (NULL, delim);
		}
		g_free (data);
	}
	g_free (kb_name);
}

/**********************************************************************
 * Accumulates correctly typed characters
 */
void
accur_correct (gunichar uchr, double touch_time)
{
	gint i, j;

	if (uchr == L' ' || uchr == UPSYM || uchr == L'\t' || uchr == L'\b')
		return;
	if (!keyb_is_inset (uchr))
		return;

	/*
	 * First, accuracy
	 */
	for (i = 0; i < terror_n; i++)
	{
		if (uchr == terror[i].uchr)
		{
			if (terror[i].correct > ERROR_INERTIA)
			{
				terror[i].wrong -= (terror[i].wrong == 0 ? 0 : 1);
				terror[i].correct = 1;
			}
			else
				terror[i].correct++;
			break;
		}
	}

	/*
	 * Second, proficiency
	 */
	if (touch_time < 0.001)
		return;

	uchr = g_unichar_tolower (uchr);
	for (i = 0; i < ttime_n; i++)
	{
		if (uchr == ttime[i].uchr)
		{
			ttime[i].dt[ttime[i].idx] = touch_time;
			if (++ttime[i].idx == MAX_TT_SAVED)
				ttime[i].idx = 0;
			return;
		}
	}
	if (i >= MAX_CHARS_EVALUATED)
	{
		for (i = MAX_CHARS_EVALUATED - 1; i >= 0; i--)
		{
			if (touch_time > ttime[i].dt[0])
				break;
		}
		if (i < 0)
			return;
		for (j = 1; j < MAX_TT_SAVED; j++)
			ttime[i].dt[j] = 0;
	}
	ttime[i].uchr = uchr;
	ttime[i].dt[0] = touch_time;
	ttime[i].idx = 1;
	ttime_n = i + 1;
}

/**********************************************************************
 * Accumulates mistyped characters
 */
void
accur_wrong (gunichar uchr)
{
	gint i;
	gint i_min = -1;
	gdouble correct_min = 1e9;
	gdouble hlp;

	if (uchr == L' ' || uchr == UPSYM || uchr == L'\t' || uchr == L'\b')
		return;
	if (!keyb_is_inset (uchr))
		return;

	/*
	 * Only for accuracy
	 */
	for (i = 0; i < terror_n; i++)
	{
		if (uchr == terror[i].uchr)
		{
			terror[i].wrong++;
			return;
		}
		hlp = ((gdouble) terror[i].wrong) / ((gdouble) terror[i].correct + terror[i].wrong + 1);
		if (hlp <= correct_min)
		{
			correct_min = hlp;
			i_min = i;
		}
	}

	if (terror_n < MAX_CHARS_EVALUATED)
	{
		i = terror_n;
		terror_n++;
	}
	else
	{
		i = (i_min > -1 ? i_min : terror_n - 1);
	}

	terror[i].uchr = uchr;
	terror[i].wrong = 1;
	terror[i].correct = 1;
}

gulong
accur_error_total ()
{
	gint i;
	gulong n = 0;

	for (i = 0; i < terror_n; i++)
		n += (terror[i].wrong < 12345 ? terror[i].wrong : 0);

	return n;
}

gdouble
accur_profi_aver (gint idx)
{
	gint i, n;
	gdouble sum = 0;

	if (idx < 0)
		return -10;

	n = 0;
	for (i = 0; i < MAX_TT_SAVED; i++)
	{
		if (ttime[idx].dt[i] > 0 && ttime[idx].dt[i] < 2)
		{
			sum += ttime[idx].dt[i];
			n++;
		}
	}
	if (n == 0)
		return -1;
	return (sum / n);
}

gint
accur_profi_aver_norm (gint idx)
{
	gint norm;

	if (idx < 0)
		return 1;

	norm = rint (accur_profi_aver (idx) / accur_profi_aver (ttime_n-1));
	
	return norm;
}

/*******************************************************************************
 * Sorting first: decreasing wrongness; second: increasing correctness
 */
void
accur_terror_sort ()
{
	gint i, j;
	gunichar uchr;
	gulong correct;
	gulong wrong;

	for (i = 1; i < terror_n; i++)
	{
		for (j = i; j > 0; j--)
		{
			if (terror[j].correct < terror[j-1].correct)
			{
				uchr = terror[j].uchr;
				terror[j].uchr = terror[j-1].uchr;
				terror[j-1].uchr = uchr;

				wrong = terror[j].wrong;
				terror[j].wrong = terror[j-1].wrong;
				terror[j-1].wrong = wrong;

				correct = terror[j].correct;
				terror[j].correct = terror[j-1].correct;
				terror[j-1].correct = correct;
			}
		}
	}
	for (i = 1; i < terror_n; i++)
	{
		for (j = i; j > 0; j--)
		{
			if (terror[j].wrong > terror[j-1].wrong)
			{
				uchr = terror[j].uchr;
				terror[j].uchr = terror[j-1].uchr;
				terror[j-1].uchr = uchr;

				wrong = terror[j].wrong;
				terror[j].wrong = terror[j-1].wrong;
				terror[j-1].wrong = wrong;

				correct = terror[j].correct;
				terror[j].correct = terror[j-1].correct;
				terror[j-1].correct = correct;
			}
		}
	}
}

/*******************************************************************************
 * Decreasing order, touch time 
 */
void
accur_ttime_sort ()
{
	gint i, j, k;
	gunichar uchr;
	gdouble dt;
	gint idx;

	for (i = 1; i < ttime_n; i++)
	{
		for (j = i; j > 0; j--)
		{
			if (accur_profi_aver (j) > accur_profi_aver (j-1))
			{
				uchr = ttime[j].uchr;
				ttime[j].uchr = ttime[j-1].uchr;
				ttime[j-1].uchr = uchr;

				idx = ttime[j].idx;
				ttime[j].idx = ttime[j-1].idx;
				ttime[j-1].idx = idx;

				for (k = 0; k < MAX_TT_SAVED; k++)
				{
					if (ttime[j].dt[k] == ttime[j-1].dt[k])
						continue;
					dt = ttime[j].dt[k];
					ttime[j].dt[k] = ttime[j-1].dt[k];
					ttime[j-1].dt[k] = dt;
				}
			}
		}
	}

	for (i = ttime_n - 1; i > -1; i--)
	{
		if (accur_profi_aver (i) < 0.001)
		{
			ttime[i].uchr = 0;
			for (j = 0; j < MAX_TT_SAVED; j++)
				ttime[i].dt[j] = 0;
			ttime[i].idx = 0;
			ttime_n--;
		}
		else
			break;
	}
}

void
accur_sort ()
{
	accur_terror_sort ();
	accur_ttime_sort ();
}

/*******************************************************************************
 * Creates a random weird word based on error profile
 */
gboolean
accur_create_word (gunichar word[MAX_WORD_LEN + 1])
{
	gint i, j;
	gint ind;
	gint n;
	gunichar vowels[20];
	gunichar last = 0;
	gint vlen;
	gboolean ptype_terror;
	gboolean ptype_ttime;
	static gint profile_type = 0;

	if (terror_n < 10 && ttime_n < 10)
		return FALSE;

	ptype_terror = accur_error_total () >= ERROR_LIMIT;
	ptype_ttime = accur_profi_aver_norm (0) >= PROFI_LIMIT;
	vlen = keyb_get_vowels (vowels);
	n = rand () % (MAX_WORD_LEN) + 1;
	for (i = 0; i < n; i++)
	{
		if (profile_type == 0)
			profile_type = ptype_ttime ? 1 : 0;
		else
			profile_type = ptype_terror ? 0 : 1;

		if (profile_type == 0) /* Error */
		{
			for (j = 0; j < 100; j++)
			{
				ind = rand () % terror_n;
				if (terror[ind].uchr == last)
					continue;
				if (rand () % terror[0].wrong < terror[ind].wrong)
					break;
			}
			word[i] = terror[ind].uchr;
		}
		else /* Time */
		{
			for (j = 0; j < 100; j++)
			{
				ind = rand () % ttime_n;
				if (ttime[ind].uchr == last)
					continue;
				if (rand () % accur_profi_aver_norm (0) < accur_profi_aver_norm (ind))
					break;
			}
			word[i] = ttime[ind].uchr;
		}
		last = word[i];

		/* Avoid double diacritics
		 */
		if (i > 0)
			if (keyb_is_diacritic (word[i - 1]) && keyb_is_diacritic (word[i]))
			{
				word[i] = vowels[rand () % vlen];
				last = word[i];
			}
	}
	/*
	 * Null terminated unistring
	 */
	word[n] = L'\0';

	return TRUE;
}

/*******************************************************************************
 * Saves the accuracy accumulator
 */
void
accur_close ()
{
	gint i;
	gchar *kb_name;
	gchar *tmp;
	gchar *utf8;
	FILE *fh;

	accur_sort ();

	kb_name = g_strdup (keyb_get_name ());
	for (i=0; kb_name[i]; i++)
		kb_name[i] = (kb_name[i] == ' ') ? '_' : kb_name[i];

	/*
	 * First, the accuracy log
	 */
	tmp = g_strconcat (main_path_stats (), DIRSEP_S, ACCUR_LOG_FILE, "_", kb_name, NULL);
	fh = g_fopen (tmp, "wb");
	g_free (tmp);
	if (fh)
	{
		for (i = 0; i < terror_n; i++)
		{
			if (terror[i].wrong == 0 || terror[i].correct > ERROR_INERTIA)
				continue;

			utf8 = accur_terror_char_utf8 (i);
			g_fprintf (fh, "%s\t%lu\t%lu\n", utf8, terror[i].wrong, terror[i].correct);
			g_free (utf8);
		}
		fclose (fh);
	}
	else
		g_message ("Could not save an accuracy log file at %s", main_path_stats ());

	/*
	 * Second, the proficiency log
	 */
	tmp = g_strconcat (main_path_stats (), DIRSEP_S, PROFI_LOG_FILE, "_", kb_name, NULL);
	fh = g_fopen (tmp, "wb");
	g_free (tmp);
	if (fh)
	{
		for (i = 0; i < ttime_n; i++)
		{
			utf8 = accur_ttime_char_utf8 (i);
			g_fprintf (fh, "%s", utf8);
			g_free (utf8);
			g_fprintf (fh, "\t%g\t%.2f\n",
					accur_profi_aver (i),
					accur_profi_aver (i) / accur_profi_aver (ttime_n-1));
		}
		fclose (fh);
	}
	else
		g_message ("Could not save a proficiency log file at %s", main_path_stats ());

	g_free (kb_name);
}
