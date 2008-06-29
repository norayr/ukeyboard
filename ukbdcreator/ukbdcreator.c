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

#include <string.h>
#include <glib.h>
#include <gtk/gtk.h>
#include <hildon/hildon.h>
#include <hildon/hildon-file-chooser-dialog.h>
#include <libosso.h>
#include <libgnomevfs/gnome-vfs.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "version.h"
#include "ukbdcreator.h"

static HildonWindow *window_main;
static GtkClipboard *clipboard;
static GtkTextView *view;
static GtkTextBuffer *buffer;
static gchar *filename = NULL;
static gboolean is_fullscreen = FALSE;
static gboolean is_modified = FALSE;

static int last_error_line = 0;
static gchar *last_error_msg = NULL;

static GConfClient *conf;


static void toggle_fullscreen(void)
{
	if (is_fullscreen)
		gtk_window_unfullscreen(GTK_WINDOW(window_main));
	else
		gtk_window_fullscreen(GTK_WINDOW(window_main));
	is_fullscreen = !is_fullscreen;
}

static void set_modified(void)
{
	is_modified = TRUE;
}

static void set_filename(gchar *fname)
{
	if (filename)
		g_free(filename);
	filename = fname;
}

void disp_error(gchar *msg)
{
	hildon_banner_show_information(GTK_WIDGET(window_main), GTK_STOCK_DIALOG_ERROR, msg);
}

void disp_info(gchar *msg)
{
	hildon_banner_show_information(GTK_WIDGET(window_main), GTK_STOCK_DIALOG_INFO, msg);
}

static void __disp_compile_error(void)
{
	GtkTextIter iter;

	if (!last_error_msg)
		return;
	if (last_error_line > 0) {
		gtk_text_buffer_get_iter_at_line(buffer, &iter, last_error_line - 1);
		gtk_text_buffer_place_cursor(buffer, &iter);
		gtk_text_view_scroll_mark_onscreen(view, gtk_text_buffer_get_mark(buffer, "insert"));
	}
	hildon_banner_show_information(GTK_WIDGET(window_main), GTK_STOCK_DIALOG_ERROR, last_error_msg);
}

void disp_compile_error(int line, gchar *msg)
{
	g_free(last_error_msg);
	last_error_msg = g_strdup(msg);
	last_error_line = line;
	__disp_compile_error();
}

static gboolean read_file(gchar *fname)
{
	GnomeVFSFileInfo finfo;
	GnomeVFSHandle *handle = NULL;
	GnomeVFSFileSize bytes;
	gchar *buf;

	if (gnome_vfs_get_file_info(fname, &finfo, GNOME_VFS_FILE_INFO_DEFAULT) != GNOME_VFS_OK ||
			gnome_vfs_open(&handle, fname, GNOME_VFS_OPEN_READ) != GNOME_VFS_OK) {
		disp_error("Error opening file");
		return FALSE;
	}
	buf = g_malloc0(finfo.size + 1);
	if (!buf) {
		gnome_vfs_close(handle);
		disp_error("Not enough memory");
		return FALSE;
	}
	if (gnome_vfs_read(handle, buf, finfo.size, &bytes) != GNOME_VFS_OK || bytes != finfo.size) {
		gnome_vfs_close(handle);
		g_free(buf);
		disp_error("Error reading file");
		return FALSE;
	}
	gnome_vfs_close(handle);

	gtk_text_buffer_set_text(buffer, buf, -1);
	g_free(buf);
	return TRUE;
}

static gchar *get_buffer(void)
{
	GtkTextIter start, end;

	gtk_text_buffer_get_bounds(buffer, &start, &end);
	return gtk_text_buffer_get_slice(buffer, &start, &end, TRUE);
}

