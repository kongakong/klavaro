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
 * Time for cursor blinkings, in miliseconds
 */
#define TIME_CURSOR_ON 800
#define TIME_CURSOR_OFF 400

/*
 * Interface functions
 */
gboolean cursor_get_blink (void);

void cursor_set_blink (gboolean status);

/*
 * Auxilaiar functions
 */
void cursor_paint_char (gchar * color_tag_name);

gint cursor_advance (gint n);

gunichar cursor_get_char (void);

gboolean cursor_init (gpointer data);

gboolean cursor_on (gpointer data);

gboolean cursor_off (gpointer data);

void cursor_switch_on (void);

void cursor_switch_off (void);
