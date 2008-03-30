/*
 *  Copyright (c) 2007 Jiri Benc <jbenc@upir.cz>
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
#include <string.h>

#define BUF_SIZE	8192

/*** tokenizer ***/

struct tokenizer {
	FILE *f;
	char *fname;
	int line;
	char *buf, *pos;
	char *pushed;
};

char keywords[][32] = {
	"header", "name", "lang", "wc", "size", "width", "height",
	"textpos", "left", "top", "kbd_normal", "kbd_thumb", "lowercase",
	"lowercase_num", "uppercase", "uppercase_num", "special1",
	"special2", "margin", "default_size", "row", "key", "slide",
	"white", "alpha", "num", "special", "hexa", "tele", "dead"
};
enum keywords_const {
	TOK_HEADER, TOK_NAME, TOK_LANG, TOK_WC, TOK_SIZE, TOK_WIDTH, TOK_HEIGHT,
	TOK_TEXTPOS, TOK_LEFT, TOK_TOP, TOK_KBD_NORMAL, TOK_KBD_THUMB, TOK_LOWERCASE,
	TOK_LOWERCASE_NUM, TOK_UPPERCASE, TOK_UPPERCASE_NUM, TOK_SPECIAL1,
	TOK_SPECIAL2, TOK_MARGIN, TOK_DEFAULT_SIZE, TOK_ROW, TOK_KEY, TOK_SLIDE,
	TOK_WHITE, TOK_ALPHA, TOK_NUM, TOK_SPECIAL, TOK_HEXA, TOK_TELE, TOK_DEAD
};

enum tok_type {
	TT_KEYWORD, TT_NUM, TT_STRING, TT_BEGIN, TT_END, TT_EOF
};

void tok_error(struct tokenizer *tokenizer, char *msg)
{
	fprintf(stderr, "%s:%d: Error: %s.\n", tokenizer->fname, tokenizer->line, msg);
	exit(1);
}

void tok_warning(struct tokenizer *tokenizer, char *msg)
{
	fprintf(stderr, "%s:%d: Warning: %s.\n", tokenizer->fname, tokenizer->line, msg);
}

void *e_malloc(size_t size)
{
	void *p = calloc(1, size);
	if (!p) {
		fprintf(stderr, "Out of memory.\n");
		exit(1);
	}
	return p;
}

char *newstrcpy(char *s)
{
	char *res = e_malloc(strlen(s) + 1);
	strcpy(res, s);
	return res;
}

char *ltrim(char *buf)
{
	while (buf && (*buf == ' ' || *buf == '\t' || *buf == '\n' || *buf == '\r'))
		buf++;
	return buf;
}

char *find_white(char *buf)
{
	while (buf && *buf && *buf != ' ' && *buf != '\t' && *buf != '\n' && *buf != '\r')
		buf++;
	return buf;
}

