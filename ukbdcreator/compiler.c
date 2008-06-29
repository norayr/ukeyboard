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

#include <sys/types.h>
#include <sys/wait.h>
#include <unistd.h>
#include <stdlib.h>
#include <gtk/gtk.h>
#include <glib/gstdio.h>
#include <gconf/gconf.h>
#include <gconf/gconf-client.h>
#include "../vkb_compiler.h"
#include "ukbdcreator.h"

static gchar *saved_layout = NULL;
static gchar *saved_lang = NULL;
static gchar *act_layout = NULL;

struct priv {
	char *buf;
	size_t pos;
	int fout;
	gchar *lang;
};

static void error(void *p, int line, char *msg)
{
	(void)p;
	disp_compile_error(line, msg);
}

static int warning(void *p, int line, char *msg)
{
	(void)p;
	disp_compile_error(line, msg);
	return -1;
}

static int get_line(void *p, void *buf, size_t size)
{
	struct priv *priv = p;
	size_t dpos = 0;
	char *dbuf = buf;

	while (priv->buf[priv->pos]) {
		if (dpos + 1 < size)
			dbuf[dpos++] = priv->buf[priv->pos];
		if (priv->buf[priv->pos++] == '\n')
			break;
	}
	dbuf[dpos] = '\0';
	return (priv->buf[priv->pos] || dpos) ? 0 : -1;
}

static int write_buf(void *p, void *buf, size_t size)
{
	struct priv *priv = p;

	if (write(priv->fout, buf, size) != (ssize_t)size) {
		disp_error("Error writing temporary file");
		return -1;
	}
	return 0;
}

static off_t tell_buf(void *p)
{
	struct priv *priv = p;

	return lseek(priv->fout, 0, SEEK_CUR);
}

static void seek_buf(void *p, off_t pos)
{
	struct priv *priv = p;

	lseek(priv->fout, pos, SEEK_SET);
}

static void return_lang(void *p, char *lang)
{
	struct priv *priv = p;

	priv->lang = g_strdup(lang);
}

static struct compiler_ops ops = {
	.get_line = get_line,
	.write = write_buf,
	.tell = tell_buf,
	.seek = seek_buf,
	.error = error,
	.warning = warning,
	.return_lang = return_lang,
};

gboolean compile_layout(gchar *buf, gchar **fname, gchar **lang)
{
	struct priv priv;
	int res;

	priv.buf = buf;
	priv.pos = 0;
	priv.lang = NULL;

	*fname = g_build_filename(g_get_tmp_dir(), "ukbdXXXXXX", NULL);
	priv.fout = g_mkstemp(*fname);
	if (priv.fout < 0) {
		g_free(*fname);
		disp_error("Error creating temporary file");
		return FALSE;
	}
	res = compile(&ops, &priv);
	close(priv.fout);
	if (res < 0) {
		g_unlink(*fname);
		g_free(*fname);
		g_free(priv.lang);
		return FALSE;
	}
	*lang = priv.lang;
	return TRUE;
}

static gchar *get_lang(GConfClient *conf)
{
	return gconf_client_get_string(conf,
		"/apps/osso/inputmethod/hildon-im-languages/language-1", NULL);
}

static void set_lang(GConfClient *conf, gchar *val)
{
	gconf_client_set_string(conf,
		"/apps/osso/inputmethod/hildon-im-languages/language-1", val, NULL);
}

void test_layout(GConfClient *conf, gchar *fname, gchar *lang)
{
	gchar *cmd;
	int res;

	cmd = g_strdup_printf("sudo /usr/libexec/ukeyboard-set -s %s %s", fname, lang);
	res = system(cmd);
	g_free(cmd);
	if (!WIFEXITED(res)) {
		disp_error("Cannot execute helper script");
		return;
	}
	res = WEXITSTATUS(res);
	if (res == 2) {
		disp_error("Redefining a system language is not possible");
		return;
	}
	if (res) {
		disp_error("Activating of the layout failed");
		return;
	}
	if (!saved_lang) {
		saved_lang = g_strdup(lang);
		saved_layout = get_lang(conf);
	}
	set_lang(conf, "en_GB");
	set_lang(conf, lang);
	if (act_layout) {
		g_unlink(act_layout);
		g_free(act_layout);
	}
	act_layout = g_strdup(fname);
	disp_info("Layout activated");
}

void restore_layout(GConfClient *conf, gboolean warn)
{
	gchar *cmd;
	int res;

	if (!saved_lang) {
		if (warn)
			disp_info("No layout to restore");
		return;
	}
	cmd = g_strdup_printf("sudo /usr/libexec/ukeyboard-set -r %s", saved_lang);
	res = system(cmd);
	g_free(cmd);
	if (!WIFEXITED(res)) {
		disp_error("Cannot execute helper script");
		return;
	}
	if (WEXITSTATUS(res)) {
		disp_error("Restoring of original layout failed");
		return;
	}
	set_lang(conf, "en_GB");
	set_lang(conf, saved_layout);
	g_free(saved_lang);
	g_free(saved_layout);
	saved_lang = saved_layout = NULL;
	if (act_layout) {
		g_unlink(act_layout);
		g_free(act_layout);
		act_layout = NULL;
	}
}

