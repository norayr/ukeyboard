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
#include <hildon/hildon.h>
#include <libosso.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "prefs.h"
#include "hw.h"

static GtkWidget *start(GConfClient *client, GtkWidget *win, void **data)
{
	GtkWidget *scroll;
	GtkWidget *label;

	(void)client; (void)win;

	scroll = hildon_pannable_area_new();

	label = gtk_label_new(NULL);
	gtk_label_set_markup(GTK_LABEL(label),
#include "about.inc"
	);
	hildon_pannable_area_add_with_viewport (HILDON_PANNABLE_AREA(scroll), label);
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
