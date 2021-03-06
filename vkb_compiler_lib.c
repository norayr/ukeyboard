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

#include <stdlib.h>
#include <string.h>
#include <setjmp.h>
#include "vkb_compiler.h"

static struct compiler_ops *cops;
static void *cpriv;
static jmp_buf cjmp;

#define BUF_SIZE	8192

/*** tokenizer ***/

struct tokenizer {
	int line;
	char *buf, *pos;
	char *pushed;
};

static char keywords[][32] = {
	"header", "name", "lang", "wc", "size", "width", "height",
	"textpos", "left", "top", "kbd_normal", "kbd_thumb", "kbd_special",
	"lowercase", "lowercase_num", "uppercase", "uppercase_num",
	"special_lowercase", "special_uppercase",
	"special", "margin", "default_size", "row", "key", "slide",
	"white", "tab", "backspace", "shift", "alpha", "num", "hexa", "tele", "dead"
};
enum keywords_const {
	TOK_HEADER, TOK_NAME, TOK_LANG, TOK_WC, TOK_SIZE, TOK_WIDTH, TOK_HEIGHT,
	TOK_TEXTPOS, TOK_LEFT, TOK_TOP, TOK_KBD_NORMAL, TOK_KBD_THUMB, TOK_KBD_SPECIAL,
	TOK_LOWERCASE, TOK_LOWERCASE_NUM, TOK_UPPERCASE, TOK_UPPERCASE_NUM,
	TOK_SPEC_LOWERCASE, TOK_SPEC_UPPERCASE,
	TOK_SPECIAL, TOK_MARGIN, TOK_DEFAULT_SIZE, TOK_ROW, TOK_KEY, TOK_SLIDE,
	TOK_WHITE, TOK_TAB, TOK_BACKSPACE, TOK_SHIFT, TOK_ALPHA, TOK_NUM, TOK_HEXA, TOK_TELE, TOK_DEAD
};

enum tok_type {
	TT_KEYWORD, TT_NUM, TT_STRING, TT_BEGIN, TT_END, TT_EOF
};

static void tok_error(struct tokenizer *tokenizer, char *msg)
{
	cops->error(cpriv, tokenizer->line, msg);
	longjmp(cjmp, 1);
}

static void tok_warning(struct tokenizer *tokenizer, char *msg)
{
	if (cops->warning(cpriv, tokenizer->line, msg))
		longjmp(cjmp, 1);
}

static void *e_malloc(size_t size)
{
	void *p = calloc(1, size);
	if (!p) {
		cops->error(cpriv, -1, "Out of memory");
		longjmp(cjmp, 1);
	}
	return p;
}

static void set_str(char **str, char *val)
{
	if (*str)
		free(*str);
	if (!val) {
		*str = NULL;
		return;
	}
	*str = e_malloc(strlen(val) + 1);
	strcpy(*str, val);
}

static char *ltrim(char *buf)
{
	while (buf && (*buf == ' ' || *buf == '\t' || *buf == '\n' || *buf == '\r'))
		buf++;
	return buf;
}

static char *find_white(char *buf)
{
	while (buf && *buf && *buf != ' ' && *buf != '\t' && *buf != '\n' && *buf != '\r')
		buf++;
	return buf;
}

static int read_raw(struct tokenizer *tokenizer, char *dest, size_t size, int no_ltrim)
{
	char *s;
	char tmp;

	while (1) {
		if (!no_ltrim)
			tokenizer->pos = ltrim(tokenizer->pos);
		if (!*tokenizer->pos) {
			tokenizer->line++;
			tokenizer->pos = tokenizer->buf;
			if (cops->get_line(cpriv, tokenizer->buf, BUF_SIZE))
				return -1;
		} else
			break;
	}
	s = find_white(tokenizer->pos + 1);
	tmp = *s;
	*s = '\0';
	if (size) {
		strncpy(dest, tokenizer->pos, size - 1);
		dest[size - 1] = '\0';
	}
	*s = tmp;
	tokenizer->pos = s;
	return 0;
}

