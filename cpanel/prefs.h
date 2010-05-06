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

#ifndef _PREFS_H
#define _PREFS_H

#define _HL(str) dgettext("hildon-libs",str)

#define IM_CONF_DIR "/apps/osso/inputmethod"

struct prefs {
	GtkWidget *(*start)(GConfClient *, GtkWidget *, void **);
	void (*action)(GConfClient *, void *);
	void (*stop)(GConfClient *, void *);
	char *name;
};

typedef void (*init_func)(struct prefs *);

extern gboolean internal_kbd;
extern gboolean inside_scratchbox;

gboolean get_bool(GConfClient *client, char *key);
void set_bool(GConfClient *client, char *key, gboolean val);
gint get_int(GConfClient *client, char *key);
void set_int(GConfClient *client, char *key, gint val);
gchar *get_str(GConfClient *client, char *key);
void set_str(GConfClient *client, char *key, gchar *val);

#endif
