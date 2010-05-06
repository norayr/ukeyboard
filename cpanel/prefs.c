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
#include <unistd.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <libosso.h>
#include <hildon-cp-plugin/hildon-cp-plugin-interface.h>
#include <hildon/hildon.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "prefs.h"
#include "hw.h"
#include "onscreen.h"
#include "lang.h"
#include "about.h"

#define GETTEXT_PACKAGE "osso-applet-textinput"
#include <glib/gi18n-lib.h>

static init_func inits[] = { prefs_hw_init, prefs_onscreen_init, prefs_lang_init, prefs_about_init };
#define PLUGINS		(sizeof(inits) / sizeof(init_func))
static struct prefs prefs[PLUGINS];

gboolean internal_kbd;
gboolean inside_scratchbox;

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
	GtkDialog *dialog, *about;
	GtkWidget *scroll, *widget, *vbox;
	gchar *title;
	void *plugin_data[PLUGINS];
	unsigned i;
	int res;

	(void)osso; (void)user_activated;	/* shut up, GCC */

	conf = init_conf();
	if (!conf)
		return OSSO_ERROR;

	if (access("/targets/links/scratchbox.config", F_OK) == 0)
		inside_scratchbox = TRUE;
	else
		inside_scratchbox = FALSE;

	for (i = 0; i < PLUGINS; i++) {
		inits[i](prefs + i);
	}

	dialog = GTK_DIALOG(gtk_dialog_new());
	if (!dialog) {
		deinit_conf(conf);
		return OSSO_ERROR;
	}
	gtk_window_set_transient_for(GTK_WINDOW(dialog), GTK_WINDOW(data));

	title = g_strdup_printf("%s (ukeyboard)", _("tein_ti_text_input_title"));
	gtk_window_set_title(GTK_WINDOW(dialog), title);
	g_free(title);


	gtk_dialog_add_button(GTK_DIALOG(dialog), GTK_STOCK_ABOUT, GTK_RESPONSE_HELP);
	gtk_dialog_add_button(GTK_DIALOG(dialog), _HL("wdgt_bd_save"), GTK_RESPONSE_ACCEPT);

	scroll = hildon_pannable_area_new();
	g_object_set (G_OBJECT (scroll), "hscrollbar-policy", GTK_POLICY_NEVER, NULL);
	gtk_widget_set_size_request(GTK_WIDGET (scroll), -1, 345);
	gtk_container_add(GTK_CONTAINER(GTK_DIALOG(dialog)->vbox), scroll);

	vbox = gtk_vbox_new(FALSE, 0);
	hildon_pannable_area_add_with_viewport (HILDON_PANNABLE_AREA(scroll), vbox);

	gtk_widget_show_all(GTK_WIDGET(dialog));

	for (i = 0; i < PLUGINS - 1; i++) {
		widget = prefs[i].start(conf, GTK_WIDGET(dialog), &plugin_data[i]);
		if (widget)
			gtk_box_pack_start(GTK_BOX(vbox), widget, FALSE, TRUE, 0);
	}

	while ((res = gtk_dialog_run(GTK_DIALOG(dialog))) == GTK_RESPONSE_HELP) {
		about = GTK_DIALOG(gtk_dialog_new());
		gtk_window_set_title(GTK_WINDOW(about), _HL("ecdg_ti_aboutdialog_title"));
		gtk_widget_set_size_request (GTK_WIDGET (about), -1, 300);

		widget = prefs[3].start(conf, GTK_WIDGET(about), &plugin_data[3]);
		gtk_container_add(GTK_CONTAINER(GTK_DIALOG(about)->vbox), widget);

		gtk_widget_show_all(GTK_WIDGET(about));
		gtk_dialog_run(GTK_DIALOG(about));
		gtk_widget_destroy(GTK_WIDGET(about));

	}
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
