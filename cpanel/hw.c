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
#include "hw.h"

struct layout {
	gchar *model;
	gchar *layout;
	gchar *name;
};

struct data {
	GList *layouts;
	GtkComboBox *combo;
};

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

static GtkWidget *start(GConfClient *client, GtkWidget *win, void **data)
{
	struct data *d;
	GList *item;
	gchar *omodel, *olayout;
	struct layout *lay;
	unsigned i;

	GtkBox *vbox;
	GtkSizeGroup *group;
	GtkWidget *align;

	(void)win;

	if (!internal_kbd) {
		*data = NULL;
		return NULL;
	}

	d = g_malloc(sizeof(struct data));

	omodel = get_str(client, "int_kb_model");
	olayout = get_str(client, "int_kb_layout");
	d->layouts = get_layouts("/usr/share/X11/xkb/symbols/nokia_vndr/rx-44", "nokiarx44", NULL);
	d->layouts = get_layouts("/usr/share/X11/xkb/symbols/nokia_vndr/ukeyboard", "ukeyboard", d->layouts);

	vbox = GTK_BOX(gtk_vbox_new(FALSE, 0));
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	d->combo = GTK_COMBO_BOX(gtk_combo_box_new_text());
	for (item = d->layouts, i = 0; item; item = g_list_next(item), i++) {
		lay = item->data;
		gtk_combo_box_append_text(d->combo, lay->name);
		if (omodel && olayout && !strcmp(lay->model, omodel) && !strcmp(lay->layout, olayout))
			gtk_combo_box_set_active(d->combo, i);
	}
	gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Keyboard layout",
		GTK_WIDGET(d->combo), NULL, HILDON_CAPTION_MANDATORY));

	g_free(olayout);
	g_free(omodel);
	g_object_unref(G_OBJECT(group));

	*data = d;

	align = gtk_alignment_new(0, 0, 1, 0);
	gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(vbox));
	return align;
}

static void action(GConfClient *client, void *data)
{
	struct data *d = data;
	struct layout *lay;
	int res;

	if (!d)
		return;
	res = gtk_combo_box_get_active(d->combo);
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