static void finish_read_string(struct tokenizer *tokenizer, char *dest, size_t size)
{
	char *s = dest + 1;
	char *new = dest;
	int special = 0;

	while (1) {
		while (*s) {
			if (special) {
				*new++ = *s++;
				special = 0;
				continue;
			}
			if (*s == '\\') {
				special = 1;
				s++;
				continue;
			}
			if (*s == '"') {
				*new = '\0';
				if (s[1] != '\0')
					tok_warning(tokenizer, "Ignored extra characters after the string");
				return;
			}
			*new++ = *s++;
		}
		if (read_raw(tokenizer, s, size - (s - dest), 1) < 0)
			tok_error(tokenizer, "Unterminated (or too long) string");
	}
}

static void skip_comment(struct tokenizer *tokenizer)
{
	while (*tokenizer->pos)
		tokenizer->pos++;
}

static int find_keyword(char *s)
{
	int i;
	int max = (sizeof(keywords) / sizeof(*keywords));

	for (i = 0; i < max; i++) {
		if (!strcmp(s, keywords[i]))
			return i;
	}
	return -1;
}

static void unread_tok(struct tokenizer *tokenizer, char *tok)
{
	strncpy(tokenizer->pushed, tok, BUF_SIZE - 1);
	tokenizer->pushed[BUF_SIZE - 1] = '\0';
}

static enum tok_type read_tok(struct tokenizer *tokenizer, char *dest, size_t size, int *parsed_int)
{
	int res;

	while (1) {
		if (*tokenizer->pushed) {
			strncpy(dest, tokenizer->pushed, size - 1);
			dest[size - 1] = '\0';
			*tokenizer->pushed = '\0';
		} else {
			res = read_raw(tokenizer, dest, size, 0);
			if (res < 0)
				return TT_EOF;
		}
		if (dest[0] == '#')
			skip_comment(tokenizer);
		else
			break;
	}
	if ((dest[0] >= '0' && dest[0] <= '9') || dest[0] == '-') {
		*parsed_int = atoi(dest);
		return TT_NUM;
	}
	if (dest[0] == '"') {
		finish_read_string(tokenizer, dest, size);
		return TT_STRING;
	}
	if (dest[0] == '{') {
		if (dest[1] != '\0')
			tok_error(tokenizer, "Whitespace required after {");
		return TT_BEGIN;
	}
	if (dest[0] == '}') {
		if (dest[1] != '\0')
			tok_error(tokenizer, "Whitespace required after }");
		return TT_END;
	}
	res = find_keyword(dest);
	if (res < 0)
		tok_error(tokenizer, "Uknown keyword");
	*parsed_int = res;
	return TT_KEYWORD;
}

static void init_tokenizer(struct tokenizer *tokenizer)
{
	tokenizer->pos = tokenizer->buf = e_malloc(BUF_SIZE);
	tokenizer->pushed = e_malloc(BUF_SIZE);
}

static void close_tokenizer(struct tokenizer *tokenizer)
{
	if (!tokenizer)
		return;
	free(tokenizer->buf);
	free(tokenizer->pushed);
}

/*** parser structures ***/

struct slide_key {
	char *name;
	struct slide_key *next;
};

struct key {
	int slides_cnt;
	union {
		char *name;
		struct slide_key *slides;
	} u;
	int size;
	int flags;
	struct key *next;
};

#define KEY_ALPHA	0x01
#define KEY_NUM		0x02
#define KEY_HEXA	0x04
#define KEY_TELE	0x08
#define KEY_SPECIAL	0x10
#define KEY_DEAD	0x20
#define KEY_WHITE	0x40
#define KEY_EXTEND	0x80

#define KEY_TAB		(0x0400 | KEY_EXTEND)
#define KEY_BACKSPACE	(0x0800 | KEY_EXTEND)
#define KEY_SHIFT	(0x1000 | KEY_EXTEND)

struct row {
	int keys_cnt;
	struct key *keys;
	struct row *next;
};

struct layout {
	int type;
	char *name;
	int margins[4];
	int default_size;
	int rows_cnt;
	struct row *rows;
	struct layout *sublayout;
	struct layout *next;
};

#define LAY_LOWERCASE		0
#define LAY_UPPERCASE		1
#define LAY_LOWERCASE_NUM	2
#define LAY_UPPERCASE_NUM	3
#define LAY_SPECIAL		4
#define LAY_SPECIAL_LOWER	5
#define LAY_SPECIAL_UPPER	6

struct kbd {
	int type;
	int layouts_cnt;
	struct layout *layouts;
	struct kbd *next;
};

