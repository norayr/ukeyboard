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
#include "hw.h"

#define GETTEXT_PACKAGE "osso-applet-textinput"
#include <glib/gi18n-lib.h>

struct layout {
	gchar *model;
	gchar *layout;
	gchar *name;
};

struct data {
	GList *layouts;
	HildonTouchSelector *combo;
};

typedef struct {
	gchar *layout;
	gchar *name;
} layouts;

static layouts layout_names[] = {
    {"cz",		"Čeština"},
    {"cz_qwerty",	"Čeština - QWERTY"},
    {"dano",		"Dansk, Norsk"},
    {"de",		"Deutsch"},
    {"us",		"English, Nederlands"},
    {"ptes",		"Español, Français (Canada), Português"},
    {"fr",		"Français (France)"},
    {"it",		"Italiano"},
    {"pl",		"Polski"},
    {"fise",		"Suomi, Svenska"},
    {"ch",		"Suisse, Schweiz"},
    {"ru",		"Русский"},
    {"sk",		"Slovenčina"},
    {"sk_qwerty",	"Slovenčina - QWERTY"},
    {"aren",		"Arabic, English"},
    {"dv",		"Dvorak"},
    {"gr",		"Ελληνικά"},
    {"bg_phonetic",	"Български - Phonetic"},
    {"faen",		"Persian, English"},
    {"mk",		"Македонски"},
    {"ru_phonetic",	"Русский - Phonetic"},
    {NULL,		NULL}
};

static gchar *resolve_layout_name(const gchar *layout)
{
    unsigned char i = 0;

    while (layout_names[i].layout != NULL)
    {
	    if (!strcmp(layout_names[i].layout, layout))
		    return layout_names[i].name;
	    i++;
    }
    return NULL;
}

static char *strip(char *s)
{
	while (*s == ' ' || *s == '\t' || *s == '\n' || *s == '\r')
		s++;
	return s;
}

static GList *get_layouts(gchar *path, gchar *model, GList *list)
{
	FILE *f;
	char *buf, *s, *s2;
	gchar *layout = NULL;
	gchar *name = NULL;
	struct layout *lay;

	f = fopen(path, "r");
	if (!f)
		return list;
	buf = g_malloc(512);
	if (!buf) {
		fclose(f);
		return list;
	}
	while (fgets(buf, 512, f)) {
		s = strip(buf);
		if (!strncmp(s, "xkb_symbols", 11)) {
			if (layout) {
				g_free(layout);
				layout = NULL;
			}
			s = strip(s + 11);
			if (*s != '"')
				continue;
			s++;
			s2 = strchr(s, '"');
			if (!s2)
				continue;
			*s2 = '\0';
			layout = g_strdup(s);

			/* ignore nordic layout */
			if (!strcmp(layout, "nordic")) {
				layout = NULL;
				continue;
			}

			/* WORKAROUND: ignore cz_qwerty nokiarx51 layout,
			   because it's broken in PR1.1 and PR1.1.1 */
			if (!strcmp(model, "nokiarx51")) {
				if (!strcmp(layout, "cz_qwerty")) {
					layout = NULL;
					continue;
				}
			}

			name = resolve_layout_name(layout);
			if (name)
			{
				lay = g_malloc(sizeof(struct layout));
	    			lay->model = g_strdup(model);
				lay->layout = layout;
				lay->name = g_strdup(name);
				layout = NULL;
				list = g_list_append(list, lay);
				continue;
			}
		} else if (!strncmp(s, "name", 4) && layout) {
			s = strip(s + 4);
			if (*s != '[')
				continue;
			s2 = strchr(s, ']');
			if (!s2)
				continue;
			s = strip(s2 + 1);
			if (*s != '=')
				continue;
			s = strip(s + 1);
			if (*s != '"')
				continue;
			s++;
			s2 = strchr(s, '"');
			if (!s2)
				continue;
			*s2 = '\0';

			lay = g_malloc(sizeof(struct layout));
			lay->model = g_strdup(model);
			lay->layout = layout;
			lay->name = g_strdup(s);
			layout = NULL;
			list = g_list_append(list, lay);
		}
	}
	fclose(f);
	return list;
}

static void free_layouts(GList *list)
{
	GList *item;
	struct layout *lay;

	for (item = list; item; item = g_list_next(item)) {
		lay = item->data;
		g_free(lay->model);
		g_free(lay->layout);
		g_free(lay->name);
		g_free(lay);
	}
	g_list_free(list);
}

static gint layouts_compare_func(gconstpointer a, gconstpointer b)
{
	struct layout *layout_a = (struct layout *) a;
	struct layout *layout_b = (struct layout *) b;

	return g_utf8_collate (layout_a->name, layout_b->name);
}

static GtkWidget *start(GConfClient *client, GtkWidget *win, void **data)
{
	struct data *d;
	GList *item;
	gchar *omodel, *olayout;
	struct layout *lay;
	unsigned i;

	GtkWidget *button;

	(void)win;

	if (!internal_kbd) {
		*data = NULL;
		return NULL;
	}

	d = g_malloc(sizeof(struct data));

	omodel = get_str(client, "int_kb_model");
	olayout = get_str(client, "int_kb_layout");
	d->layouts = get_layouts("/usr/share/X11/xkb/symbols/nokia_vndr/rx-51", "nokiarx51", NULL);
	d->layouts = get_layouts("/usr/share/X11/xkb/symbols/nokia_vndr/ukeyboard", "ukeyboard", d->layouts);
	d->layouts = g_list_sort(d->layouts, layouts_compare_func);

	d->combo = HILDON_TOUCH_SELECTOR(hildon_touch_selector_new_text());

	button = hildon_picker_button_new(HILDON_SIZE_FINGER_HEIGHT, HILDON_BUTTON_ARRANGEMENT_VERTICAL);
	hildon_button_set_title(HILDON_BUTTON(button), _("tein_fi_keyboard_layout"));
	hildon_picker_button_set_selector(HILDON_PICKER_BUTTON (button), d->combo);
	hildon_button_set_alignment (HILDON_BUTTON (button), 0.0, 0.5, 1.0, 0.0);
	hildon_button_set_title_alignment(HILDON_BUTTON(button), 0.0, 0.5);
	hildon_button_set_value_alignment (HILDON_BUTTON (button), 0.0, 0.5);

	for (item = d->layouts, i = 0; item; item = g_list_next(item), i++) {
		lay = item->data;
		hildon_touch_selector_append_text(d->combo, lay->name);
		if (omodel && olayout && !strcmp(lay->model, omodel) && !strcmp(lay->layout, olayout))
			hildon_touch_selector_set_active(d->combo, 0, i);
	}

	g_free(olayout);
	g_free(omodel);

	*data = d;

	gtk_widget_show(button);

	return button;
}

static void action(GConfClient *client, void *data)
{
	struct data *d = data;
	struct layout *lay;
	int res;

	if (!d)
		return;
	res = hildon_touch_selector_get_active(d->combo, 0);
	if (res >= 0) {
		lay = g_list_nth_data(d->layouts, res);
		if (lay) {
			set_str(client, "int_kb_model", lay->model);
			set_str(client, "int_kb_layout", lay->layout);
		}
	}
}

static void stop(GConfClient *client, void *data)
{
	struct data *d = data;

	(void)client;
	if (d) {
		free_layouts(d->layouts);
		g_free(d);
	}
}

void prefs_hw_init(struct prefs *prefs)
{
	prefs->start = start;
	prefs->action = action;
	prefs->stop = stop;
	prefs->name = "Hardware";
}
