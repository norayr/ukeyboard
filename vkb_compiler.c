/*
 *  Copyright (c) 2007-2008 Jiri Benc <jbenc@upir.cz>
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
#include <stdlib.h>
#include "vkb_compiler.h"

struct priv {
	FILE *fin;
	FILE *fout;
	char *fin_name;
	char *fout_name;
};

static void error(void *p, int line, char *msg)
{
	struct priv *priv = p;

	if (line < 0)
		fprintf(stderr, "%s: Error: %s.\n", priv->fin_name, msg);
	else
		fprintf(stderr, "%s:%d: Error: %s.\n", priv->fin_name, line, msg);
}

static int warning(void *p, int line, char *msg)
{
	struct priv *priv = p;

	fprintf(stderr, "%s:%d: Warning: %s.\n", priv->fin_name, line, msg);
	return 0;
}

static int get_line(void *p, void *buf, size_t size)
{
	struct priv *priv = p;

	return fgets(buf, size, priv->fin) ? 0 : -1;
}

static int write_buf(void *p, void *buf, size_t size)
{
	struct priv *priv = p;

	if (fwrite(buf, 1, size, priv->fout) != size) {
		fprintf(stderr, "Error writing %s.\n", priv->fout_name);
		return -1;
	}
	return 0;
}

static off_t tell_buf(void *p)
{
	struct priv *priv = p;

	return ftell(priv->fout);
}

static void seek_buf(void *p, off_t pos)
{
	struct priv *priv = p;

	fseek(priv->fout, pos, SEEK_SET);
}

static void init_input(struct priv *priv, char *fname)
{
	priv->fin_name = fname;
	priv->fin = fopen(fname, "r");
	if (!priv->fin) {
		error(priv, -1, "Cannot open input file");
		exit(1);
	}
}

static void init_output(struct priv *priv, char *fname)
{
	priv->fout_name = fname;
	priv->fout = fopen(fname, "w");
	if (!priv->fout) {
		fprintf(stderr, "Error creating %s.\n", fname);
		exit(1);
	}
}

static void close_input(struct priv *priv)
{
	fclose(priv->fin);
}

static void close_output(struct priv *priv)
{
	fclose(priv->fout);
}

static struct compiler_ops ops = {
	.get_line = get_line,
	.write = write_buf,
	.tell = tell_buf,
	.seek = seek_buf,
	.error = error,
	.warning = warning,
};

int main(int argc, char **argv)
{
	struct priv priv;
	int res;

	if (argc < 3) {
		fprintf(stderr, "Missing parameters (source and destination filename).\n");
		exit(1);
	}
	init_input(&priv, argv[1]);
	init_output(&priv, argv[2]);
	res = compile(&ops, &priv);
	close_output(&priv);
	close_input(&priv);
	return res ? 1 : 0;
}