int read_raw(struct tokenizer *tokenizer, char *dest, size_t size, int no_ltrim)
{
	char *s;
	char tmp;

	while (1) {
		if (!no_ltrim)
			tokenizer->pos = ltrim(tokenizer->pos);
		if (!*tokenizer->pos) {
			tokenizer->line++;
			tokenizer->pos = tokenizer->buf;
			if (!fgets(tokenizer->buf, BUF_SIZE, tokenizer->f))
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

void finish_read_string(struct tokenizer *tokenizer, char *dest, size_t size)
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

int find_keyword(char *s)
{
	int i;
	int max = (sizeof(keywords) / sizeof(*keywords));

	for (i = 0; i < max; i++) {
		if (!strcmp(s, keywords[i]))
			return i;
	}
	return -1;
}

void unread_tok(struct tokenizer *tokenizer, char *tok)
{
	strncpy(tokenizer->pushed, tok, BUF_SIZE - 1);
	tokenizer->pushed[BUF_SIZE - 1] = '\0';
}

enum tok_type read_tok(struct tokenizer *tokenizer, char *dest, size_t size, int *parsed_int)
{
	int res;

	if (*tokenizer->pushed) {
		strncpy(dest, tokenizer->pushed, size - 1);
		dest[size - 1] = '\0';
		*tokenizer->pushed = '\0';
	} else {
		res = read_raw(tokenizer, dest, size, 0);
		if (res < 0)
			return TT_EOF;
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

void init_tokenizer(struct tokenizer *tokenizer, char *fname)
{
	tokenizer->fname = fname;
	tokenizer->f = fopen(fname, "r");
	if (!tokenizer->f)
		tok_error(tokenizer, "Cannot open input file");
	tokenizer->pos = tokenizer->buf = e_malloc(BUF_SIZE);
	tokenizer->pushed = e_malloc(BUF_SIZE);
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
};

struct kbd {
	int type;
	int layouts_cnt;
	struct layout *(layouts[4]);
};

#define LIDX_LOWERCASE		0
#define LIDX_UPPERCASE		1
#define LIDX_LOWERCASE_NUM	2
#define LIDX_UPPERCASE_NUM	3
#define LIDX_SPECIAL1		2
#define LIDX_SPECIAL2		3

struct size {
	int dim[5];
	struct size *next;
};

struct global {
	char *name;
	char *lang;
	char *wc;
	int sizes_cnt;
	struct size *sizes;
	int kbds_cnt;
	struct kbd *(kbds[2]);
};

#define KIDX_NORMAL		0
#define KIDX_THUMB		1

/*** parser ***/

struct parser {
	struct tokenizer *tokenizer;
	struct global *global;
	int ttype;
	char *tstr;
	int tint;
};

void init_parser(struct parser *parser, char *fname)
{
	parser->tokenizer = e_malloc(sizeof(struct tokenizer));
	init_tokenizer(parser->tokenizer, fname);
	parser->tstr = e_malloc(BUF_SIZE);
}

#define error(parser, msg)	tok_error((parser)->tokenizer, msg)
#define warning(parser, msg)	tok_warning((parser)->tokenizer, msg)
#define error_end(parser)	error(parser, "} expected")

void get_tok(struct parser *parser)
{
	parser->ttype = read_tok(parser->tokenizer, parser->tstr, BUF_SIZE, &parser->tint);
}

void __get_raw(struct parser *parser)
{
	if (read_raw(parser->tokenizer, parser->tstr, BUF_SIZE, 0) < 0)
		error(parser, "Unexpected end of file");
}

void push_tok(struct parser *parser)
{
	unread_tok(parser->tokenizer, parser->tstr);
}

#define is_type(parser, t)	((parser)->ttype == (t))
#define is_keyword(parser, w)	((parser)->ttype == TT_KEYWORD && (parser)->tint == (w))
#define is_begin(parser)	((parser)->ttype == TT_BEGIN)
#define is_end(parser)		((parser)->ttype == TT_END)
#define is_eof(parser)		((parser)->ttype == TT_EOF)

void get_tok_type(struct parser *parser, int tt)
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

void parse_sizes(struct parser *parser, struct global *glob)
{
	struct size *size = e_malloc(sizeof(struct size));

	get_tok_num(parser);
	if (parser->tint != glob->sizes_cnt)
		error(parser, "size number out of order");
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

void parse_header(struct parser *parser, struct global *glob)
{
	get_tok_begin(parser);
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_NAME)) {
			get_tok_string(parser);
			glob->name = newstrcpy(parser->tstr);
		} else if (is_keyword(parser, TOK_LANG)) {
			get_tok_string(parser);
			glob->lang = newstrcpy(parser->tstr);
		} else if (is_keyword(parser, TOK_WC)) {
			get_tok_string(parser);
			glob->wc = newstrcpy(parser->tstr);
		} else if (is_keyword(parser, TOK_SIZE)) {
			parse_sizes(parser, glob);
		} else if (is_end(parser)) {
			return;
		} else
			error_end(parser);
	}
}

void parse_slide(struct parser *parser, struct key *key)
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
			skey->name = newstrcpy(parser->tstr);
		} else if (is_end(parser))
			return;
		else
			error_end(parser);
	}
}

