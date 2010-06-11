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
#include <stdint.h>
#include <stdlib.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <hildon/hildon-caption.h>
#include <hildon/hildon-banner.h>
#include <hildon/hildon.h>
#include <libosso.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "prefs.h"
#include "lang.h"
#include "langset.h"

#define GETTEXT_PACKAGE "ukeyboard"
#include <glib/gi18n-lib.h>

char *ignore_autocapitalisation[] = {"ar_AR", "fa_IR", "he_IL", "ka_GE", "km_KH", "pa_IN", "th_TH", NULL };

struct langlink {
	gchar *src;
	gchar *dest;
};

struct data {
	GConfClient *client;
	GtkWidget *win;
	GList *langs;
	GList *dicts;
	GList *langlinks;
	HildonCheckButton *word_compl;
	HildonCheckButton *auto_cap;
	HildonCheckButton *sp_after;
	HildonPickerButton *sec_dictsel;
	HildonTouchSelector *langsel[2];
	HildonTouchSelector *dictsel[2];
	HildonCheckButton *dual;
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

static gboolean check_ukbd_layout(gchar *code, GList *langlinks)
{
	GList *item;
	struct langlink *langlink;

	if (!code || !*code)
		return FALSE;
	if (langlinks)
		for (item = langlinks; item; item = g_list_next(item)) {
			langlink = item->data;
			if (!strcmp(langlink->dest, code)) {
				if (!strncmp(langlink->src, "ukbd", 4)) {
					return TRUE;
				}
			}
		}
	return FALSE;
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

static void fill_langsel(HildonTouchSelector *combo, GList *langs, struct lang *deflang, gboolean empty)
{
	GList *item;
	struct lang *lang;
	unsigned i;

	for (item = langs, i = 0; item; item = g_list_next(item), i++) {
		lang = item->data;
		hildon_touch_selector_append_text(combo, lang->desc);
		if (lang == deflang)
			hildon_touch_selector_set_active(combo, 0, i);
	}
	if (empty) {
		hildon_touch_selector_append_text(combo, _TI("tein_fi_not_in_use"));
		if (!deflang)
			hildon_touch_selector_set_active(combo, 0, i);
	}
}

static void sensitivity_langsel(struct data *d)
{
	gboolean sens;

	sens = (hildon_touch_selector_get_active(d->langsel[1], 0) < d->num_langs);
	gtk_widget_set_sensitive(GTK_WIDGET(d->dual), sens);

	if (sens)
		gtk_widget_show(GTK_WIDGET(d->sec_dictsel));
	else
		gtk_widget_hide(GTK_WIDGET(d->sec_dictsel));
}

static void verify_langsel(HildonTouchSelector *combo, gint column, struct data *d)
{
	struct lang *lang[2];
	int res;
	unsigned i;

	(void)combo;
	for (i = 0; i < 2; i++) {
		res = hildon_touch_selector_get_active(d->langsel[i], column);
		lang[i] = (res >= 0) ? g_list_nth_data(d->langs, res) : NULL;
	}
	if (lang[0] && lang[1] && !strcmp(lang[0]->code, lang[1]->code)) {
		hildon_banner_show_information(d->win, "gtk-dialog-error",
			"Impossible combination of languages");
		hildon_touch_selector_set_active(d->langsel[1], column, d->num_langs);
	}
	sensitivity_langsel(d);
}

static gint langs_compare_func(gconstpointer a, gconstpointer b)
{
	struct lang *lang_a = (struct lang *) a;
	struct lang *lang_b = (struct lang *) b;

	return g_utf8_collate (lang_a->desc, lang_b->desc);
}

static GtkWidget *start(GConfClient *client, GtkWidget *win, void **data)
{
	struct data *d;
	gchar *tmp;
	GtkWidget *vbox, *table;
	struct lang *lang;
	unsigned i;

	d = g_new0(struct data, 1);
	d->client = client;
	d->win = win;

	d->langs = get_langs("/usr/share/keyboards", &d->langlinks, NULL);
	d->langs = get_langs("/usr/share/ukeyboard", NULL, d->langs);
	d->langs = g_list_sort(d->langs, langs_compare_func);
	d->num_langs = g_list_length(d->langs);

	d->dicts = get_dicts(d->langs);

	vbox = gtk_vbox_new(FALSE, 0);

	d->word_compl = HILDON_CHECK_BUTTON(hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT));
	gtk_button_set_label (GTK_BUTTON (d->word_compl), _TI("tein_fi_settings_word_completion"));
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(d->word_compl), TRUE, TRUE, 0);

