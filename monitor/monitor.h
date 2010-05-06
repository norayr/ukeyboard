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

#ifndef MONITOR_STATUS_MENU_ITEM_H
#define MONITOR_STATUS_MENU_ITEM_H

#include <libhildondesktop/libhildondesktop.h>

G_BEGIN_DECLS

#define MONITOR_TYPE_STATUS_MENU_ITEM            (monitor_status_menu_item_get_type ())
#define MONITOR_STATUS_MENU_ITEM(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MONITOR_TYPE_STATUS_MENU_ITEM, MonitorStatusMenuItem))
#define MONITOR_STATUS_MENU_ITEM_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MONITOR_TYPE_STATUS_MENU_ITEM, MonitorStatusMenuItemClass))
#define MONITOR_IS_STATUS_MENU_ITEM(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MONITOR_TYPE_STATUS_MENU_ITEM))
#define MONITOR_IS_STATUS_MENU_ITEM_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MONITOR_TYPE_STATUS_MENU_ITEM))
#define MONITOR_STATUS_MENU_ITEM_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MONITOR_TYPE_STATUS_MENU_ITEM, MonitorStatusMenuItemClass))

typedef struct _MonitorStatusMenuItem        MonitorStatusMenuItem;
typedef struct _MonitorStatusMenuItemClass   MonitorStatusMenuItemClass;
typedef struct _MonitorStatusMenuItemPrivate MonitorStatusMenuItemPrivate;

struct _MonitorStatusMenuItem
{
  HDStatusMenuItem parent;

  MonitorStatusMenuItemPrivate *priv;
};

struct _MonitorStatusMenuItemClass
{
  HDStatusMenuItemClass parent;
};

GType monitor_status_menu_item_get_type (void);

G_END_DECLS

#endif