struct size {
	int dim[5];
	struct size *next;
};

#define KBD_NORMAL		0
#define KBD_THUMB		4
#define KBD_SPECIAL		1

struct global {
	char *name;
	char *lang;
	char *wc;
	int sizes_cnt;
	struct size *sizes;
	int kbds_cnt;
	struct kbd *kbds;
};

static struct global *new_global(void)
{
	return e_malloc(sizeof(struct global));
}

static void free_sizes(struct size *size)
{
	struct size *next;

	while (size) {
		next = size->next;
		free(size);
		size = next;
	}
}

static void free_slides(struct slide_key *sl)
{
	struct slide_key *next;

	while (sl) {
		next = sl->next;
		free(sl->name);
		free(sl);
		sl = next;
	}
}

static void free_keys(struct key *key)
{
	struct key *next;

	while (key) {
		next = key->next;
		if (key->slides_cnt)
			free_slides(key->u.slides);
		else
			free(key->u.name);
		free(key);
		key = next;
	}
}

static void free_rows(struct row *row)
{
	struct row *next;

	while (row) {
		next = row->next;
		free_keys(row->keys);
		free(row);
		row = next;
	}
}

static void free_layouts(struct layout *lay)
{
	struct layout *next;

	while (lay) {
		next = lay->next;
		free_rows(lay->rows);
		free_layouts(lay->sublayout);
		free(lay->name);
		free(lay);
		lay = next;
	}
}

static void free_kbds(struct kbd *kbd)
{
	struct kbd *next;

	while (kbd) {
		next = kbd->next;
		free_layouts(kbd->layouts);
		free(kbd);
		kbd = next;
	}
}

static void free_global(struct global *glob)
{
	if (!glob)
		return;
	free_sizes(glob->sizes);
	free_kbds(glob->kbds);
	free(glob->name);
	free(glob->lang);
	free(glob->wc);
	free(glob);
}

/*** parser ***/

struct parser {
	struct tokenizer *tokenizer;
	struct global *global;
	int ttype;
	char *tstr;
	int tint;
};

static void init_parser(struct parser *parser)
{
	parser->tokenizer = e_malloc(sizeof(struct tokenizer));
	init_tokenizer(parser->tokenizer);
	parser->tstr = e_malloc(BUF_SIZE);
	parser->global = new_global();
}

static void close_parser(struct parser *parser)
{
	if (!parser)
		return;
	free_global(parser->global);
	free(parser->tstr);
	close_tokenizer(parser->tokenizer);
	free(parser->tokenizer);
}

#define error(parser, msg)	tok_error((parser)->tokenizer, msg)
#define warning(parser, msg)	tok_warning((parser)->tokenizer, msg)
#define error_end(parser)	error(parser, "} expected")

static void get_tok(struct parser *parser)
{
	parser->ttype = read_tok(parser->tokenizer, parser->tstr, BUF_SIZE, &parser->tint);
}

static void __get_raw(struct parser *parser)
{
	if (read_raw(parser->tokenizer, parser->tstr, BUF_SIZE, 0) < 0)
		error(parser, "Unexpected end of file");
}

static void push_tok(struct parser *parser)
{
	unread_tok(parser->tokenizer, parser->tstr);
}

#define is_type(parser, t)	((parser)->ttype == (t))
#define is_keyword(parser, w)	((parser)->ttype == TT_KEYWORD && (parser)->tint == (w))
#define is_begin(parser)	((parser)->ttype == TT_BEGIN)
#define is_end(parser)		((parser)->ttype == TT_END)
#define is_eof(parser)		((parser)->ttype == TT_EOF)

static void get_tok_type(struct parser *parser, int tt)
{
	get_tok(parser);
	if (!is_type(parser, tt)) {
		switch (tt) {
		case TT_NUM: error(parser, "Number expected");
		case TT_STRING: error(parser, "String expected");
		case TT_BEGIN: error(parser, "{ expected");
		case TT_END: error(parser, "} expected");
		default: error(parser, "Syntax error");
		}
	}
}

#define get_tok_num(parser)	get_tok_type(parser, TT_NUM)
#define get_tok_string(parser)	get_tok_type(parser, TT_STRING)
#define get_tok_begin(parser)	get_tok_type(parser, TT_BEGIN)
#define get_tok_end(parser)	get_tok_type(parser, TT_END)

