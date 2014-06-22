/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <fcntl.h>

#include "wiki_index.h"
#include "my_strstr.h"
#include "wiki_parse.h"
#include "wiki_common.h"

#include "gzip_compress.h"

#define my_strncmp(x, y) strncmp(x, y, sizeof(y) - 1)
#define my_strncasecmp(x, y) strncasecmp(x, y, sizeof(y) - 1)

#define parse_not_use_title(title) \
	(my_strncasecmp(title, "Wikipedia:") == 0  || my_strncasecmp(title, "Help:") == 0 \
			|| my_strncasecmp(title, "MediaWiki:") == 0 || my_strncasecmp(title, "Portal:") == 0 \
			|| my_strncasecmp(title, "Template:") == 0 || my_strncasecmp(title, "File:") == 0 \
			|| my_strncasecmp(title, "Category:") == 0)


char *format_title(char *title)
{
	int len = 0;

	for (int i = 0; title[i]; i++) {
		if (title[i] == '&') {
			if (my_strncmp(title + i + 1, "amp;") == 0) {
				title[len++] = '&';
				i += 4;
				continue;
			} else if (my_strncmp(title + i + 1, "lt;") == 0) {
				title[len++] = '<';
				i += 3;
				continue;
			} else if (my_strncmp(title + i + 1, "gt;") == 0) {
				title[len++] = '>';
				i += 3;
				continue;
			}

		}
		title[len++] = title[i];
	}

	title[len] = 0;

	return title;
}

WikiParse::WikiParse()
{
	m_complete_flag = wiki_is_complete();

	m_wiki_math = new WikiMath();
	m_mem = new WikiMemory();

	m_mem->w_mem_init(3*1024*1024, 4);

	m_wiki_zh = new WikiZh();
	m_wiki_zh->wz_init();

	m_zh_flag = 0;

	m_image_fname = new ImageFname();
	m_image_fname->if_init();
}

WikiParse::~WikiParse()
{
}

int WikiParse::wp_ref_init(struct wiki_parse *p)
{
	p->ref_count = 0;
	p->ref_size = 0;

	p->chapter_size = 0;
	p->chapter_count = 0;

	memset(p->chapter_index, 0, sizeof(p->chapter_index));

	return 0;
}

int WikiParse::wp_get_title(char *p, char *title, char *redirect)
{
	char *w, *x, *r;

	title[1023] = redirect[1023] = 0;

	title[0] = redirect[0] = 0;

	r = my_strcasestr(p, "<redirect", 512*1024);

	if ((w = strstr(p, "<title>")) == NULL)
		return 0;

	if ((x = strstr(w + 7, "</title>")) == NULL)
		return 0;

	*x = 0;
	strncpy(title, w + 7, 512);

	*x = '<';

	if (r == NULL) {
		return 1;
	}

#if 1
	for (r += 9; *r == ' ' || *r == '\t' || *r == '\n'; r++);

	if ((w = strchr(r, '='))) {
		*w++ = 0;
		if ((x = strchr(w, '>'))) {
			*x = 0;
			for (x--; *x == ' ' || *x == '\t' || *x == '\n' || *x == '/'; x--);
			if ((*w == '"' && *x == '"') || (*w = '\'' && *x == '\'')) {
				*x = 0;
				w++;
			} else
				x[1] = 0;

			strncpy(redirect, w, 512);
		}
	}
#endif

	return 1;
}

#define IS_A2Z(c) (((c) >= 'a' && (c) <= 'z') || ((c) >= 'A' && (c) <= 'Z'))

int WikiParse::wp_is_lang_key(char *key)
{
	char *p;

	if ((p = strchr(key, ':')) == NULL)
		return 0;

	if (IS_A2Z(key[0]) && IS_A2Z(key[1]))
		return 1;

	return 0;
}

int WikiParse::wp_is_http(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	char ch, *w;

	for (int i = 0; i < max_size; i++) {
		if (p[i] == ']') {
			if (i > 16*1024)
				return 0;
			ch = p[i];
			p[i] = 0;
			if ((w = strchr(p, ' ')) == NULL) {
				p[i] = ch;
				return 0;
			}
			*w++ = 0;
			*out_size = sprintf(out, "<a href='%s'>%s</a>", p, w);
			*do_len = i;
			return 1;
		}
	}

	return 0;
}

/*
 * href="1.2.3#key1,key2,key3"
 */
