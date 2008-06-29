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

#ifndef _VKB_COMPILER_H
#define _VKB_COMPILER_H

struct compiler_ops {
	/* return -1 on eof or if error occurred */
	int (*get_line)(void *priv, void *buf, size_t size);
	/* return -1 if error occurred; error callback is not called for
	 * write errors */
	int (*write)(void *priv, void *buf, size_t size);
	off_t (*tell)(void *priv);
	void (*seek)(void *priv, off_t pos);
	/* line is -1 if general error */
	void (*error)(void *priv, int line, char *msg);
	/* return -1 if the warning should be fatal */
	int (*warning)(void *priv, int line, char *msg);
	/* can be NULL; copy the string if you need it later */
	void (*return_lang)(void *priv, char *lang);
};

int compile(struct compiler_ops *ops, void *priv);

#endif
