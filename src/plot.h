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
 * Number of points for charts
 */
#define DATA_POINTS 50
#define MAX_Y_LABELS 13

#define PLOT_GREEN "#ddffee"
#define PLOT_GREEN_2 "#66aa88"
#define PLOT_GREEN_3 "#aaeebb"
#define PLOT_RED "#ffeeee"
#define PLOT_RED_2 "#cc4466"
#define PLOT_RED_3 "#eeaaaa"
#define PLOT_PURPLE "#bb44ee"
#define PLOT_BLUE "#e8eeff"
#define PLOT_BLUE_2 "#6666bb"
#define PLOT_BLUE_3 "#b0b0ff"
#define PLOT_ORANGE "#fff8d0"
#define PLOT_ORANGE_2 "#eebb88"
#define PLOT_ORANGE_3 "#ffeeaa"

GtkWidget * plot_get_databox (void);

void plot_initialize (void);

void plot_draw_chart (gint field);

void plot_pointer_update (gdouble x);