int WikiParse::wp_is_link(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	split_t sp;
	char *t;

	*out_size = 0;

	for (int i = 0;  i < max_size; i++) {
		if (p[i] == ']' && p[i + 1] == ']') {
			if (i > 32*1024)
				return 0;
			if (i == 0 || (p[i + 2] == '\n' && wp_is_lang_key(p))) {
				*out_size = 0;
				if (i == 0)
					*do_len = i + 1;
				else
					*do_len = i + 2;
				return 1;
			}
			p[i] = 0;
			split('|', p, sp);

			int key_idx = -1, idx_len = 0, key_len = 0;
			wiki_mem_t idx, key;

			m_mem->w_mem_malloc(&idx);
			m_mem->w_mem_malloc(&key);

			idx_len = key_len = 0;
			int _flag = 0;
			for (int j = 0; j < split_total(sp); j++) {
				if ((t = strrchr(sp[j], '#'))) {
					if (t[1] == '.' && t[4] == '.')
						*t = 0;
				}
				if ((key_idx = m_find_key(sp[j], strlen(sp[j]))) >= 0) {
					idx_len += sprintf(idx.addr + idx_len, "%s%d", _flag == 0 ? "" : ",", key_idx);
					key_len += sprintf(key.addr + key_len, "%s%s", _flag == 0 ? "" : ",", sp[j]);
					_flag = 1;
				}
			}

			if (idx_len > 0)
				*out_size = sprintf(out, "<a href=\"%s#%s\">%s</a>", idx.addr, key.addr, sp[0]);
			else {
				memcpy(out, p, i);
				*out_size = i;
			}

			*do_len = i + 1;

			m_mem->w_mem_free(&key);
			m_mem->w_mem_free(&idx);

			return 1;
		}
	}

	return 0;
}

#define WP_DO_ITALIC(_PREFIX, _B1, _B2, _n) \
	do { \
		if (i <= 4) { \
			*out_size = sprintf(out, _PREFIX _B1 "%s" _B2, p + _n); \
		} else {  \
			m_mem->w_mem_malloc(&tmp); \
			wp_do_parse(p + _n, i - _n, tmp.addr, &size, 0, wp); \
			memcpy(out, _B1, sizeof(_B1) - 1); \
			pos += sizeof(_B1) - 1; \
\
			memcpy(out + pos, tmp.addr, size); \
			pos += size; \
\
			memcpy(out + pos, _B2, sizeof(_B2) - 1); \
			pos += sizeof(_B2) - 1; \
\
			*out_size = pos; \
			m_mem->w_mem_free(&tmp); \
		} \
	} while (0)


/*
 * TODO
 */
int WikiParse::wp_is_italic(char *p, int max_size, char *out, int *out_size, int *do_len,
				struct wiki_parse *wp)
{
	wiki_mem_t tmp;
	int size, pos = 0;
	int count = 0;

	for (int i = 0; i < max_size && i < 32*1024; i++) {
		if (p[i] == '\'' && p[i + 1] == '\'') {
			if (p[0] != '\'') {
				p[i] = 0;
				*do_len = i + 1;
				WP_DO_ITALIC("", "<i>", "</i>", 0);
				return 1;
			}
			if (p[i + 2] == '\'') {
				if (count % 2 == 0) {
					p[i] = 0;
					*do_len = i + 2;
					WP_DO_ITALIC("", "<b>", "</b>", 1);
					return 1;
				}
			}
			count++;
			i++;
		}
	}

	return 0;
}

int WikiParse::wp_is_source_code(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	int pos = 0;

	for (int i = 0; i < max_size; i++) {
		if (p[i] == '\n')
			break;
		if (my_strncasecmp(p + i, "&gt;") == 0) {
			for (int j = i + 4; j < max_size; j++) {
				if (my_strncasecmp(p + j, "&lt;/source&gt;") == 0) {
					*do_len = j + 15;
					*out_size = pos;
					return 1;
				}
				if (p[j] == ' ') {
					memcpy(out + pos, "&nbsp;", 6);
					pos += 6;
				} else if (p[j] == '\n') {
					memcpy(out + pos, "<br/>\n", 6);
					pos += 6;
				} else 
					out[pos++] = p[j];
			}
		}
	}

	return 0;
}

int WikiParse::wp_is_comment(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	for (int i = 0; i < max_size; i++) {
		if (0[p + i] == '-' && 1[p + i] == '-' && 2[p + i] == '&' && 3[p + i] == 'g' && 4[p + i] == 't') {
			*out_size = 0;
			*do_len = i + (p[i + 6] == '\n' ? 6 : 5);
			return 1;
		}
	}

	return 0;
}

