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
 * This definition contains the set of languages configured for
 * localization.
 * The letters in parenthesis must agree with LL_CC,
 * where LL is the language code
 * and CC is the country code.
 * The only exception is for the generic
 * English, which uses the 'C' code.
 * Keep the alphabetic order, because it is put
 * in the interface as done here.
 */

#define LANG_SET \
"العربية (ar) [qwerty_ar] \n" \
"български език (bg) [dvorak_bg]\n" \
" বাংলা (bn) [qwerty_us]\n" \
"Bokmål (nb) [qwerty_no]\n" \
"Català (ca) [qwerty_es]\n" \
"Čeština (cs) [qwertz_cz]\n" \
"Dansk (da) [qwerty_dk]\n" \
"Deutsch (de) [qwertz_de]\n" \
"ελληνικά (el) [qwerty_gr]\n" \
"English (C) [qwerty_us]\n" \
"English UK (en_GB) [qwerty_uk]\n" \
"Esperanto (eo) [dvorak_eo_eurokeys]\n" \
"Español; Castellano (es) [qwerty_es]\n" \
"Euskara (eu) [qwerty_es]\n" \
"Français (fr) [azerty_fr]\n" \
"Galego (gl) [qwerty_es]\n" \
"Hrvatski (hr) [qwertz_hr]\n" \
"Italiano (it) [qwerty_it]\n" \
"Қазақ (kk) [jtsuken_kk]\n" \
"한국어 (ko) [dubeolsik_kr]\n" \
"Кыргызча (ky) [jtsuken_ru]\n" \
"Magyar (hu) [qwertz_hu]\n" \
"Nederlands (nl) [qwerty_us]\n" \
"ਪੰਜਾਬੀ (pa) [gumurkhi_in_jehlum]\n" \
"Polski (pl) [qwerty_pl_us]\n" \
"Português (pt_BR) [qwerty_br_abnt2]\n" \
"Русский (ru) [jtsuken_ru]\n" \
"Suomen kieli (fi) [qwerty_se]\n"\
"Slovenščina (sl) [qwertz_sl]\n" \
"Српски (sr) [qwertz_rs]\n" \
"Svenska (sv) [qwerty_se]\n" \
"తెలుగు (te) [qwerty_us]\n" \
"Українська (uk) [jtsuken_ua]\n" \
"اردو (ur) [qwerty_pk_crulp]\n" \
"Tiếng Việt (vi) [qwerty_us]\n" \
"Wolof (wo) [azerty_fr]\n" \
"简体字 (zh_CN) [qwerty_cn_us]"

#define LANG_NAME_MAX_LEN 60
#define KBD_NAME_MAX_LEN 20
typedef struct
{
	gchar *name;
	gchar *code;
	gchar cd[3];
	gchar *kbd;
} Lang_Name_Code;

void trans_init_lang_name_code (void);

const gchar * trans_code_to_country (gchar *code);

gchar * trans_get_default_keyboard (void);

gchar * trans_get_code (gint i);

gboolean trans_lang_is_available (gchar * test);

gboolean trans_lang_has_stopmark (void);

FILE *trans_lang_get_similar_file (const gchar * file_end);

gchar * trans_lang_get_similar_file_name (const gchar * file_end);

void trans_init_language_env (void);

void trans_set_combo_language (void);

gchar *trans_get_current_language (void);

void trans_change_language (gchar *language);

gchar *trans_read_text (const gchar *);
