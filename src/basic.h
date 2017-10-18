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

#define	MAX_BASIC_LESSONS 256

/*
 * Interface functions
 */
gint basic_get_lesson (void);

void basic_set_lesson (gint lesson);

gunichar *basic_get_char_set (void);

gboolean basic_get_lesson_increased (void);

void basic_set_lesson_increased (gboolean state);

/*
 * Auxiliar functions
 */
void basic_init (void);

gint basic_init_char_set (void);

void basic_save_lesson (gchar * charset);

void basic_draw_lesson (void);

void basic_comment (gdouble accuracy);