int WikiParse::wp_is_ref(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	int i, size;

	for (i = 0; i < max_size; i++) {
		if (p[i] == '&' && p[i + 1] == 'g' && p[i + 2] == 't' && p[i + 3] == ';') {
			if (p[i - 1] == '/') {
				*out_size = 0;
				*do_len = i + 3;
				return 1;
			}
			for (int j = i + 4; j < max_size; j++) {
				if (0[p + j] == '&' && 1[p + j] == 'l' && 2[p + j] == 't' 
						&& 3[p + j] == ';' && 4[p + j] == '/' 
						&& 5[p + j] == 'r' && 6[p + j] == 'e' 
						&& 7[p + j] == 'f' && 8[p + j] == '&' 
						&& 9[p + j] == 'g' && 10[p + j] == 't' && 11[p + j] == ';') {
					if (j - i > 32*1024)
						return 0;
					p[j] = 0;
					*out_size = sprintf(out, "<sup id=ref%d><a href='#note%d'>[%d]</a></sup>", 
							wp->ref_count, wp->ref_count, wp->ref_count + 1);
					*do_len = j + 11;

					wiki_mem_t tmp;

					m_mem->w_mem_malloc(&tmp);

					wp_do_parse(p + i + 4, j - i - 4, tmp.addr, &size, 0, wp);

					wp->ref_size += sprintf(wp->ref_buf + wp->ref_size, "<li id=note%d><a href='#ref%d'>" 
								"%d</a> ", wp->ref_count, wp->ref_count, wp->ref_count + 1);

					memcpy(wp->ref_buf + wp->ref_size, tmp.addr, size);
					wp->ref_size += size;

					memcpy(wp->ref_buf + wp->ref_size, "</li>\n", 6);
					wp->ref_size += 6;
					wp->ref_count++;

					m_mem->w_mem_free(&tmp);

					return 1;
				}
			}
		}
	}

	return 0;
}

#define wp_replace_table_null_item() \
	do { \
		if (row_buf_2.size < 10) { \
			chomp(row_buf_2.addr); \
			row_buf_2.size = strlen(row_buf_2.addr); \
		} \
		if (row_buf_2.size == 0) { \
			row_buf_2.size = sprintf(row_buf_2.addr, "%s", "&nbsp;"); \
		} \
	} while (0)


#define MAX_TABLE_LINE 1024
#define MAX_TABLE_ROW  128

/*
 * sub = || or !!
 * th = "th" or "td"
 */
inline int WikiParse::wp_table_one_item(char *t, const char *sub, const char *th,
		char *out, int *out_size, struct wiki_parse *wp)
{
	char *X, *Y, *Z;

	int row_total;
	char one_row_buf[8192];
	char *row[MAX_TABLE_ROW];

	wiki_mem_t row_buf_1, row_buf_2;

	m_mem->w_mem_malloc(&row_buf_1);
	m_mem->w_mem_malloc(&row_buf_2);

	row_total = split_two(sub, t, one_row_buf, sizeof(one_row_buf), row, MAX_TABLE_ROW);

	for (int j = 0; j < row_total; j++) {
		X = row[j];
		for (; *X == ' ' || *X == '\t'; X++);
		if ((Z = strchr(X, '=')) && (Y = strchr(X, '|')) && Z < Y) {
			*Y++ = 0;
#if 0
			if (th[1] == 'h') {
				X = Y;
				goto th;
			}
#endif
			wp_do_parse(X, strlen(X), row_buf_1.addr, &row_buf_1.size, 0, wp);
			wp_do_parse(Y, strlen(Y), row_buf_2.addr, &row_buf_2.size, 0, wp);
			row_buf_1.addr[row_buf_1.size] = 0;
			row_buf_2.addr[row_buf_2.size] = 0;

			wp_replace_table_null_item();

			*out_size += sprintf(out + *out_size, "<%s %s>%s</%s>", th, row_buf_1.addr, row_buf_2.addr, th);
			continue;
		}
#if 1
th:
		wp_do_parse(X, strlen(X), row_buf_2.addr, &row_buf_2.size, 0, wp);
		row_buf_2.addr[row_buf_2.size] = 0;

		wp_replace_table_null_item();

		*out_size += sprintf(out + *out_size, "<%s>%s</%s>", th, row_buf_2.addr, th);
#endif
	}

	m_mem->w_mem_free(&row_buf_1);
	m_mem->w_mem_free(&row_buf_2);

	return 0;
}

int WikiParse::wp_do_table(char *p, int max_size, char *out, int *out_size,
			struct wiki_parse *wp, const char *newline)
{
	char *t;

	int line_total;
	char *line[MAX_TABLE_LINE];
	wiki_mem_t line_buf, row_buf;

	int start_flag = 0;
	char row_head[8192]; /* |+ */
	int row_head_len = 0;

	m_mem->w_mem_malloc(&line_buf);
	m_mem->w_mem_malloc(&row_buf);

	p[max_size] = 0;
	
	line_total = split('\n', p, line_buf.addr, line_buf.size, line, MAX_TABLE_LINE);

	*out_size = sprintf(out, "<table border=1 cellspacing=0.5><tr>");

	for (int i = 0; i < line_total; i++) {
		t = line[i];
		for (; *t == ' ' || *t == '\t'; t++);

		if (t[0] == '|' && t[1] == '+') {
			wp_do_parse(t + 2, strlen(t + 2), row_buf.addr, &row_buf.size, 0, wp);
			row_buf.addr[row_buf.size] = 0;
			row_head_len += snprintf(row_head + row_head_len,
						sizeof(row_head) - row_head_len, "%s<br/>%s", row_buf.addr, newline);
			continue;
		}

		if (t[0] == '!') {
			t += t[1] == '!' ? 2 : 1;
			wp_table_one_item(t, "!!||", "th", out, out_size, wp);
			start_flag = 1;
			continue;
		}

		if (t[0] == '|' && t[1] == '-') {
			if (start_flag != 0)
				*out_size += sprintf(out + *out_size, "</tr><tr>%s", newline);
			continue;
		}

		if (t[0] == '|') {
			t += t[1] == '|' ? 2 : 1;
			wp_table_one_item(t, "||", "td", out, out_size, wp);
			start_flag = 1;
		}
	}

