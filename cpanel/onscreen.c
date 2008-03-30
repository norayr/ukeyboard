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
#include <hildon/hildon-controlbar.h>
#include <device_symbols.h>
#include <libosso.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "prefs.h"
#include "onscreen.h"

struct data {
	GtkToggleButton *stylus_im;
	GtkToggleButton *use_finger;
	GtkToggleButton *use_finger_sel;
	GtkToggleButton *case_corr;
	HildonControlbar *hand_speed;
};

static gint get_hand_speed(GConfClient *client)
{
	/* 150 300 400 600 900 */
	gint res = get_int(client, "handwriting_timeout") / 200 + 1;
	if (res > 5)
		res = 5;
	if (res < 1)
		res = 1;
	return res;
}

static void set_hand_speed(GConfClient *client, gint val)
{
	switch (val) {
	case 1: val = 150; break;
	case 2: val = 300; break;
	case 3: val = 400; break;
	case 4: val = 600; break;
	case 5: val = 900; break;
	}
	set_int(client, "handwriting_timeout", val);
}

static GtkWidget *start(GConfClient *client, GtkWidget *win, void **data)
{
	struct data *d;
	GtkBox *vbox;
	GtkSizeGroup *group;
	GtkWidget *align, *caption;

	(void)win;
	d = g_new0(struct data, 1);

	vbox = GTK_BOX(gtk_vbox_new(FALSE, 0));
	group = gtk_size_group_new(GTK_SIZE_GROUP_HORIZONTAL);

	if (internal_kbd) {
		d->stylus_im = GTK_TOGGLE_BUTTON(gtk_check_button_new());
		gtk_toggle_button_set_active(d->stylus_im, get_bool(client, "enable-stylus-im"));
		gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Enable stylus input methods",
			GTK_WIDGET(d->stylus_im), NULL, HILDON_CAPTION_MANDATORY));
	}

	d->use_finger = GTK_TOGGLE_BUTTON(gtk_check_button_new());
	gtk_toggle_button_set_active(d->use_finger, get_bool(client, "use_finger_kb"));
	gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Launch finger keyboard with finger tap",
		GTK_WIDGET(d->use_finger), NULL, HILDON_CAPTION_MANDATORY));

	if (!internal_kbd) {
		d->use_finger_sel = GTK_TOGGLE_BUTTON(gtk_check_button_new());
		gtk_toggle_button_set_active(d->use_finger_sel, get_bool(client, "launch_finger_kb_on_select"));
		caption = hildon_caption_new(group, NULL,
			GTK_WIDGET(d->use_finger_sel), NULL, HILDON_CAPTION_MANDATORY);
		hildon_caption_set_label_markup(HILDON_CAPTION(caption), "Launch finger keyboard with " HWK_BUTTON_SELECT);
		gtk_box_pack_start_defaults(vbox, caption);
	}

	d->hand_speed = HILDON_CONTROLBAR(hildon_controlbar_new());
	hildon_controlbar_set_range(d->hand_speed, 1, 5);
	hildon_controlbar_set_value(d->hand_speed, get_hand_speed(client));
	gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Handwriting speed",
		GTK_WIDGET(d->hand_speed), NULL, HILDON_CAPTION_MANDATORY));

	d->case_corr = GTK_TOGGLE_BUTTON(gtk_check_button_new());
	gtk_toggle_button_set_active(d->case_corr, get_bool(client, "case_correction"));
	gtk_box_pack_start_defaults(vbox, hildon_caption_new(group, "Handwriting case correction",
		GTK_WIDGET(d->case_corr), NULL, HILDON_CAPTION_MANDATORY));

	g_object_unref(G_OBJECT(group));
	*data = d;

	align = gtk_alignment_new(0, 0, 1, 0);
	gtk_container_add(GTK_CONTAINER(align), GTK_WIDGET(vbox));
	return align;
}

static void action(GConfClient *client, void *data)
{
	struct data *d = data;

	if (d->stylus_im)
		set_bool(client, "enable-stylus-im", gtk_toggle_button_get_active(d->stylus_im));
	set_bool(client, "use_finger_kb", gtk_toggle_button_get_active(d->use_finger));
	if (d->use_finger_sel)
		set_bool(client, "launch_finger_kb_on_select", gtk_toggle_button_get_active(d->use_finger_sel));
	set_hand_speed(client, hildon_controlbar_get_value(d->hand_speed));
	set_bool(client, "case_correction", gtk_toggle_button_get_active(d->case_corr));
}

void prefs_onscreen_init(struct prefs *prefs)
{
	prefs->start = start;
	prefs->action = action;
	prefs->stop = NULL;
	prefs->name = internal_kbd ? "On-screen" : "General";
}