void parse_key(struct parser *parser, struct row *row, int type)
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
		key->u.name = newstrcpy(parser->tstr);
	} else if (type == TOK_WHITE) {
		key->u.name = newstrcpy("");
		key->flags |= KEY_WHITE;
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

void parse_row(struct parser *parser, struct layout *lay)
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
		else if (is_keyword(parser, TOK_SLIDE))
			parse_key(parser, row, TOK_SLIDE);
		else if (is_end(parser))
			return;
		else
			error_end(parser);
	}
}

void parse_layout(struct parser *parser, struct kbd *kbd, int idx)
{
	struct layout *lay = e_malloc(sizeof(struct layout));

	kbd->layouts[idx] = lay;
	lay->type = idx;
	get_tok_begin(parser);
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_NAME)) {
			get_tok_string(parser);
			lay->name = newstrcpy(parser->tstr);
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

void parse_kbd(struct parser *parser, struct global *glob, int idx)
{
	struct kbd *kbd = e_malloc(sizeof(struct kbd));

	glob->kbds[idx] = kbd;
	kbd->type = idx;
	get_tok_begin(parser);
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_LOWERCASE))
			parse_layout(parser, kbd, LIDX_LOWERCASE);
		else if (is_keyword(parser, TOK_LOWERCASE_NUM))
			parse_layout(parser, kbd, LIDX_LOWERCASE_NUM);
		else if (is_keyword(parser, TOK_UPPERCASE))
			parse_layout(parser, kbd, LIDX_UPPERCASE);
		else if (is_keyword(parser, TOK_UPPERCASE_NUM))
			parse_layout(parser, kbd, LIDX_UPPERCASE_NUM);
		else if (is_keyword(parser, TOK_SPECIAL1))
			parse_layout(parser, kbd, LIDX_SPECIAL1);
		else if (is_keyword(parser, TOK_SPECIAL2))
			parse_layout(parser, kbd, LIDX_SPECIAL2);
		else if (is_end(parser)) {
			int i;
			for (i = 0; i < 5; i++)
				if (kbd->layouts[i])
					kbd->layouts_cnt++;
			return;
		} else
			error_end(parser);
	}
}

void parse_global(struct parser *parser)
{
	struct global *glob = e_malloc(sizeof(struct global));

	parser->global = glob;
	while (1) {
		get_tok(parser);
		if (is_keyword(parser, TOK_HEADER))
			parse_header(parser, glob);
		else if (is_keyword(parser, TOK_KBD_NORMAL))
			parse_kbd(parser, glob, KIDX_NORMAL);
		else if (is_keyword(parser, TOK_KBD_THUMB))
			parse_kbd(parser, glob, KIDX_THUMB);
		else if (is_eof(parser)) {
			if (glob->kbds[0])
				glob->kbds_cnt++;
			if (glob->kbds[1])
				glob->kbds_cnt++;
			return;
		} else
			error(parser, "header, kbd_normal or kbd_thumb expected");
	}
}

/*** writer ***/

struct writer {
	FILE *f;
	char *fname;
	struct global *global;
	int start_table;
	int *starts;
};

void werror(struct writer *writer)
{
	fprintf(stderr, "Error writing %s.\n", writer->fname);
	exit(1);
}

void init_writer(struct writer *writer, char *fname, struct global *global)
{
	writer->fname = fname;
	writer->global = global;
	writer->starts = e_malloc(sizeof(int) * global->kbds_cnt);
	writer->f = fopen(fname, "wb");
	if (!writer->f)
		werror(writer);
}

void close_writer(struct writer *writer)
{
	fclose(writer->f);
}

void writer_byte(struct writer *writer, unsigned char b)
{
	if (fwrite(&b, 1, 1, writer->f) != 1)
		werror(writer);
}

void writer_word(struct writer *writer, unsigned int w)
{
	unsigned char a[2];

	a[0] = w & 0xff;
	a[1] = (w >> 8) & 0xff;
	if (fwrite(a, 1, 2, writer->f) != 2)
		werror(writer);
}

void writer_string(struct writer *writer, char *s)
{
	int len;
	
	if (!s)
		len = 0;
	else
		len = strlen(s);
	writer_byte(writer, len);
	if (len) {
		if (fwrite(s, 1, len, writer->f) != (size_t)len)
			werror(writer);
	}
}