	*out_size += sprintf(out + *out_size, "</tr></table>%s", newline);

	if (row_head_len > 0) {
		memcpy(out + *out_size, row_head, row_head_len);
		*out_size += row_head_len;
	}

	m_mem->w_mem_free(&line_buf);
	m_mem->w_mem_free(&row_buf);

	return 0;
}

int WikiParse::wp_is_table(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	for (int i = 0; i < max_size && i < 256*1024; i++) {
		if (p[i] == '|' && p[i + 1] == '}') {
			*do_len = i + 1;

			if (p[i - 1] == '\n')
				i--;

			p[i] = 0;
			wp_do_table(p, i, out, out_size, wp, "\n");

			return 1;
		}
	}

	return 0;
}

int WikiParse::wp_is_math(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	char *T, *start;
	int i = 0, size;
	char fname[1024];

	for (i = 0; i < max_size; i++) {
		if (p[i] != ' ' && p[i] != '\t' && p[i] != '\n')
			break;
	}

	if (strncmp(p + i, "&gt;", 4) == 0) {
		i += 4;
		if ((T = strstr(p + i, "&lt;/math&gt;"))) {
			if ((T - p) >= 8192)
				return 0;

			*T = 0;
			*do_len = T - p + 12;

			start = p + i;
			size = T - (p + i);

			math_format(start, &size, wp->math);

			wm_math2fname(start, size, fname);
			*out_size = sprintf(out, "<img src=\"%s\" alt=\"%s\"/>", fname, start);
			return 1;
		}
	}

	return 0;
}

int check_is_px(const char *p)
{
	const char *t = p;

	for (; (*t >= '0' && *t <= '9') || *t == ' '; t++);

	if (strcasecmp(t, "px") == 0)
		return 1;

	return 0;
}

int WikiParse::wp_do_file(int flag, char *p, int max_size, char *out, int *out_size, struct wiki_parse *wp)
{
	char fname[1024];
	int last = 0;
	wiki_mem_t tmp;

	m_mem->w_mem_malloc(&tmp);

	split_t sp;
	split('|', p, sp);

	m_image_fname->if_add_fname(sp[0]);

	strncpy(fname, sp[0], sizeof(fname) - 1);
	fname[sizeof(fname) - 1] = 0;

	format_image_name(fname, sizeof(fname));

	last = split_total(sp) - 1;

	if (last > 0) {
		if (check_is_px(sp[last])) {
			p = (char *)"";
			goto null;
		}

		if (flag > 0) {
			for (char *t = sp[last]; *t; t++) {
				if (*t == _SPLIT_TMP_CH) {
					*t = '|';
				}
			}
		}

		wp_do_parse(sp[last], strlen(sp[last]), tmp.addr, &tmp.size, 0, wp);
		tmp.addr[tmp.size] = 0;
		p = tmp.addr;
	} else
		p = (char *)"";
null:

	*out_size = sprintf(out, "<br/>\n<img src=\"I.%s\" alt=\"%s\"/><br/>\n%s<br/>\n",
			fname, sp[0], p);

	m_mem->w_mem_free(&tmp);

	return 0;
}

/*
 * File: ...
 * Image: ...
 */
int WikiParse::wp_is_file2(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	const char *prefix[32] = {
		".bmp", ".djvu", ".gif", ".jpeg", ".jpg",
		".jpgg", ".mid", ".oga", ".ogg", ".ogv",
		".pdf", ".png", ".svg", ".tif", ".tiff",
		NULL,
	};

	int len, flag = 0;
	char ch;

	for (int i = 0; i < max_size; i++) {
		if (p[i] == '|' || p[i] == '\n')
			break;
		for (int j = 0; prefix[j]; j++) {
			len = strlen(prefix[j]);
			if (strncasecmp(p + i, prefix[j], len) == 0) {
				flag = 1;
				break;
			}
		}
		if (flag) {
			ch = p[i + len];
			p[i + len] = 0;

			*do_len = i + len;

			wp_do_file(0, p, i + len, out, out_size, wp);

			p[i + len] = ch;

			return 1;
		}
	}

	return 0;
}

/*
 * [[File: .... ]]
 */