static gboolean write_file(gchar *fname)
{
	GnomeVFSHandle *handle = NULL;
	GnomeVFSFileSize bytes;
	gchar *buf;
	size_t size;

	if (gnome_vfs_create(&handle, fname, GNOME_VFS_OPEN_WRITE, FALSE, 0644) != GNOME_VFS_OK) {
		disp_error("Error creating file");
		return FALSE;
	}
	buf = get_buffer();
	size = strlen(buf);
	if (gnome_vfs_write(handle, buf, size, &bytes) != GNOME_VFS_OK || bytes != size) {
		gnome_vfs_close(handle);
		g_free(buf);
		disp_error("Error writing file");
		return FALSE;
	}
	gnome_vfs_close(handle);
	g_free(buf);
	disp_info("Saved");
	return TRUE;
}

static gchar *file_chooser(gboolean open)
{
	GtkWidget *chooser;
	GtkFileFilter *filter;
	gchar *result = NULL;

	chooser = hildon_file_chooser_dialog_new(GTK_WINDOW(window_main),
		open ? GTK_FILE_CHOOSER_ACTION_OPEN : GTK_FILE_CHOOSER_ACTION_SAVE);
	filter = gtk_file_filter_new();
	gtk_file_filter_add_pattern(filter, "*.def");
	gtk_file_chooser_set_filter(GTK_FILE_CHOOSER(chooser), filter);
	gtk_widget_show_all(chooser);
	if (gtk_dialog_run(GTK_DIALOG(chooser)) == GTK_RESPONSE_OK)
		result = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(chooser));
	gtk_widget_destroy(chooser);
	if (!open && result && !g_str_has_suffix(result, ".def")) {
		gchar *tmp = result;

		result = g_strconcat(result, ".def", NULL);
		g_free(tmp);
	}
	return result;
}

static void file_save_as(void)
{
	gchar *fname;

	fname = file_chooser(FALSE);
	if (fname) {
		if (write_file(fname)) {
			set_filename(fname);
			is_modified = FALSE;
		} else
			g_free(fname);
	}
}

static void file_save(void)
{
	if (!filename) {
		file_save_as();
		return;
	}
	if (write_file(filename))
		is_modified = FALSE;
}

static gboolean file_ask_save(void)
{
	GtkWidget *note;
	gint res;

	if (!is_modified)
		return TRUE;
	note = hildon_note_new_confirmation_add_buttons(GTK_WINDOW(window_main),
		"Save the file before closing?", "Yes", 1, "No", 2, "Cancel", 0, NULL);
	res = gtk_dialog_run(GTK_DIALOG(note));
	gtk_widget_destroy(note);
	if (res == 1) {
		file_save_as();
		return !is_modified;
	}
	if (res == 2)
		return TRUE;
	return FALSE;
}

static void file_open(void)
{
	gchar *fname;

	if (!file_ask_save())
		return;
	fname = file_chooser(TRUE);
	if (fname) {
		if (read_file(fname)) {
			set_filename(fname);
			is_modified = FALSE;
			g_free(last_error_msg);
			last_error_msg = NULL;
		} else
			g_free(fname);
	}
}

static void file_new(void)
{
	if (!file_ask_save())
		return;
	gtk_text_buffer_set_text(buffer, "", -1);
	set_filename(NULL);
	is_modified = FALSE;
	g_free(last_error_msg);
	last_error_msg = NULL;
}

static gboolean file_quit(void)
{
	if (!file_ask_save())
		return TRUE;
	restore_layout(conf, FALSE);
	gtk_main_quit();
	return FALSE;
}

static void edit_cut(void)
{
	gtk_text_buffer_cut_clipboard(buffer, clipboard, TRUE);
}

static void edit_copy(void)
{
	gtk_text_buffer_copy_clipboard(buffer, clipboard);
}

static void edit_paste(void)
{
	gtk_text_buffer_paste_clipboard(buffer, clipboard, NULL, TRUE);
}