static void parse_sizes(struct parser *parser, struct global *glob)
{
	struct size *size;

	get_tok_num(parser);
	if (parser->tint != glob->sizes_cnt)
		error(parser, "size number out of order");
	size = e_malloc(sizeof(struct size));
	glob->sizes_cnt++;
	if (!glob->sizes)
		glob->sizes = size;
	else {
		struct size *last = glob->sizes;
		while (last->next)
			last = last->next;
		last->next = size;
	}
	get_tok_begin(parser);
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_WIDTH)) {
			get_tok_num(parser);
			size->dim[0] = parser->tint;
		} else if (is_keyword(parser, TOK_HEIGHT)) {
			get_tok_num(parser);
			size->dim[1] = parser->tint;
		} else if (is_keyword(parser, TOK_TEXTPOS)) {
			get_tok_num(parser);
			size->dim[2] = parser->tint;
		} else if (is_keyword(parser, TOK_LEFT)) {
			get_tok_num(parser);
			size->dim[3] = parser->tint;
		} else if (is_keyword(parser, TOK_TOP)) {
			get_tok_num(parser);
			size->dim[4] = parser->tint;
		} else if (is_end(parser)) {
			return;
		} else
			error_end(parser);
	}
}

static void parse_header(struct parser *parser, struct global *glob)
{
	get_tok_begin(parser);
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_NAME)) {
			get_tok_string(parser);
			set_str(&glob->name, parser->tstr);
		} else if (is_keyword(parser, TOK_LANG)) {
			get_tok_string(parser);
			set_str(&glob->lang, parser->tstr);
		} else if (is_keyword(parser, TOK_WC)) {
			get_tok_string(parser);
			set_str(&glob->wc, parser->tstr);
		} else if (is_keyword(parser, TOK_SIZE)) {
			parse_sizes(parser, glob);
		} else if (is_end(parser)) {
			return;
		} else
			error_end(parser);
	}
}

static void parse_slide(struct parser *parser, struct key *key)
{
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_KEY)) {
			struct slide_key *skey = e_malloc(sizeof(struct slide_key));

			key->slides_cnt++;
			if (!key->u.slides)
				key->u.slides = skey;
			else {
				struct slide_key *last = key->u.slides;
				while (last->next)
					last = last->next;
				last->next = skey;
			}
			__get_raw(parser);
			set_str(&skey->name, parser->tstr);
		} else if (is_end(parser))
			return;
		else
			error_end(parser);
	}
}

static void parse_key(struct parser *parser, struct row *row, int type)
{
	struct key *key = e_malloc(sizeof(struct key));

	key->size = -1;
	row->keys_cnt++;
	if (!row->keys)
		row->keys = key;
	else {
		struct key *last = row->keys;
		while (last->next)
			last = last->next;
		last->next = key;
	}
	if (type == TOK_KEY) {
		__get_raw(parser);
		set_str(&key->u.name, parser->tstr);
	} else if (type == TOK_WHITE) {
		set_str(&key->u.name, "");
		key->flags |= KEY_WHITE;
	} else if (type == TOK_TAB) {
		set_str(&key->u.name, "");
		key->flags |= KEY_TAB;
	} else if (type == TOK_BACKSPACE) {
		set_str(&key->u.name, "");
		key->flags |= KEY_BACKSPACE;
	} else if (type == TOK_SHIFT) {
		set_str(&key->u.name, "");
		key->flags |= KEY_SHIFT;
	}
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_ALPHA))
			key->flags |= KEY_ALPHA;
		else if (is_keyword(parser, TOK_NUM))
			key->flags |= KEY_NUM;
		else if (is_keyword(parser, TOK_HEXA))
			key->flags |= KEY_HEXA;
		else if (is_keyword(parser, TOK_TELE))
			key->flags |= KEY_TELE;
		else if (is_keyword(parser, TOK_SPECIAL))
			key->flags |= KEY_SPECIAL;
		else if (is_keyword(parser, TOK_DEAD))
			key->flags |= KEY_DEAD;
		else if (is_keyword(parser, TOK_SIZE)) {
			get_tok_num(parser);
			key->size = parser->tint;
		} else if (is_begin(parser) && type == TOK_SLIDE)
			parse_slide(parser, key);
		else if (is_keyword(parser, TOK_KEY) ||
			   is_keyword(parser, TOK_WHITE) ||
			   is_keyword(parser, TOK_TAB) ||
			   is_keyword(parser, TOK_BACKSPACE) ||
			   is_keyword(parser, TOK_SHIFT) ||
			   is_keyword(parser, TOK_SLIDE) ||
			   is_end(parser)) {
			push_tok(parser);
			if (type == TOK_SLIDE && !key->slides_cnt)
				error(parser, "{ expected");
			return;
		} else
			error_end(parser);
	}
}