int WikiParse::wp_is_file(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	int pos[256];
	int pos_total = 0;

	for (int i = 0; i < max_size && i < 8*1024; i++) {
		if (p[i] == '[' && p[i + 1] == '[') {
			for (; i < max_size; i++) {
				if (p[i] == '|') {
					if (pos_total < (int)(sizeof(pos) / sizeof(int))) {
						p[i] = _SPLIT_TMP_CH;
						pos[pos_total++] = i;
					}
				}
				if (p[i] == ']' && p[i + 1] == ']') {
					i++;
					break;
				}
			}
			continue;
		}
		if (p[i] == ']' && p[i + 1] == ']') {
			p[i] = 0;
			*do_len = i + 1;

			wp_do_file(pos_total, p, i, out, out_size, wp);

			return 1;
		}
	}

	/*
	 * resume
	 */
	for (int i = 0; i < pos_total; i++) {
		if (p[pos[i]] == _SPLIT_TMP_CH)
			p[pos[i]] = '|';
	}

	return 0;
}

/*
 * TODO
 */
int WikiParse::wp_is_chatper(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	int i = 0, total = 0, new_line = 0;
	char index[256];
	int index_pos = 0;

	memset(index, 0, sizeof(index));

	wiki_mem_t buf;

	for (; p[i] == '='; i++, total++);

	for (; i < max_size; i++) {
		if (p[i] == '=' && p[i + 1] == '=') {
			for (int j = 0; j < total; j++) {
				if (p[i + 2 + j] != '=')
					return 0;
			}
			if (p[i + 2 + total] != '\n')
				return 0;

			m_mem->w_mem_malloc(&buf);

			for (int j = i + 2 + total + 1; p[j] == '\n'; new_line++, j++);

			p[i] = 0;

			for (int j = total + 1; j < MAX_CHAPTER_INDEX; j++) {
				wp->chapter_index[j] = 0;
			}
			wp->chapter_index[total]++;

			wp_do_parse(p + total, i - total, buf.addr, &buf.size, 0, wp);
			buf.addr[buf.size] = 0;

			p[i] = 0;
			*do_len = i + total + 2 + new_line;

			for (int j = 0; j < MAX_CHAPTER_INDEX && wp->chapter_index[j] > 0; j++) {
				index_pos += snprintf(index + index_pos, sizeof(index) - index_pos,
						"%d.", wp->chapter_index[j]);
			}

			*out_size = sprintf(out, "<b><a id=c%d href='#h%d'>%s</a> %s</b><br/><br/>\n",
					wp->chapter_count, wp->chapter_count, index, buf.addr);

			wp->chapter_size += sprintf(wp->chapter + wp->chapter_size,
					"<a id=h%d href='#c%d'>%s</a>%s<br/>\n", 
					wp->chapter_count, wp->chapter_count, index, buf.addr);
			wp->chapter_count++;

			m_mem->w_mem_free(&buf);

			return 1;
		}
	}

	return 0;
}

int WikiParse::wp_is_brackets(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	char ch;
	int left = 1, right = 0;

	for (int i = 0; i < max_size && i < 256*1024; i++) {
		if (p[i] == '{' && p[i + 1] == '{') {
			left++;
			continue;
		}
		if (p[i] == '}' && p[i + 1] == '}') {
			right++;
			if (right != left)
				continue;

			ch = p[i];
			p[i] = 0;

			/*
			 * NoteTA, Good, portal, reflist, Link, unref, DayIn, comm
			 */
			if (is_noteta(p) || is_good(p)
					|| is_portal(p)
					|| is_link(p) || is_reflist(p) || is_unref(p)
					|| is_dayin(p) || is_comm(p) || is_inforbox(p)) {
				*do_len = i + (p[i + 2] == '\n' ? 2 : 1);
				*out_size = 0;
				return 1;
			}
			p[i] = ch;
		}
	}

	return 0;
}

struct list_st {
	int count;
	char *str;
	int len;
};

#define MAX_LIST_TOTAL 4096

#define uol_start() \
	do { \
		memcpy(out + *out_size, uol, 5); \
		*out_size += 5; \
	} while (0)

#define uol_end() \
	do { \
		memcpy(out + *out_size, _uol, 6); \
		*out_size += 6; \
	} while (0)

#define li_start() \
	do { \
		memcpy(out + *out_size, "<li>", 4); \
		*out_size += 4; \
	} while (0)

#define li_end() \
	do { \
		memcpy(out + *out_size, "</li>\n", 6); \
		*out_size += 6; \
	} while (0)

/*
 * before:
 *   *  <li> </li>
 *   **
 *   ***
 *   #
 *   ##
 */
