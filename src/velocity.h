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

#define MAX_WORDS 12123

/*
 * Interface functions
 */
gchar *velo_get_dict_name (void);

void velo_reset_dict (void);

/*
 * Auxiliar functions
 */
void velo_init (void);

void velo_init_dict (gchar *);

void velo_draw_random_words (void);

gchar *velo_filter_utf8 (gchar * text);

void velo_text_write_to_file (gchar * text_raw, gboolean overwrite);

void velo_create_dict (gchar * file_name, gboolean overwrite);

void velo_comment (gdouble accuracy, gdouble velocity);