static void parse_row(struct parser *parser, struct layout *lay)
{
	struct row *row = e_malloc(sizeof(struct row));

	lay->rows_cnt++;
	if (!lay->rows)
		lay->rows = row;
	else {
		struct row *last = lay->rows;
		while (last->next)
			last = last->next;
		last->next = row;
	}
	get_tok_begin(parser);
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_KEY))
			parse_key(parser, row, TOK_KEY);
		else if (is_keyword(parser, TOK_WHITE))
			parse_key(parser, row, TOK_WHITE);
		else if (is_keyword(parser, TOK_TAB))
			parse_key(parser, row, TOK_TAB);
		else if (is_keyword(parser, TOK_BACKSPACE))
			parse_key(parser, row, TOK_BACKSPACE);
		else if (is_keyword(parser, TOK_SHIFT))
			parse_key(parser, row, TOK_SHIFT);
		else if (is_keyword(parser, TOK_SLIDE))
			parse_key(parser, row, TOK_SLIDE);
		else if (is_end(parser))
			return;
		else
			error_end(parser);
	}
}

static void parse_layout(struct parser *parser, struct kbd *kbd, int type)
{
	struct layout *lay = e_malloc(sizeof(struct layout));

	if (type == LAY_LOWERCASE_NUM || type == LAY_UPPERCASE_NUM) {
		/* numeric layout is a sublayout of a normal layout */
		struct layout *find = kbd->layouts;
		while (find) {
			if (find->type == type - 2) {
				find->sublayout = lay;
				break;
			}
			find = find->next;
		}
		if (!find) {
			free(lay);
			error(parser, "lowercase_num/uppercase_num has to follow lowercase/uppercase definition");
		}
	} else {
		struct layout *last = kbd->layouts;

		if (!last)
			kbd->layouts = lay;
		else {
			while (last->next)
				last = last->next;
			last->next = lay;
		}
		kbd->layouts_cnt++;
		if ((type == LAY_UPPERCASE && (!last || last->type != LAY_LOWERCASE)) ||
		    (type == LAY_SPECIAL_UPPER && (!last || last->type != LAY_SPECIAL_LOWER)))
			error(parser, "uppercase has to follow lowercase definition");
		if (type == LAY_SPECIAL_UPPER && last->name)
			set_str(&lay->name, last->name);
	}
	lay->type = type;
	get_tok_begin(parser);
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_NAME)) {
			get_tok_string(parser);
			if (lay->name)
				error(parser, "name must not be specified for uppercase layout in kbd_special section");
			set_str(&lay->name, parser->tstr);
		} else if (is_keyword(parser, TOK_MARGIN)) {
			int i;
			for (i = 0; i < 4; i++) {
				get_tok_num(parser);
				lay->margins[i] = parser->tint;
			}
		} else if (is_keyword(parser, TOK_DEFAULT_SIZE)) {
			get_tok_num(parser);
			lay->default_size = parser->tint;
		} else if (is_keyword(parser, TOK_ROW))
			parse_row(parser, lay);
		else if (is_end(parser))
			return;
		else
			error_end(parser);
	}
}