static void run_about(void)
{
	gtk_show_about_dialog(GTK_WINDOW(window_main),
		"version", UKBD_VERSION,
		"comments", "Licensed under GPLv2. Please send bug reports and feature requests to jbenc@upir.cz.",
		"copyright", "(c) 2008 Jiri Benc",
		"website", "http://upir.cz/maemo/keyboards/");
}

static void compile_and_test(void)
{
	gchar *buf, *fname, *lang;
	gboolean res;

	buf = get_buffer();
	res = compile_layout(buf, &fname, &lang);
	g_free(buf);
	if (res) {
		test_layout(conf, fname, lang);
		g_free(fname);
		g_free(lang);
	}
}

static void untest(void)
{
	restore_layout(conf, TRUE);
}

static gboolean key_pressed(GtkWidget *widget, GdkEventKey *event)
{
	(void)widget;
	switch (event->keyval) {
	case GDK_F6:
		toggle_fullscreen();
		return TRUE;
	}
	return FALSE;
}

static GtkWidget *main_layout(void)
{
	GtkWidget *scroll;

	view = GTK_TEXT_VIEW(gtk_text_view_new());
	gtk_text_view_set_editable(view, TRUE);
	gtk_text_view_set_left_margin(view, 10);
	gtk_text_view_set_right_margin(view, 10);

	scroll = gtk_scrolled_window_new(NULL, NULL);
	gtk_scrolled_window_set_policy(GTK_SCROLLED_WINDOW(scroll), GTK_POLICY_AUTOMATIC, GTK_POLICY_AUTOMATIC);
	gtk_container_add(GTK_CONTAINER(scroll), GTK_WIDGET(view));

	buffer = gtk_text_view_get_buffer(view);
	g_signal_connect(G_OBJECT(buffer), "changed", G_CALLBACK(set_modified), NULL);
	g_signal_connect(G_OBJECT(buffer), "modified-changed", G_CALLBACK(set_modified), NULL);

	return scroll;
}

static GtkMenu *main_menu(void)
{
	GtkMenuShell *menu, *submenu;
	GtkWidget *item;

	menu = GTK_MENU_SHELL(gtk_menu_new());

	submenu = GTK_MENU_SHELL(gtk_menu_new());
	item = gtk_menu_item_new_with_label("File");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), GTK_WIDGET(submenu));
	gtk_menu_shell_append(menu, item);

	item = gtk_menu_item_new_with_label("New");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(file_new), NULL);
	gtk_menu_shell_append(submenu, item);
	item = gtk_menu_item_new_with_label("Open...");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(file_open), NULL);
	gtk_menu_shell_append(submenu, item);
	item = gtk_menu_item_new_with_label("Save");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(file_save), NULL);
	gtk_menu_shell_append(submenu, item);
	item = gtk_menu_item_new_with_label("Save as...");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(file_save_as), NULL);
	gtk_menu_shell_append(submenu, item);

	submenu = GTK_MENU_SHELL(gtk_menu_new());
	item = gtk_menu_item_new_with_label("Edit");
	gtk_menu_item_set_submenu(GTK_MENU_ITEM(item), GTK_WIDGET(submenu));
	gtk_menu_shell_append(menu, item);

	item = gtk_menu_item_new_with_label("Cut");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(edit_cut), NULL);
	gtk_menu_shell_append(submenu, item);
	item = gtk_menu_item_new_with_label("Copy");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(edit_copy), NULL);
	gtk_menu_shell_append(submenu, item);
	item = gtk_menu_item_new_with_label("Paste");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(edit_paste), NULL);
	gtk_menu_shell_append(submenu, item);

	gtk_menu_shell_append(menu, gtk_separator_menu_item_new());
	item = gtk_menu_item_new_with_label("Test layout");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(compile_and_test), NULL);
	gtk_menu_shell_append(menu, item);
	item = gtk_menu_item_new_with_label("Restore original layout");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(untest), NULL);
	gtk_menu_shell_append(menu, item);
	item = gtk_menu_item_new_with_label("Show last error");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(__disp_compile_error), NULL);
	gtk_menu_shell_append(menu, item);

	gtk_menu_shell_append(menu, gtk_separator_menu_item_new());
	item = gtk_menu_item_new_with_label("Fullscreen");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(toggle_fullscreen), NULL);
	gtk_menu_shell_append(menu, item);
	item = gtk_menu_item_new_with_label("About");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(run_about), NULL);
	gtk_menu_shell_append(menu, item);
	item = gtk_menu_item_new_with_label("Exit");
	g_signal_connect(G_OBJECT(item), "activate", G_CALLBACK(file_quit), NULL);
	gtk_menu_shell_append(menu, item);

	return GTK_MENU(menu);
}

