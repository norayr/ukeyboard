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
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "prefs.h"
#include "hw.h"

static gboolean label_focus(GtkWidget *widget, GtkDirectionType arg, gpointer data)
{
	(void)arg; (void)data;
	gtk_label_select_region(GTK_LABEL(widget), 0, 0);
	return TRUE;
}

static GtkWidget *start(GConfClient *client, GtkWidget *win, void **data)
{
	GtkScrolledWindow *scroll;
	GtkWidget *label;

	(void)client; (void)win;

	scroll = GTK_SCROLLED_WINDOW(gtk_scrolled_window_new(NULL, NULL));
	gtk_scrolled_window_set_policy(scroll, GTK_POLICY_NEVER, GTK_POLICY_AUTOMATIC);

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
#include "about.inc"
	);
	gtk_label_set_selectable(GTK_LABEL(label), TRUE);
	g_signal_connect(G_OBJECT(label), "focus", G_CALLBACK(label_focus), NULL);
	gtk_scrolled_window_add_with_viewport(scroll, label);

	*data = NULL;
	return GTK_WIDGET(scroll);
}

void prefs_about_init(struct prefs *prefs)
{
	prefs->start = start;
	prefs->action = NULL;
	prefs->stop = NULL;
	prefs->name = "About";
}