void writer_sizes(struct writer *writer, struct size *sizes)
{
	int i;

	while (sizes) {
		for (i = 0; i < 5; i++)
			writer_byte(writer, sizes->dim[i]);
		sizes = sizes->next;
	}
}

void writer_keys(struct writer *writer, struct key *key, int default_size)
{
	struct slide_key *skey;

	while (key) {
		writer_byte(writer, key->slides_cnt ? 1 : 0);
		writer_byte(writer, key->flags);
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

void writer_sublayout(struct writer *writer, struct layout *lay)
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

void writer_layouts(struct writer *writer, struct layout *lay1, struct layout *lay2)
{
	int w;

	switch (lay1->type) {
	case LIDX_LOWERCASE:
		w = 0x0100;
		break;
	case LIDX_UPPERCASE:
		w = 0x0001;
		break;
	case LIDX_SPECIAL1:
	case LIDX_SPECIAL2:
	default:
		w = 0xff02;
		break;
	}
	writer_word(writer, w);
	writer_string(writer, lay1->name);
	writer_byte(writer, lay2 ? 2 : 1);
	writer_sublayout(writer, lay1);
	if (lay2)
		writer_sublayout(writer, lay2);
}

void writer_kbd(struct writer *writer, struct kbd *kbd)
{
	int i;

	writer_byte(writer, kbd->type == KIDX_NORMAL ? 0 : 4);	/* magic */
	if (kbd->type == KIDX_NORMAL) {
		i = 0;
		if (kbd->layouts[LIDX_LOWERCASE])
			i++;
		if (kbd->layouts[LIDX_UPPERCASE])
			i++;
	} else
		i = kbd->layouts_cnt;
	writer_byte(writer, i);
	writer_byte(writer, 0);					/* magic */
	writer_byte(writer, kbd->type == KIDX_NORMAL ? 0 : 3);	/* magic */
	for (i = 0; i < 4; i++) {
		if (kbd->type == KIDX_NORMAL &&
		    (i == LIDX_LOWERCASE_NUM || i == LIDX_UPPERCASE_NUM))
			continue;
		if (!kbd->layouts[i])
			continue;
		writer_layouts(writer, kbd->layouts[i],
			       (kbd->type == KIDX_NORMAL) ? kbd->layouts[i + 2] : NULL);
	}
}

void writer_global(struct writer *writer)
{
	struct global *glob = writer->global;
	int i, j;

	writer_byte(writer, 1);		/* version */
	writer_byte(writer, glob->kbds_cnt);
	writer_string(writer, glob->name);
	writer_string(writer, glob->lang);
	writer_string(writer, glob->wc);
	writer_byte(writer, 2);		/* magic */
	writer_byte(writer, 0);
	writer_byte(writer, 1);
	writer_byte(writer, glob->sizes_cnt);
	writer_sizes(writer, glob->sizes);
	writer->start_table = ftell(writer->f);
	for (i = 0; i < glob->kbds_cnt; i++)
		writer_word(writer, 0);
	for (i = 0; i < 20; i++)
		writer_byte(writer, 0);
	j = 0;
	for (i = 0; i < 2; i++) {
		if (!glob->kbds[i])
			continue;
		writer->starts[j] = ftell(writer->f);
		writer_kbd(writer, glob->kbds[i]);
		j++;
	}
	fseek(writer->f, writer->start_table, SEEK_SET);
	for (i = 0; i < glob->kbds_cnt; i++)
		writer_word(writer, writer->starts[i]);
}

/*** main ***/

int main(int argc, char **argv)
{
	struct parser *parser = e_malloc(sizeof(struct parser));
	struct writer *writer = e_malloc(sizeof(struct writer));

	if (argc < 3) {
		fprintf(stderr, "Missing parameters (source and destination filename).\n");
		exit(1);
	}
	init_parser(parser, argv[1]);
	parse_global(parser);
	init_writer(writer, argv[2], parser->global);
	writer_global(writer);
	close_writer(writer);
	return 0;
}
