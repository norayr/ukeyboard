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
#include <libosso.h>
#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "prefs.h"
#include "hw.h"
#include "onscreen.h"
#include "lang.h"
#include "about.h"

static init_func inits[] = { prefs_hw_init, prefs_onscreen_init, prefs_lang_init, prefs_about_init };
#define PLUGINS		(sizeof(inits) / sizeof(init_func))
static struct prefs prefs[PLUGINS];

gboolean internal_kbd;

#define IM_CONF_DIR	"/apps/osso/inputmethod"

gboolean get_bool(GConfClient *client, char *key)
{
	char *tmp = g_strjoin("/", IM_CONF_DIR, key, NULL);
	gboolean res;

	res = gconf_client_get_bool(client, tmp, NULL);
	g_free(tmp);
	return res;
}

void set_bool(GConfClient *client, char *key, gboolean val)
{
	char *tmp = g_strjoin("/", IM_CONF_DIR, key, NULL);

	gconf_client_set_bool(client, tmp, val, NULL);
	g_free(tmp);
}

gint get_int(GConfClient *client, char *key)
{
	char *tmp = g_strjoin("/", IM_CONF_DIR, key, NULL);
	gint res;

	res = gconf_client_get_int(client, tmp, NULL);
	g_free(tmp);
	return res;
}

void set_int(GConfClient *client, char *key, gint val)
{
	char *tmp = g_strjoin("/", IM_CONF_DIR, key, NULL);

	gconf_client_set_int(client, tmp, val, NULL);
	g_free(tmp);
}

gchar *get_str(GConfClient *client, char *key)
{
	char *tmp = g_strjoin("/", IM_CONF_DIR, key, NULL);
	gchar *res;

	res = gconf_client_get_string(client, tmp, NULL);
	g_free(tmp);
	return res;
}

void set_str(GConfClient *client, char *key, gchar *val)
{
	char *tmp = g_strjoin("/", IM_CONF_DIR, key, NULL);

	gconf_client_set_string(client, tmp, val, NULL);
	g_free(tmp);
}

static GConfClient *init_conf(void)
{
	GConfClient *client;

	client = gconf_client_get_default();
	if (!client)
		return NULL;
	gconf_client_add_dir(client, IM_CONF_DIR, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
	internal_kbd = get_bool(client, "have-internal-keyboard");
	return client;
}

static void deinit_conf(GConfClient *client)
{
	g_object_unref(G_OBJECT(client));
}

osso_return_t execute(osso_context_t *osso, gpointer data, gboolean user_activated)
{
	GConfClient *conf;
	GtkDialog *dialog;
	GtkWidget *widget, *notebook;
	void *plugin_data[PLUGINS];
	unsigned i;
	int res;

	(void)osso; (void)user_activated;	/* shut up, GCC */

	conf = init_conf();
	if (!conf)
		return OSSO_ERROR;

	for (i = 0; i < PLUGINS; i++) {
		inits[i](prefs + i);
	}

	dialog = GTK_DIALOG(gtk_dialog_new());
	if (!dialog) {
		deinit_conf(conf);
		return OSSO_ERROR;
	}
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(data));
	gtk_window_set_title(GTK_WINDOW(dialog), "Text input (ukeyboard)");
	gtk_dialog_set_has_separator(dialog, FALSE);
	gtk_window_set_modal(GTK_WINDOW(dialog), TRUE);

	gtk_dialog_add_action_widget(dialog, gtk_button_new_with_label("OK"), GTK_RESPONSE_ACCEPT);
	gtk_dialog_add_action_widget(dialog, gtk_button_new_with_label("Cancel"), GTK_RESPONSE_REJECT);

	notebook = gtk_notebook_new();
	for (i = 0; i < PLUGINS; i++) {
		widget = prefs[i].start(conf, GTK_WIDGET(dialog), &plugin_data[i]);
		if (widget)
			gtk_notebook_append_page(GTK_NOTEBOOK(notebook), widget, gtk_label_new(prefs[i].name));
	}

	gtk_container_add(GTK_CONTAINER(dialog->vbox), notebook);
	gtk_widget_show_all(GTK_WIDGET(dialog));
	res = gtk_dialog_run(dialog);
	if (res == GTK_RESPONSE_ACCEPT) {
		for (i = 0; i < PLUGINS; i++)
			if (prefs[i].action)
				prefs[i].action(conf, plugin_data[i]);
	}
	gtk_widget_destroy(GTK_WIDGET(dialog));

	for (i = 0; i < PLUGINS; i++)
		if (prefs[i].stop)
			prefs[i].stop(conf, plugin_data[i]);

	deinit_conf(conf);
	return OSSO_OK;
}