static void parse_kbd(struct parser *parser, struct global *glob, int type)
{
	struct kbd *kbd = e_malloc(sizeof(struct kbd));

	if (!glob->kbds)
		glob->kbds = kbd;
	else {
		struct kbd *last = glob->kbds;
		while (last->next)
			last = last->next;
		last->next = kbd;
	}
	glob->kbds_cnt++;
	kbd->type = type;
	get_tok_begin(parser);
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_LOWERCASE))
			parse_layout(parser, kbd, LAY_LOWERCASE);
		else if (is_keyword(parser, TOK_UPPERCASE))
			parse_layout(parser, kbd, LAY_UPPERCASE);
		else if (is_keyword(parser, TOK_SPEC_LOWERCASE)) {
			if (type != KBD_SPECIAL)
				error(parser, "special_lowercase allowed only in kbd_special section");
			parse_layout(parser, kbd, LAY_SPECIAL_LOWER);
		} else if (is_keyword(parser, TOK_SPEC_UPPERCASE)) {
			if (type != KBD_SPECIAL)
				error(parser, "special_uppercase allowed only in kbd_special section");
			parse_layout(parser, kbd, LAY_SPECIAL_UPPER);
		} else if (is_keyword(parser, TOK_LOWERCASE_NUM)) {
			if (type != KBD_NORMAL)
				error(parser, "lowercase_num allowed only in kbd_normal section");
			parse_layout(parser, kbd, LAY_LOWERCASE_NUM);
		} else if (is_keyword(parser, TOK_UPPERCASE_NUM)) {
			if (type != KBD_NORMAL)
				error(parser, "uppercase_num allowed only in kbd_normal section");
			parse_layout(parser, kbd, LAY_UPPERCASE_NUM);
		} else if (is_keyword(parser, TOK_SPECIAL)) {
			if (type != KBD_THUMB && type != KBD_SPECIAL)
				error(parser, "special allowed only in kbd_thumb and kbd_special sections");
			parse_layout(parser, kbd, LAY_SPECIAL);
		} else if (is_end(parser)) {
			if (!kbd->layouts_cnt)
				error(parser, "no keyboard layouts defined");
			return;
		} else
			error_end(parser);
	}
}

static void parse_global(struct parser *parser)
{
	struct global *glob = parser->global;

	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_HEADER))
			parse_header(parser, glob);
		else if (is_keyword(parser, TOK_KBD_NORMAL))
			parse_kbd(parser, glob, KBD_NORMAL);
		else if (is_keyword(parser, TOK_KBD_THUMB))
			parse_kbd(parser, glob, KBD_THUMB);
		else if (is_keyword(parser, TOK_KBD_SPECIAL))
			parse_kbd(parser, glob, KBD_SPECIAL);
		else if (is_eof(parser)) {
			if (!glob->kbds_cnt)
				error(parser, "no keyboards defined");
			return;
		} else
			error(parser, "header, kbd_normal, kbd_thumb or kbd_special expected");
	}
}

/*** writer ***/

struct writer {
	struct global *global;
	int start_table;
	int *starts;
};

static void werror(struct writer *writer)
{
	(void)writer;
	longjmp(cjmp, 1);
}

static void init_writer(struct writer *writer, struct global *global)
{
	writer->global = global;
	writer->starts = e_malloc(sizeof(int) * global->kbds_cnt);
}

static void close_writer(struct writer *writer)
{
	if (!writer)
		return;
	free(writer->starts);
}

static void writer_byte(struct writer *writer, unsigned char b)
{
	if (cops->write(cpriv, &b, 1))
		werror(writer);
}

static void writer_word(struct writer *writer, unsigned int w)
{
	unsigned char a[2];

	a[0] = w & 0xff;
	a[1] = (w >> 8) & 0xff;
	if (cops->write(cpriv, a, 2))
		werror(writer);
}

static void writer_string(struct writer *writer, char *s)
{
	int len;

	if (!s)
		len = 0;
	else
		len = strlen(s);
	writer_byte(writer, len);
	if (len) {
		if (cops->write(cpriv, s, len))
			werror(writer);
	}
}

static void writer_sizes(struct writer *writer, struct size *sizes)
{
	int i;

	while (sizes) {
		for (i = 0; i < 5; i++)
			writer_byte(writer, sizes->dim[i]);
		sizes = sizes->next;
	}
}

static void writer_keys(struct writer *writer, struct key *key, int default_size)
{
	struct slide_key *skey;

	while (key) {
		writer_byte(writer, key->slides_cnt ? 1 : 0);
		writer_byte(writer, key->flags & 0xff);
		if (key->flags & KEY_EXTEND)
			writer_byte(writer, (key->flags >> 8) & 0xff);
		if (!key->slides_cnt)
			writer_string(writer, key->u.name);
		else {
			writer_byte(writer, key->slides_cnt | 0x80);
			skey = key->u.slides;
			while (skey) {
				writer_string(writer, skey->name);
				skey = skey->next;
			}
		}
		writer_byte(writer, key->size >= 0 ? key->size : default_size);
		key = key->next;
	}
}

