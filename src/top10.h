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

/**************************************************
 * Contest for fluidness performance
 */
#define DOWNHOST "klavaro.sourceforge.net/top10"
#define CGI_SERVER "klavaro.sourceforge.net/cgi-bin/klavaro_rangilo"
#define MIN_CHARS_TO_LOG 500

#define TIMEOUT 10
#define LOW_SPEED_LIMIT 160
#define LOW_SPEED_TIME 5

/**************************************************
 * Structures
 */
typedef struct USERS
{
	gchar *id;
	gchar *name;
} User;

#define MAX_NAME_LEN 255
typedef struct STATISTICS
{
	gchar lang[2];		// Language code
	gchar genv;		// Graphical environment: 'x' = X; 'w' = Windows
	time_t when;		// Epoch of stats logging
	gint32 nchars;		// Number of chars typed in the test
	gfloat accur;
	gfloat velo;
	gfloat fluid;
	gfloat score;		// s = f (accur, velo, fluid)
	gint32 name_len;
	gchar name[MAX_NAME_LEN + 1];
} Statistics;

void top10_init (void);

void top10_message (gchar * msg);

void top10_init_stats (gboolean locally);

void top10_clean_stat (gint i, gboolean locally);

void top10_insert_stat (Statistics * stat, gint i, gboolean locally);

gboolean top10_compare_insert_stat (Statistics * stat, gboolean locally);

void top10_delete_stat (gint i, gboolean locally);

gfloat top10_calc_score (Statistics * stat);

gboolean top10_validate_stat (Statistics * stat);

gchar *top10_get_score_file (gboolean locally, gint lang);

gboolean top10_read_stats_from_file (gboolean locally, gchar * file);

void top10_read_stats (gboolean locally, gint lang);

void top10_write_stats (gboolean locally, gint lang);

void top10_show_stat (Statistics * stats);

void top10_show_stats (gboolean locally);

gboolean top10_global_update (gpointer data);

gboolean top10_global_publish (gpointer data);