	d->auto_cap = HILDON_CHECK_BUTTON(hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT));
	gtk_button_set_label (GTK_BUTTON (d->auto_cap), _TI("tein_fi_settings_auto_capitalization"));
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(d->auto_cap), TRUE, TRUE, 0);

	d->sp_after = HILDON_CHECK_BUTTON(hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT));
	gtk_button_set_label (GTK_BUTTON (d->sp_after), _TI("tein_fi_settings_space_after_word"));
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(d->sp_after), TRUE, TRUE, 0);

	table = gtk_table_new(2, 2, TRUE);
	gtk_box_pack_start(GTK_BOX(vbox), table, TRUE, TRUE, 0);

	for (i = 0; i < 2; i++) {
		GtkWidget *button;
		gchar *code;

		d->langsel[i] = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
		code = get_lang(client, i);
		fill_langsel(d->langsel[i], d->langs, get_def_lang(code, d->langs, d->langlinks), i == 1);
		g_signal_connect(G_OBJECT(d->langsel[i]), "changed", G_CALLBACK(verify_langsel), d);

		button = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT, HILDON_BUTTON_ARRANGEMENT_VERTICAL);
		hildon_picker_button_set_selector(HILDON_PICKER_BUTTON (button), d->langsel[i]);
		hildon_button_set_title(HILDON_BUTTON(button), i == 0 ? _TI("tein_fi_primary_language") : _TI("tein_fi_secondary_language"));

		/* if nothing was selected, try set selector button value text */
		if (hildon_touch_selector_get_active(d->langsel[i], 0) < 0) {
			if (check_ukbd_layout(code, d->langlinks))
				hildon_button_set_value(HILDON_BUTTON(button), "UKeyboard Creator Layout");
			else
				hildon_button_set_value(HILDON_BUTTON(button), _TI("tein_fi_not_in_use"));
		}

		hildon_button_set_alignment (HILDON_BUTTON (button), 0.0, 0.5, 1.0, 0.0);
		hildon_button_set_title_alignment(HILDON_BUTTON(button), 0.0, 0.5);
		hildon_button_set_value_alignment (HILDON_BUTTON (button), 0.0, 0.5);
		gtk_table_attach_defaults(GTK_TABLE(table), button, 0, 1, i, i + 1);

		d->dictsel[i] = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());
		tmp = get_l_str(client, code, "dictionary");
		lang = get_def_lang(code, d->langs, d->langlinks);

		/* If tmp is NULL (i.e. the gconf key is unset), try to use the same 
		 * dictionary as the keyboard. But don't do this if the keyboard is 
		 * from our package. */
		fill_dict(d->dictsel[i], d->dicts, (tmp || (lang && lang->ext)) ? tmp : code);
		if (tmp)
			g_free(tmp);

		button = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT, HILDON_BUTTON_ARRANGEMENT_VERTICAL);
		hildon_button_set_title(HILDON_BUTTON(button), _TI("tein_fi_settings_dictionary"));
		hildon_picker_button_set_selector(HILDON_PICKER_BUTTON (button), d->dictsel[i]);
		hildon_button_set_alignment (HILDON_BUTTON (button), 0.0, 0.5, 1.0, 0.0);
		hildon_button_set_title_alignment(HILDON_BUTTON(button), 0.0, 0.5);
		hildon_button_set_value_alignment (HILDON_BUTTON (button), 0.0, 0.5);
		gtk_table_attach_defaults(GTK_TABLE(table), button, 1, 2, i, i + 1);

		if (i == 1) {
			/* secondary language */
			d->sec_dictsel = HILDON_PICKER_BUTTON(button);

			/* check if auto-capitalization for the second language isn't enabled */
			if (get_l_bool(client, code, "auto-capitalisation")) {
				if (!hildon_check_button_get_active(d->auto_cap))
					hildon_check_button_set_active(d->auto_cap, TRUE);
			}
		} else {
			/* primary language */
			hildon_check_button_set_active(d->word_compl, get_l_bool(client, code, "word-completion"));
			hildon_check_button_set_active(d->auto_cap, get_l_bool(client, code, "auto-capitalisation"));
			hildon_check_button_set_active(d->sp_after, get_l_bool(client, code, "insert-space-after-word"));
		}

		if (code)
			g_free(code);
	}

	d->dual = HILDON_CHECK_BUTTON(hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT));
	hildon_check_button_set_active(d->dual, get_bool(client, "dual-dictionary"));
	gtk_button_set_label (GTK_BUTTON (d->dual), _TI("tein_fi_dual_dictionary_use"));
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(d->dual), TRUE, TRUE, 0);

	gtk_widget_show_all(vbox);

	sensitivity_langsel(d);

	*data = d;

	return vbox;
}

static void action(GConfClient *client, void *data)
{
	struct data *d = data;
	struct lang *lang;
	gchar *tmp;
	int res;
	unsigned i, j;

	for (i = 0; i < 2; i++) {
		struct lang *dict;

		res = hildon_touch_selector_get_active(d->langsel[i], 0);
		if (res < 0)
			continue;
		lang = g_list_nth_data(d->langs, res);
		if (lang) {
			if (lang->ext) {
				if (inside_scratchbox)
					tmp = g_strdup_printf("fakeroot /usr/libexec/ukeyboard-set %s %s", lang->fname, lang->code);
				else
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

			set_l_bool(client, lang->code, "auto-capitalisation", hildon_check_button_get_active(d->auto_cap));
			set_l_bool(client, lang->code, "word-completion", hildon_check_button_get_active(d->word_compl));
			set_l_bool(client, lang->code, "insert-space-after-word", hildon_check_button_get_active(d->sp_after));

			/* forced disabling of autocapitalization for selected language codes,
			   that doesn't support autocapitalization */
			for (j = 0; ignore_autocapitalisation[j] != NULL; j++) {
				if (!strcmp(lang->code, ignore_autocapitalisation[j])) {
					set_l_bool(client, lang->code, "auto-capitalisation", FALSE);
				}
			}

			res = hildon_touch_selector_get_active(d->dictsel[i], 0);
			if (res >= 0)
				dict = g_list_nth_data(d->dicts, res);

			if (dict)
				set_l_str(client, lang->code, "dictionary", dict->code);
			else
				set_l_str(client, lang->code, "dictionary", "");
		} else
			set_lang(client, i, "");
	}
	set_bool(client, "dual-dictionary", hildon_check_button_get_active(d->dual));
}

static void stop(GConfClient *client, void *data)
{
	struct data *d = data;

	(void)client;

	free_langs(d->langs);
	free_langs(d->dicts);
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
