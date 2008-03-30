/*
 *  Copyright (c) 2008 Jiri Benc <jbenc@upir.cz>
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
#include <libosso.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "prefs.h"
#include "lang.h"
#include "langset.h"

#define L_CONF_DIR	"/apps/osso/inputmethod/hildon-im-languages"

static gboolean get_l_bool(GConfClient *client, char *lang, char *key)
{
	char *tmp = g_strjoin("/", L_CONF_DIR, lang, key, NULL);
	gboolean res;

	res = gconf_client_get_bool(client, tmp, NULL);
	g_free(tmp);
	return res;
}

static void set_l_bool(GConfClient *client, char *lang, char *key, gboolean val)
{
	char *tmp = g_strjoin("/", L_CONF_DIR, lang, key, NULL);

	gconf_client_set_bool(client, tmp, val, NULL);
	g_free(tmp);
}

static gchar *get_l_str(GConfClient *client, char *lang, char *key)
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

static void set_l_str(GConfClient *client, char *lang, char *key, gchar *val)
{
	char *tmp = g_strjoin("/", L_CONF_DIR, lang, key, NULL);

	gconf_client_set_string(client, tmp, val, NULL);
	g_free(tmp);
}

static void fill_dict(GtkComboBox *combo, GList *langs, gchar *deflang)
{
	GList *item;
	struct lang *lang;
	unsigned i;

	for (item = langs, i = 0; item; item = g_list_next(item)) {
		lang = item->data;
		if (lang->ext)
			continue;
		gtk_combo_box_append_text(combo, lang->desc);
		if (deflang && !strcmp(lang->code, deflang))
			gtk_combo_box_set_active(combo, i);
		i++;
	}
	gtk_combo_box_append_text(combo, "Custom/Empty");
	if (!deflang || !*deflang)
		gtk_combo_box_set_active(combo, i);
}

void lang_settings(GConfClient *client, GtkWidget *win, GList *langs, int n)
{
	GtkDialog *dialog;
	GtkBox *vbox;
	GtkSizeGroup *group;
	GtkToggleButton *auto_cap, *word_compl, *word_pred, *sp_after;
	GtkComboBox *dict;
	struct lang *lang;
	gchar *code, *tmp;
	int res;

	if (n < 0)
		return;
	lang = g_list_nth_data(langs, n);
	if (!lang)
		return;
	code = lang->code;

	dialog = GTK_DIALOG(gtk_dialog_new());
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(win));
	tmp = g_strconcat("On-screen text input: ", lang->desc, NULL);
	gtk_window_set_title(GTK_WINDOW(dialog), tmp);
	g_free(tmp);
	gtk_dialog_set_has_separator(dialog, FALSE);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

	gtk_dialog_add_action_widget(dialog, gtk_button_new_with_label("OK"), GTK_RESPONSE_ACCEPT);
	gtk_dialog_add_action_widget(dialog, gtk_button_new_with_label("Cancel"), GTK_RESPONSE_REJECT);

	vbox = GTK_BOX(gtk_vbox_new(FALSE, 0));
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	auto_cap = GTK_TOGGLE_BUTTON(gtk_check_button_new());
	gtk_toggle_button_set_active(auto_cap, get_l_bool(client, code, "auto-capitalisation"));
	gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Auto-capitalisation",
		GTK_WIDGET(auto_cap), NULL, HILDON_CAPTION_MANDATORY));

	word_compl = GTK_TOGGLE_BUTTON(gtk_check_button_new());
	gtk_toggle_button_set_active(word_compl, get_l_bool(client, code, "word-completion"));
	gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Word completion",
		GTK_WIDGET(word_compl), NULL, HILDON_CAPTION_MANDATORY));

	word_pred = GTK_TOGGLE_BUTTON(gtk_check_button_new());
	gtk_toggle_button_set_active(word_pred, get_l_bool(client, code, "next-word-prediction"));
	gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Next word prediction",
		GTK_WIDGET(word_pred), NULL, HILDON_CAPTION_MANDATORY));

	sp_after = GTK_TOGGLE_BUTTON(gtk_check_button_new());
	gtk_toggle_button_set_active(sp_after, get_l_bool(client, code, "insert-space-after-word"));
	gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Insert space after word",
		GTK_WIDGET(sp_after), NULL, HILDON_CAPTION_MANDATORY));

	dict = GTK_COMBO_BOX(gtk_combo_box_new_text());
	tmp = get_l_str(client, code, "dictionary");
	/* If tmp is NULL (i.e. the gconf key is unset), try to use the same
	 * dictionary as the keyboard. But don't do this if the keyboard is
	 * from our package. */
	fill_dict(dict, langs, (tmp || lang->ext) ? tmp : code);
	if (tmp)
		g_free(tmp);
	gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Dictionary",
		GTK_WIDGET(dict), NULL, HILDON_CAPTION_MANDATORY));

	g_object_unref(G_OBJECT(group));

	gtk_container_add(GTK_CONTAINER(dialog->vbox), GTK_WIDGET(vbox));
	gtk_widget_show_all(GTK_WIDGET(dialog));
	res = gtk_dialog_run(dialog);
	if (res == GTK_RESPONSE_ACCEPT) {
		set_l_bool(client, code, "auto-capitalisation", gtk_toggle_button_get_active(auto_cap));
		set_l_bool(client, code, "word-completion", gtk_toggle_button_get_active(word_compl));
		set_l_bool(client, code, "next-word-prediction", gtk_toggle_button_get_active(word_pred));
		set_l_bool(client, code, "insert-space-after-word", gtk_toggle_button_get_active(sp_after));
		res = gtk_combo_box_get_active(dict);
		if (res >= 0) {
			lang = g_list_nth_data(langs, res);
			tmp = (lang && !lang->ext) ? lang->code : "";
			set_l_str(client, code, "dictionary", tmp);
		}
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));
}
