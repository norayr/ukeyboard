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
#include <hildon/hildon-controlbar.h>
#include <hildon/hildon.h>
#include <device_symbols.h>
#include <libosso.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "prefs.h"
#include "onscreen.h"

#define GETTEXT_PACKAGE "osso-applet-textinput"
#include <glib/gi18n-lib.h>

struct data {
	HildonCheckButton *use_finger;
};

static GtkWidget *start(GConfClient *client, GtkWidget *win, void **data)
{
	struct data *d;
	GtkWidget *vbox;

	(void)win;
	d = g_new0(struct data, 1);

	vbox = gtk_vbox_new(FALSE, 0);

	d->use_finger = HILDON_CHECK_BUTTON(hildon_check_button_new(HILDON_SIZE_FINGER_HEIGHT));
	hildon_check_button_set_active(d->use_finger, get_bool(client, "use_finger_kb"));
	gtk_button_set_label (GTK_BUTTON (d->use_finger), _("tein_fi_use_virtual_keyboard"));
	gtk_box_pack_start(GTK_BOX(vbox), GTK_WIDGET(d->use_finger), TRUE, TRUE, 0);

	*data = d;

	gtk_widget_show_all(vbox);

	return vbox;
}

static void action(GConfClient *client, void *data)
{
	struct data *d = data;

	set_bool(client, "use_finger_kb", hildon_check_button_get_active(d->use_finger));
}

void prefs_onscreen_init(struct prefs *prefs)
{
	prefs->start = start;
	prefs->action = action;
	prefs->stop = NULL;
	prefs->name = internal_kbd ? "On-screen" : "General";
}
