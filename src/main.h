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

#include <sys/stat.h>

#ifdef G_OS_UNIX
# define UNIX_OK TRUE
# define DIRSEP_S "/"
# define DIRSEP '/'
# define LESSON_FONT "Monospace 14"
# define NORMAL_FONT "Sans 14"
# define DIR_PERM (S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH)
#else
# define UNIX_OK FALSE
# define DIRSEP_S "\\"
# define DIRSEP '\\'
# define LESSON_FONT "Courier Bold 14"
# define NORMAL_FONT "Comic Sans MS 12"
# undef PACKAGE_LOCALE_DIR
# define PACKAGE_LOCALE_DIR "..\\share\\locale"
# undef PACKAGE_DATA_DIR
# define PACKAGE_DATA_DIR "..\\share"
# define DIR_PERM (0xFFFF)
#endif

#define LOCAL TRUE
#define GLOBAL FALSE

/*
 * Interface
 */
gchar *main_path_user (void);

gchar *main_path_stats (void);

gchar *main_path_data (void);

gchar *main_path_score (void);

gboolean main_curl_ok (void);

gboolean main_preferences_exist (gchar * group, gchar * key);

void main_preferences_remove (gchar * group, gchar * key);

gchar *main_preferences_get_string (gchar * group, gchar * key);

void main_preferences_set_string (gchar * group, gchar * key, gchar * value);

gint main_preferences_get_int (gchar * group, gchar * key);

void main_preferences_set_int (gchar * group, gchar * key, gint value);

gboolean main_preferences_get_boolean (gchar * group, gchar * key);

void main_preferences_set_boolean (gchar * group, gchar * key, gboolean value);

void main_preferences_save (void);

void main_window_pass_away (void);