int WikiParse::wp_is_list(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	int i, pos = 0;
	char ch = *p;
	struct list_st *list, *l = NULL, *w;
	int list_total = 0;
	char *buf;
	int tmp_size;

	wiki_mem_t tmp;
	m_mem->w_mem_malloc(&tmp);

	list = (struct list_st *)tmp.addr;
	buf = tmp.addr + sizeof(struct list_st) * (MAX_LIST_TOTAL + 1);

	for (i = 0; i < max_size; i++) {
		if (i == 0 || (p[i] == ch && p[i - 1] == '\n')) {
			l = &list[list_total++];
			for (l->count = 0; p[i] == ch; i++, l->count++);
			pos = i;
			l->str = p + i;
#ifndef DEBUG
			l->len = 0;	
#endif
		}

		if (p[i] == '\n') {
			l->len = i - pos;
			if (l->len == 0)
				list_total--;
			if (p[i + 1] != ch || list_total >= MAX_LIST_TOTAL - 1)
				break;
		}
	}

	if (i >= max_size) {
		l->len = max_size - pos;
	}

	*out_size = 0;

	if (list_total <= 0 || (list_total == 1 && list[0].len == 0)) {
		*do_len = i;
		m_mem->w_mem_free(&tmp);
		return 1;
	}

	char uol[16] = "<ul>\n";
	char _uol[16] = "</ul>\n";

	if (ch == '#') {
		uol[1] = 'o';
		_uol[2] = 'o';
	}

	uol_start();

	int ul_count = 0;

	for (int j = 0; j < list_total; j++) {
		w = &list[j];
		l = &list[j - 1];
		if (j > 0) {
			if (w->count > l->count) {
				uol_start();
				ul_count++;
			} else if (w->count < l->count) {
				for (int k = 0; k < l->count - w->count; k++) {
					uol_end();
					ul_count--;
				}
			}
		}


		w->str[w->len] = 0;
		wp_do_parse(w->str, w->len, buf, &tmp_size, 0, wp);

		if (tmp_size > 0) {
			li_start();
			memcpy(out + *out_size, buf, tmp_size);
			*out_size += tmp_size;
			li_end();
		}
	}
	for (int k = 0; k < ul_count; k++) {
		uol_end();
	}

	uol_end();

	*do_len = i;
	m_mem->w_mem_free(&tmp);

	return 1;
}

int WikiParse::wp_is_dd(char *p, int max_size, char *out, int *out_size, int *do_len, struct wiki_parse *wp)
{
	int i, ch, pos, len, tmp_size;
	wiki_mem_t tmp;

	m_mem->w_mem_malloc(&tmp);
	
	*out_size = 0;

	memcpy(out, "<dl>\n", 5);
	*out_size += 5;

	for (i = 0; i < max_size; i++) {
		ch = p[i];
		if (ch != ':' && ch != ';') {
			i--;
			break;
		}
		pos = i + 1;
		for (i++; i < max_size; i++) {
			if (p[i] == '\n') {
				len = i - pos;
				if (len > 0) {
					wp_do_parse(p + pos, len, tmp.addr, &tmp_size, 0, wp);
					*out_size += sprintf(out + *out_size, "<d%s>\n", ch == ':' ? "d" : "t");

					memcpy(out + *out_size, tmp.addr, tmp_size);
					*out_size += tmp_size;

					*out_size += sprintf(out + *out_size, "</d%s>\n", ch == ':' ? "d" : "t");
				}
				break;
			}
		}
	}
	memcpy(out + *out_size, "</dl>\n", 6);
	*out_size += 6;

	*do_len = i;
	m_mem->w_mem_free(&tmp);

	return 1;
}

#define WP_CHECK_FLAG(_func, _p, _p_size, _I) \
	do { \
		if (_func(_p + _I, _p_size - _I, tmp.addr, &tmp_size, &do_len, wp)) { \
			if (tmp_size > 0) { \
				if (math_flag == 1) { \
					int _X; \
					for (_X = pos - 1; _X >= 0 &&  \
							(out[_X] == ':' || out[_X] == ' ' || out[_X] == '\t'); _X--); \
					if (out[_X] == '\n') \
						pos = _X + 1; \
					math_flag = 0; \
				} \
				memcpy(out + pos, tmp.addr, tmp_size); \
				pos += tmp_size; \
			} \
			i += _I + do_len; \
			if (i >= size) \
				goto out; \
			goto next; \
		} else {  \
			goto one_char; \
		} \
	} while (0)

