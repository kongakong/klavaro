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

#include <plot.h>
#include <glib.h>

#define ACCUR_LOG_FILE "accuracy.log"
#define PROFI_LOG_FILE "proficiency.log"
#define MAX_CHARS_EVALUATED DATA_POINTS /* DATA_POINTS is in plot.h (50) */
#define MAX_TT_SAVED 100
#define ERROR_INERTIA 10 /* It was 30 before... */
#define ERROR_LIMIT 150 /* It was 200 before... */
#define PROFI_LIMIT 5
#define UTF8_BUFFER 7

void accur_init (void);
void accur_terror_reset (void);
void accur_ttime_reset (void);
void accur_reset (void);
gint accur_terror_n_get (void);
gint accur_ttime_n_get (void);
gchar * accur_terror_char_utf8 (gint i);
gchar * accur_ttime_char_utf8 (gint i);
gulong accur_wrong_get (gint i);
gdouble accur_profi_aver (gint i);
gint accur_profi_aver_norm (gint i);
void accur_correct (gunichar uchr, double touch_time);
void accur_wrong (gunichar uchr);
gulong accur_error_total (void);
void accur_terror_sort (void);
void accur_ttime_sort (void);
void accur_sort (void);
gboolean accur_create_word (gunichar *word);
void accur_close (void);
