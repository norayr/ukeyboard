/*
 *  Copyright (c) 2008 Jiri Benc <jbenc@upir.cz>
 *  Copyright (c) 2009 Roman Moravcik <roman.moravcik@gmail.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License version 2 as
 *  published by the Free Software Foundation.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA
 */

#include <stdio.h>
#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <hildon/hildon-caption.h>
#include <hildon/hildon.h>
#include <libosso.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "prefs.h"
#include "lang.h"
#include "langset.h"

#define GETTEXT_PACKAGE "osso-applet-textinput"
#include <glib/gi18n-lib.h>

#define L_CONF_DIR	"/apps/osso/inputmethod/hildon-im-languages"

gboolean get_l_bool(GConfClient *client, char *lang, char *key)
{
	char *tmp = g_strjoin("/", L_CONF_DIR, lang, key, NULL);
	gboolean res;

	res = gconf_client_get_bool(client, tmp, NULL);
	g_free(tmp);
	return res;
}

void set_l_bool(GConfClient *client, char *lang, char *key, gboolean val)
{
	char *tmp = g_strjoin("/", L_CONF_DIR, lang, key, NULL);

	g_debug("%s %d\n", tmp, val);

	gconf_client_set_bool(client, tmp, val, NULL);
	g_free(tmp);
}

gchar *get_l_str(GConfClient *client, char *lang, char *key)
{
	char *tmp = g_strjoin("/", L_CONF_DIR, lang, key, NULL);
	GConfValue *val;
	gchar *res;

	val = gconf_client_get_without_default(client, tmp, NULL);
	g_free(tmp);
	if (!val)
		return NULL;
	if (val->type != GCONF_VALUE_STRING) {
		gconf_value_free(val);
		return NULL;
	}
	res = g_strdup(gconf_value_get_string(val));
	gconf_value_free(val);
	return res;
}

void set_l_str(GConfClient *client, char *lang, char *key, gchar *val)
{
	char *tmp = g_strjoin("/", L_CONF_DIR, lang, key, NULL);

	gconf_client_set_string(client, tmp, val, NULL);
	g_free(tmp);
}

GList *get_dicts(GList *langs)
{
	GList *item, *dicts = NULL;
	struct lang *lang, *dict;
	unsigned i;

	for (item = langs, i = 0; item; item = g_list_next(item)) {
		lang = item->data;

		/* WORKAROUND: czech and russian dictionary must be listed,
		   because they are officially supported */
		if (lang->ext) {
			if ((strcmp(lang->fname, "cz-qwertz")) &&
			    (strcmp(lang->fname, "ru-windows")))
				continue;
		}

		dict = g_malloc(sizeof(struct lang));
		dict->fname = g_strdup(lang->fname);
		dict->desc = g_strdup(lang->desc);
		dict->code = g_strdup(lang->code);
		dicts = g_list_append(dicts, dict);
	}
	return dicts;
}

void fill_dict(HildonTouchSelector *combo, GList *langs, gchar *deflang)
{
	GList *item;
	struct lang *lang;
	unsigned i;

	for (item = langs, i = 0; item; item = g_list_next(item)) {
		lang = item->data;

		hildon_touch_selector_append_text(combo, lang->desc);

		if (deflang && !strcmp(lang->code, deflang))
			hildon_touch_selector_set_active(combo, 0, i);
		i++;
	}

	hildon_touch_selector_append_text(combo, _("tein_fi_word_completion_language_empty"));

	if (!deflang || !*deflang)
		hildon_touch_selector_set_active(combo, 0, i);

	/* check if something is really selected, if not select last item */
	if (hildon_touch_selector_get_active(combo, 0) < 0)
		hildon_touch_selector_set_active(combo, 0, i);
}