static void writer_sublayout(struct writer *writer, struct layout *lay)
{
	int cnt;
	struct row *tmp;

	cnt = 0;
	for (tmp = lay->rows; tmp; tmp = tmp->next)
		cnt += tmp->keys_cnt;
	writer_byte(writer, cnt);
	writer_byte(writer, lay->rows_cnt);
	writer_byte(writer, lay->margins[3]);
	writer_byte(writer, lay->margins[0]);
	writer_byte(writer, lay->margins[2]);
	writer_byte(writer, lay->margins[1]);
	for (tmp = lay->rows; tmp; tmp = tmp->next)
		writer_byte(writer, tmp->keys_cnt);
	for (tmp = lay->rows; tmp; tmp = tmp->next)
		writer_keys(writer, tmp->keys, lay->default_size);
}

static void writer_layouts(struct writer *writer, struct layout *lay, int idx)
{
	int type, other;

	switch (lay->type) {
	case LAY_LOWERCASE:
		type = 0;
		break;
	case LAY_UPPERCASE:
		type = 1;
		break;
	default:
		type = 2;
		break;
	}
	switch (lay->type) {
	case LAY_LOWERCASE:
	case LAY_SPECIAL_LOWER:
		other = idx + 1;
		break;
	case LAY_UPPERCASE:
	case LAY_SPECIAL_UPPER:
		other = idx - 1;
		break;
	default:
		other = 0xff;
		break;
	}
	writer_byte(writer, type);
	writer_byte(writer, other);
	writer_string(writer, lay->name);
	writer_byte(writer, lay->sublayout ? 2 : 1);
	writer_sublayout(writer, lay);
	if (lay->sublayout)
		writer_sublayout(writer, lay->sublayout);
}

static void writer_kbd(struct writer *writer, struct kbd *kbd)
{
	struct layout *lay;
	int i;

	writer_byte(writer, kbd->type);
	writer_byte(writer, kbd->layouts_cnt);
	writer_byte(writer, 0);
	writer_byte(writer, kbd->layouts->default_size);	/* use the default size of the first layout */
	for (lay = kbd->layouts, i = 0; lay; lay = lay->next, i++)
		writer_layouts(writer, lay, i);
}

static void writer_global(struct writer *writer)
{
	struct global *glob = writer->global;
	struct kbd *kbd;
	int i;

	writer_byte(writer, 1);		/* version */
	writer_byte(writer, glob->kbds_cnt);
	writer_string(writer, glob->name);
	writer_string(writer, glob->lang);
	writer_string(writer, glob->wc);
	writer_byte(writer, 2);		/* always use the default screen modes */
	writer_byte(writer, 0);
	writer_byte(writer, 1);
	writer_byte(writer, glob->sizes_cnt);
	writer_sizes(writer, glob->sizes);
	writer->start_table = cops->tell(cpriv);
	for (i = 0; i < glob->kbds_cnt; i++)
		writer_word(writer, 0);
	for (i = 0; i < 20; i++)
		writer_byte(writer, 0);
	for (kbd = glob->kbds, i = 0; kbd; kbd = kbd->next, i++) {
		writer->starts[i] = cops->tell(cpriv);
		writer_kbd(writer, kbd);
	}
	cops->seek(cpriv, writer->start_table);
	for (i = 0; i < glob->kbds_cnt; i++)
		writer_word(writer, writer->starts[i]);
}

/*** main ***/

int compile(struct compiler_ops *ops, void *priv)
{
	struct parser *parser = NULL;
	struct writer *writer = NULL;
	int res;

	cops = ops;
	cpriv = priv;

	res = setjmp(cjmp);
	if (!res) {
		parser = e_malloc(sizeof(struct parser));
		writer = e_malloc(sizeof(struct writer));
		init_parser(parser);
		parse_global(parser);
		init_writer(writer, parser->global);
		writer_global(writer);
		if (cops->return_lang)
			cops->return_lang(cpriv, parser->global->lang);
	}
	close_writer(writer);
	close_parser(parser);
	free(writer);
	free(parser);
	return res ? -1 : 0;
}
