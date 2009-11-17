/*
 *  Copyright (c) 2009 Roman Moravcik <roman.moravcik@gmail.com>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License
 * as published by the Free Software Foundation; version 2.1 of
 * the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301 USA
 *
 */

#include <gtk/gtk.h>
#include <hildon/hildon.h>

int main(int argc, char **argv) {
    GtkWidget *dialog, *label;

    hildon_gtk_init (&argc, &argv);

    dialog = gtk_dialog_new_with_buttons ("Ukeyboard",
                                           NULL,
                                           GTK_DIALOG_MODAL | GTK_DIALOG_DESTROY_WITH_PARENT,
                                           GTK_STOCK_OK,
                                           GTK_RESPONSE_ACCEPT,
                                           GTK_STOCK_CANCEL,
                                           GTK_RESPONSE_REJECT,
                                           NULL);

    label = gtk_label_new ("Instalation of Additional on-screen and hardware\nkeyboard layouts finished.\nYou can change your keyboard layout settings\nvia 'Settings: Text Input'.");

    gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox), label);
    gtk_widget_show_all (GTK_WIDGET (dialog));

    gtk_dialog_run (GTK_DIALOG (dialog));

    return 0;
}
