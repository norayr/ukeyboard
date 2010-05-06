/*
 *  Copyright (c) 2010 Roman Moravcik <roman.moravcik@gmail.com>
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

#include <string.h>
#include <gtk/gtk.h>
#include <hildon/hildon.h>

#include <gconf/gconf.h>
#include <gconf/gconf-client.h>

#include "../cpanel/prefs.h"
#include "monitor.h"

#define GETTEXT_PACKAGE "osso-applet-textinput"
#include <glib/gi18n-lib.h>

#define MONITOR_STATUS_MENU_ITEM_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE (obj, MONITOR_TYPE_STATUS_MENU_ITEM, MonitorStatusMenuItemPrivate))

struct _MonitorStatusMenuItemPrivate
{
	GConfClient *conf;
};

HD_DEFINE_PLUGIN_MODULE (MonitorStatusMenuItem, monitor_status_menu_item, HD_TYPE_STATUS_MENU_ITEM);

static void
monitor_status_menu_item_class_finalize (MonitorStatusMenuItemClass *klass)
{
}

static void
monitor_status_menu_item_finalize (GObject *object)
{
	MonitorStatusMenuItem *item = MONITOR_STATUS_MENU_ITEM (object);
	MonitorStatusMenuItemPrivate *priv = item->priv;

	if(priv->conf) {
		g_object_unref (G_OBJECT (priv->conf));
		priv->conf = NULL;
	}

	G_OBJECT_CLASS (monitor_status_menu_item_parent_class)->finalize (object);
}

static void
monitor_status_menu_item_class_init (MonitorStatusMenuItemClass *klass)
{
	GObjectClass *object_class = G_OBJECT_CLASS (klass);

	object_class->finalize = monitor_status_menu_item_finalize;

	g_type_class_add_private (klass, sizeof (MonitorStatusMenuItemPrivate));
}

static void
monitor_status_menu_item_init (MonitorStatusMenuItem *item)
{
	MonitorStatusMenuItemPrivate *priv;

	priv = item->priv = MONITOR_STATUS_MENU_ITEM_GET_PRIVATE (item);

	priv->conf = gconf_client_get_default ();
	if (!priv->conf) {
		/* FIXME: error? */
	}
	gconf_client_add_dir (priv->conf, IM_CONF_DIR, GCONF_CLIENT_PRELOAD_RECURSIVE, NULL);
}
