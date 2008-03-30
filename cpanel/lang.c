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
#include <stdint.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <hildon/hildon-caption.h>
#include <hildon/hildon-banner.h>
#include <libosso.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "prefs.h"
#include "lang.h"
#include "langset.h"

struct langlink {
	gchar *src;
	gchar *dest;
};

struct data {
	GConfClient *client;
	GtkWidget *win;
	GList *langs;
	GList *langlinks;
	GtkComboBox *langsel[2];
	GtkButton *settings[2];
	GtkToggleButton *dual;
	int num_langs;
};

static gchar *get_lang(GConfClient *client, int n)
{
	char *tmp = g_strdup_printf("/apps/osso/inputmethod/hildon-im-languages/language-%d", n);
	gchar *res;

	res = gconf_client_get_string(client, tmp, NULL);
	g_free(tmp);
	return res;
}

static void set_lang(GConfClient *client, int n, gchar *val)
{
	char *tmp = g_strdup_printf("/apps/osso/inputmethod/hildon-im-languages/language-%d", n);

	gconf_client_set_string(client, tmp, val, NULL);
	g_free(tmp);
}

static gchar *read_vkb_str(FILE *f)
{
	gchar *res;
	uint8_t len;

	if (!fread(&len, 1, 1, f) || !len)
		return NULL;
	res = g_malloc(len + 1);
	if (fread(res, 1, len, f) != len) {
		g_free(res);
		return NULL;
	}
	res[len] = '\0';
	return res;
}

static struct lang *read_vkb(gchar *path)
{
	FILE *f;
	struct lang *res = NULL;
	uint8_t ver;
	gchar *desc, *code;

	f = fopen(path, "r");
	if (!f)
		return NULL;
	/* version */
	if (!fread(&ver, 1, 1, f) || ver != 1)
		goto no;
	/* number of sections */
	if (!fread(&ver, 1, 1, f))
		goto no;
	/* descriptive name */
	desc = read_vkb_str(f);
	if (!desc)
		goto no;
	/* locale name */
	code = read_vkb_str(f);
	if (!code) {
		g_free(desc);
		goto no;
	}

	res = g_malloc(sizeof(struct lang));
	res->desc = desc;
	res->code = code;
no:
	fclose(f);
	return res;
}

static GList *add_lang_link(gchar *path, gchar *dest, GList *list)
/* 'dest' has to be allocated; it's eaten by this function! */
{
	struct langlink *langlink;
	gchar *link, *tmp;

	link = g_file_read_link(path, NULL);
	if (link) {
		langlink = g_malloc(sizeof(struct langlink));
		langlink->dest = dest;
		tmp = strrchr(link, '/');
		langlink->src = tmp ? tmp + 1 : link;
		tmp = strrchr(link, '.');
		if (tmp)
			*tmp = '\0';
		langlink->src = g_strdup(langlink->src);
		g_free(link);
		list = g_list_append(list, langlink);
	}
	return list;
}

static GList *get_langs(gchar *path, GList **langlinks, GList *list)
{
	GDir *d;
	const gchar *fname;
	gchar *fullpath, *shortpath, *tmp;
	struct lang *lang;

	d = g_dir_open(path, 0, NULL);
	if (!d)
		return list;
	while ((fname = g_dir_read_name(d))) {
		if (!g_str_has_suffix(fname, ".vkb"))
			continue;
		fullpath = g_strjoin("/", path, fname, NULL);
		shortpath = g_strdup(fname);
		tmp = strrchr(shortpath, '.');
		if (tmp)
			*tmp = '\0';
		if (langlinks && g_file_test(fullpath, G_FILE_TEST_IS_SYMLINK)) {
			*langlinks = add_lang_link(fullpath, shortpath, *langlinks);
			g_free(fullpath);
			continue;
		}
		lang = read_vkb(fullpath);
		if (lang) {
			lang->fname = shortpath;
			lang->ext = !langlinks;
			list = g_list_append(list, lang);
		} else
			g_free(shortpath);
		g_free(fullpath);
	}
	g_dir_close(d);
	return list;
}

static struct lang *get_def_lang(gchar *code, GList *langs, GList *langlinks)
{
	GList *item;
	struct lang *lang;
	struct langlink *langlink;

	if (!code || !*code)
		return NULL;
	for (item = langs; item; item = g_list_next(item)) {
		lang = item->data;
		if (!strcmp(lang->fname, code))
			return lang;
	}
	if (langlinks)
		for (item = langlinks; item; item = g_list_next(item)) {
			langlink = item->data;
			if (!strcmp(langlink->dest, code))
				return get_def_lang(langlink->src, langs, NULL);
		}
	return NULL;
}

static void free_langs(GList *list)
{
	GList *item;
	struct lang *lang;

	for (item = list; item; item = g_list_next(item)) {
		lang = item->data;
		g_free(lang->fname);
		g_free(lang->desc);
		g_free(lang->code);
		g_free(lang);
	}
	g_list_free(list);
}

static void free_langlinks(GList *list)
{
	GList *item;
	struct langlink *langlink;

	for (item = list; item; item = g_list_next(item)) {
		langlink = item->data;
		g_free(langlink->src);
		g_free(langlink->dest);
		g_free(langlink);
	}
	g_list_free(list);
}

