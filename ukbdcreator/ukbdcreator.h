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

#ifndef _UKBDCREATOR_H
#define _UKBDCREATOR_H

void disp_error(gchar *msg);
void disp_info(gchar *msg);
void disp_compile_error(int line, gchar *msg);

gboolean compile_layout(gchar *buf, gchar **fname, gchar **lang);
void test_layout(GConfClient *conf, gchar *fname, gchar *lang);
void restore_layout(GConfClient *conf, gboolean warn);

#endif