static GtkToolbar *main_toolbar(void)
{
	GtkToolbar *bar;
	GtkToolItem *item;

	bar = GTK_TOOLBAR(gtk_toolbar_new());
	gtk_toolbar_set_orientation(bar, GTK_ORIENTATION_HORIZONTAL);
	gtk_toolbar_set_style(bar, GTK_TOOLBAR_BOTH_HORIZ);

	item = gtk_tool_button_new_from_stock(GTK_STOCK_OPEN);
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(file_open), NULL);
	gtk_toolbar_insert(bar, item, -1);
	item = gtk_tool_button_new_from_stock(GTK_STOCK_SAVE);
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(file_save), NULL);
	gtk_toolbar_insert(bar, item, -1);
	gtk_toolbar_insert(bar, gtk_separator_tool_item_new(), -1);
	item = gtk_tool_button_new_from_stock(GTK_STOCK_CUT);
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(edit_cut), NULL);
	gtk_toolbar_insert(bar, item, -1);
	item = gtk_tool_button_new_from_stock(GTK_STOCK_COPY);
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(edit_copy), NULL);
	gtk_toolbar_insert(bar, item, -1);
	item = gtk_tool_button_new_from_stock(GTK_STOCK_PASTE);
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(edit_paste), NULL);
	gtk_toolbar_insert(bar, item, -1);
	gtk_toolbar_insert(bar, gtk_separator_tool_item_new(), -1);
	item = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_PLAY);
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(compile_and_test), NULL);
	gtk_toolbar_insert(bar, item, -1);
	item = gtk_tool_button_new_from_stock(GTK_STOCK_MEDIA_STOP);
	g_signal_connect(G_OBJECT(item), "clicked", G_CALLBACK(untest), NULL);
	gtk_toolbar_insert(bar, item, -1);

	return bar;
}

int main(int argc, char **argv)
{
	HildonProgram *program;

	gtk_init(&argc, &argv);
	program = hildon_program_get_instance();
	g_set_prgname("ukbdcreator");
	g_set_application_name("Ukeyboard Creator");
	osso_initialize("cz.upir.ukbdcreator", UKBD_VERSION, TRUE, NULL);
	gnome_vfs_init();
	conf = gconf_client_get_default();
	gconf_client_add_dir(conf, "/apps/osso/inputmethod/hildon-im-languages",
		GCONF_CLIENT_PRELOAD_NONE, NULL);

	window_main = HILDON_WINDOW(hildon_window_new());
	hildon_program_add_window(program, window_main);
	clipboard = gtk_clipboard_get(GDK_SELECTION_CLIPBOARD);
	gtk_clipboard_set_can_store(clipboard, NULL, 0);

	gtk_container_add(GTK_CONTAINER(window_main), main_layout());
	hildon_window_set_menu(window_main, main_menu());
	hildon_window_add_toolbar(window_main, main_toolbar());

	g_signal_connect(G_OBJECT(window_main), "key_press_event", G_CALLBACK(key_pressed), NULL);
	g_signal_connect(G_OBJECT(window_main), "delete_event", G_CALLBACK(file_quit), NULL);

	gtk_widget_show_all(GTK_WIDGET(window_main));
	gtk_main();
	return 0;
}