static void fill_langsel(GtkComboBox *combo, GList *langs, struct lang *deflang, gboolean empty)
{
	GList *item;
	struct lang *lang;
	unsigned i;

	for (item = langs, i = 0; item; item = g_list_next(item), i++) {
		lang = item->data;
		gtk_combo_box_append_text(combo, lang->desc);
		if (lang == deflang)
			gtk_combo_box_set_active(combo, i);
	}
	if (empty) {
		gtk_combo_box_append_text(combo, "");
		if (!deflang)
			gtk_combo_box_set_active(combo, i);
	}
}

static void sensitivity_langsel(struct data *d)
{
	gboolean sens;

	sens = (gtk_combo_box_get_active(d->langsel[1]) < d->num_langs);
	gtk_widget_set_sensitive(GTK_WIDGET(d->settings[1]), sens);
	gtk_widget_set_sensitive(GTK_WIDGET(d->dual), sens);
}

static void verify_langsel(GtkComboBox *combo, struct data *d)
{
	struct lang *lang[2];
	int res;
	unsigned i;

	(void)combo;
	for (i = 0; i < 2; i++) {
		res = gtk_combo_box_get_active(d->langsel[i]);
		lang[i] = (res >= 0) ? g_list_nth_data(d->langs, res) : NULL;
	}
	if (lang[0] && lang[1] && !strcmp(lang[0]->code, lang[1]->code)) {
		hildon_banner_show_information(d->win, "gtk-dialog-error",
			"Impossible combination of languages");
		gtk_combo_box_set_active(d->langsel[1], d->num_langs);
	}
	sensitivity_langsel(d);
}

static void do_settings(GtkButton *button, struct data *d)
{
	int which;

	which = (button == d->settings[0]) ? 0 : 1;
	lang_settings(d->client, d->win, d->langs, gtk_combo_box_get_active(d->langsel[which]));
}

static GtkWidget *start(GConfClient *client, GtkWidget *win, void **data)
{
	struct data *d;
	gchar *tmp;
	GtkBox *vbox;
	GtkSizeGroup *group;
	GtkWidget *align;
	unsigned i;

	d = g_new0(struct data, 1);
	d->client = client;
	d->win = win;

	d->langs = get_langs("/usr/share/keyboards", &d->langlinks, NULL);
	d->langs = get_langs("/usr/share/ukeyboard", NULL, d->langs);
	d->num_langs = g_list_length(d->langs);

	vbox = GTK_BOX(gtk_vbox_new(FALSE, 0));
	gtk_box_set_spacing(vbox, 5);
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	for (i = 0; i < 2; i++) {
		d->langsel[i] = GTK_COMBO_BOX(gtk_combo_box_new_text());

		tmp = get_lang(client, i);
		fill_langsel(d->langsel[i], d->langs, get_def_lang(tmp, d->langs, d->langlinks), i == 1);
		if (tmp)
			g_free(tmp);

		g_signal_connect(G_OBJECT(d->langsel[i]), "changed", G_CALLBACK(verify_langsel), d);
		gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, i == 0 ? "1st language" : "2nd language",
			GTK_WIDGET(d->langsel[i]), NULL, HILDON_CAPTION_MANDATORY));

		d->settings[i] = GTK_BUTTON(gtk_button_new_with_label("Settings"));
		g_signal_connect(G_OBJECT(d->settings[i]), "clicked", G_CALLBACK(do_settings), d);
		gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, NULL,
			GTK_WIDGET(d->settings[i]), NULL, HILDON_CAPTION_OPTIONAL));
	}

	d->dual = GTK_TOGGLE_BUTTON(gtk_check_button_new());
	gtk_toggle_button_set_active(d->dual, get_bool(client, "dual-dictionary"));
	gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Use dual dictionaries",
		GTK_WIDGET(d->dual), NULL, HILDON_CAPTION_MANDATORY));

	sensitivity_langsel(d);

	g_object_unref(G_OBJECT(group));
	*data = d;

	align = gtk_alignment_new(0, 0, 1, 0);
	gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(vbox));
	return align;
}

static void action(GConfClient *client, void *data)
{
	struct data *d = data;
	struct lang *lang;
	gchar *tmp;
	int res;
	unsigned i;

	for (i = 0; i < 2; i++) {
		res = gtk_combo_box_get_active(d->langsel[i]);
		if (res < 0)
			continue;
		lang = g_list_nth_data(d->langs, res);
		if (lang) {
			if (lang->ext) {
				tmp = g_strdup_printf("sudo /usr/libexec/ukeyboard-set %s %s", lang->fname, lang->code);
				if (system(tmp))
					hildon_banner_show_information(d->win, "gtk-dialog-warning",
						"Setting failed");
				g_free(tmp);
			}
			/* When the newly set layout has the same lang code as the
			 * previous one (but the underlaying file changed), setting
			 * the same value won't trigger layout reload. Workaround
			 * that by setting "en_GB" lang code first. */
			set_lang(client, i, "en_GB");
			set_lang(client, i, lang->code);
		} else
			set_lang(client, i, "");
	}
	set_bool(client, "dual-dictionary", gtk_toggle_button_get_active(d->dual));
}

static void stop(GConfClient *client, void *data)
{
	struct data *d = data;

	(void)client;

	free_langs(d->langs);
	free_langlinks(d->langlinks);
	g_free(d);
}

void prefs_lang_init(struct prefs *prefs)
{
	prefs->start = start;
	prefs->action = action;
	prefs->stop = stop;
	prefs->name = "Languages";
}