int WikiParse::wp_do_parse(char *buf, int size, char *out, int *out_size, int max_out_size, struct wiki_parse *wp)
{
	int math_flag = 0, pos = 0, do_len;
	char *p = buf;
	int tmp_size;

	wiki_mem_t tmp;
	m_mem->w_mem_malloc(&tmp);

	*out_size = 0;

	for (int i = 0; i < size; i++) {
		if (p[i] == '\'' && p[i + 1] == '\'') {
			for (i++; p[i] == '\''; i++);
			i--;
			continue;
			//WP_CHECK_FLAG(wp_is_italic, p + i, size - i, 2);
		}

		if (p[i] == ';') {
			if (p[i + 1] != '&' && pos > 0 && out[pos - 1] == '\n') {
				WP_CHECK_FLAG(wp_is_dd, p + i, size - i, 0);
			}
		}

		if (p[i] == ':') {
			if (0[p + i + 1] == '{' && 1[p + i + 1] == '|') {
				/* p + i + 1 == {|    //} */
				WP_CHECK_FLAG(wp_is_table, p + i, size - i, 3);
			} else if (p[i + 1] != '&' && pos > 0 && out[pos - 1] == '\n') {
				WP_CHECK_FLAG(wp_is_dd, p + i, size - i, 0);
			}
		}

		if ((p[i] == '*' || p[i] == '#') && pos > 0 && out[pos - 1] == '\n') {
			WP_CHECK_FLAG(wp_is_list, p + i, size - i, 0);
		}


		if (p[i] == '{') {
			if (p[i + 1] == '|') {
				WP_CHECK_FLAG(wp_is_table, p + i, size - i, 2);
			}

			if (p[i + 1] == '{') {
				WP_CHECK_FLAG(wp_is_brackets, p + i, size - i, 2);
			}
		}

		if (p[i] == '[') {
			if (p[i + 1] == '[') {
				/* file */
				if (is_file_flag(p + i + 2) && p[i + 6] == ':') {
					WP_CHECK_FLAG(wp_is_file, p + i, size - i, 7);
				} else {
					WP_CHECK_FLAG(wp_is_link, p + i, size - i, 2);
				}
			} else {
				if (0[p + i + 1] == 'h' && 1[p + i + 1] == 't' 
						&& 2[p + i + 1] == 't' && 3[p + i + 1] == 'p') {
					/* p + i + 1 == http */
					WP_CHECK_FLAG(wp_is_http, p + i, size - i, 1);
				}
			}
		}

		if (p[i] == '&') {
			if (0[p + i + 1] == 'l' && 1[p + i + 1] == 't' && 2[p + i + 1] == ';') {
				/* p + i + 1 == lt; */
				if (0[p + i + 4] == '!' && 1[p + i + 4] == '-' && 2[p + i + 4] == '-') {
					/* p + i + 4 == !-- */
					WP_CHECK_FLAG(wp_is_comment, p + i, size - i, 7);
				} else if (0[p + i + 4] == 'r' && 1[p + i + 4] == 'e' && 2[p + i + 4] == 'f') {
					/* p + i + 4 == ref */
					WP_CHECK_FLAG(wp_is_ref, p + i, size - i, 7);
				} else if (0[p + i + 4] == 'm' && 1[p + i + 4] == 'a' 
						&& 2[p + i + 4] == 't' && 3[p + i + 4] == 'h') {
					/* p + i + 4 == math */
					math_flag = 1;
					WP_CHECK_FLAG(wp_is_math, p + i, size - i, 8);
				} else if (is_source_code(p + i + 4)) {
					WP_CHECK_FLAG(wp_is_source_code, p + i, size - i, 9);
				}
				out[pos++] = '<';
				i += 3;
				continue;
			} else if (0[p + i + 1] == 'g' && 1[p + i + 1] == 't' && 2[p + i + 1] == ';') {
				/* p + i + 1 == gt; */
				out[pos++] = '>';
				i += 3;
				continue;
			} else if (0[p + i + 1] == 'q' && 1[p + i + 1] == 'u' 
					&& 2[p + i + 1] == 'o' && 3[p + i + 1] == 't'
					&& 4[p + i + 1] == ';') {
				/* p + i + 1 == quot; */
				out[pos++] = '\'';
				i += 5;
				continue;
			} else if (0[p + i + 1] == 'a' && 1[p + i + 1] == 'm' 
					&& 2[p + i + 1] == 'p' && 3[p + i + 1] == ';') {
				/* p + i + 1 == amp; */
				out[pos++] = '&';
				i += 4;
				continue;
			}
		}

#if 1
		if (p[i] == '=' && p[i + 1] == '=' && ((pos > 0 && out[pos - 1] == '\n') || i == 0)) {
			WP_CHECK_FLAG(wp_is_chatper, p + i, size - i, 2);
		}
#endif

		if (is_file_flag(p + i) && p[i + 4] == ':') {
			WP_CHECK_FLAG(wp_is_file2, p + i, size - i, 5);
		}

		if (is_image_flag(p + i) && p[i + 5] == ':') {
			WP_CHECK_FLAG(wp_is_file2, p + i, size - i, 6);
		}

		if (p[i] == '\n') {
			memcpy(out + pos, "<br/>\n", 6);
			pos += 6;
			continue;
		}

one_char:
		out[pos++] = p[i];
next:
		;
	}
out:

	*out_size = pos;
	m_mem->w_mem_free(&tmp);

	return 0;
}

#define TEST_IS_END(_p, _buf) \
	do { \
		if (_p - _buf >= size) { \
			goto out; \
		} \
	} while (0)

int WikiParse::wp_do(char *buf, int size, struct wiki_parse *wp)
{
	char *w, *p = buf;
	char *start;
	char title[1024], redirect[1024];
	int parse_flag = wp->parse_flag;

	wp_ref_init(wp);

	for (; p - buf < size; p++) {
		w = NULL;
		if (is_page_start(p)) {
			p += 6;
			start = p;
			TEST_IS_END(p, buf);
			if ((w = strstr(p, "<text"))) {
				w += 5;
				TEST_IS_END(w, buf);
				for (; *w != '>'; w++) {
					TEST_IS_END(w, buf);
				}
				*w++ = 0;
				start = w;
			} else {
				start = p;
			}
			wp_get_title(p, title, redirect);
			format_title(title);

			if (w)
				w[-1] = '>';
			w = p;

			TEST_IS_END(p, buf);

			if (strlen(redirect) == 0) {
				if (my_strncasecmp(start, "#REDIRECT") == 0) {
					char *T, *X;
					for (T = start + 9; *T == ' ' || *T == '\n' || *T == '\t'; T++);
					if (T[0] == '[' && T[1] == '[') {
						T += 2;
						if ((X = strstr(T, "]]"))) {
							*X = 0;
							strcpy(redirect, T);
						}
					}
				}
			}

			for (; p - buf < size; p++) {
				if (is_page_end(p) || is_page_start(p)) {
					*--p = 0;
					if (m_complete_flag) {
						if (parse_flag == PARSE_ALL_FLAG) {
							int old_size = p - w;
							int n = m_compress_func(wp->zip, 3*1024*1024, w, old_size);
							m_add_page(0, wp->zip, n, old_size, title, strlen(title), redirect, strlen(redirect));
						} else if (parse_flag == PARSE_TITLE_FLAG) {
							m_add_page(0, NULL, -1, -1, title, strlen(title), NULL, -1);
						}
					} else {
						if (parse_flag == PARSE_ALL_FLAG) {
							wp_do_parse_sys(start, p - start, title, redirect, wp);
						} else if (parse_flag == PARSE_TITLE_FLAG) {
							if (!parse_not_use_title(title)) {
								m_add_page(0, NULL, -1, -1, title, strlen(title), NULL, -1);
							}
						}
					}
					break;
				}
			}
		}
	}
out:

	return 0;
}

int WikiParse::wp_do_parse_sys(char *start, int size, const char *title, const char *redirect,
		struct wiki_parse *wp)
{
	int page_size = 0, title_len = strlen(title), redirect_len = strlen(redirect);

	if (parse_not_use_title(title)) 
		return 0;

	if (redirect_len > 0) {
		m_add_page(0, NULL, 0, 0, title, title_len, redirect, redirect_len);
		wp_ref_init(wp);

		return 0;
	}

	for (int j = size - 1; j >= 0; j--) {
		if (my_strncmp(start + j, "</text>") == 0) {
			start[j] = '\n';
			start[j + 1] = '\n';
			start[j + 2] = 0;
			size = j + 2;
		}
	}

	start[size] = 0;
	
	if (size > 2*1024*1024)
		return 0;

#define _CHAPTER_SIZE (512*1024)

	int tmp_start = _CHAPTER_SIZE;

	wp_do_parse(start, size, wp->tmp + tmp_start, &page_size, 0, wp);

	int fix = 0;
	start = wp->tmp + tmp_start;

	for (;;) {
		if (strncmp(start, "<br/>", 5) == 0) {
			fix += 5;
			start += 5;
			if (start[0] == '\n') {
				fix++;
				start++;
			}
		} else {
			break;
		}
	}
	page_size -= fix;
	tmp_start += fix;


	if (wp->ref_size > 0) {
		page_size += sprintf(wp->tmp + tmp_start + page_size, "<ul>");
		memcpy(wp->tmp + tmp_start + page_size, wp->ref_buf, wp->ref_size);
		page_size += wp->ref_size;
		page_size += sprintf(wp->tmp + tmp_start + page_size, "</ul>");
	}
	if (wp->chapter_size > 0) {
		wp->chapter_size += sprintf(wp->chapter + wp->chapter_size, "%s", "<hr align=left width=35%>\n");

		memcpy(wp->tmp + tmp_start - wp->chapter_size, wp->chapter, wp->chapter_size);
		tmp_start -= wp->chapter_size;
		page_size += wp->chapter_size;
	}

	if (page_size > 0) {
		int zip_len, n;
		if (m_zh_flag == 1 &&
				(n = m_wiki_zh->wz_convert_2hans(wp->tmp + tmp_start, page_size, wp->tmp)) > 0) {
			zip_len = m_compress_func(wp->zip, 5*1024*1024, wp->tmp, n);
			page_size = n;
		} else {
			zip_len = m_compress_func(wp->zip, 5*1024*1024, wp->tmp + tmp_start, page_size);
		}

		m_add_page(0, wp->zip, zip_len, page_size, title, title_len, redirect, strlen(redirect));
	}
	wp_ref_init(wp);

	return 0;
}

int WikiParse::wp_init(add_page_func_t _add_page, find_key_func_t _find_key, int zh_flag, int z_compress_flag)
{
	m_add_page = _add_page;
	m_find_key = _find_key;

	m_zh_flag = zh_flag;

	if (z_compress_flag == FM_FLAG_GZIP)
		m_compress_func = gzip;
	else if (z_compress_flag == FM_FLAG_LZ4)
		m_compress_func = lz4_compress;

	return 0;
}
