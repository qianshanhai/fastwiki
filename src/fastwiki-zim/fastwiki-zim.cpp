/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <string.h>
#include <stdlib.h>

#include "q_util.h"
#include "crc32sum.h"

#include "wiki_math.h"
#include "wiki_image.h"
#include "wiki_common.h"
#include "wiki_zh.h"

#include "fastwiki-dict.h"
#include "zim.h"

#define MAX_CHAPTER_INDEX 128
#define BUF_START (1024*1024)

static char *m_buf = NULL;
static char *m_tmp = NULL;

static WikiMath * m_wiki_math = NULL;
static WikiImage *m_wiki_image = NULL;
static FastwikiDict *m_wiki_dict = NULL;
static Zim *m_zim = NULL;
static WikiZh *m_wiki_zh = NULL;

static char *m_chapter = NULL;
static int m_chapter_size = 0;
static int m_chapter_count = 0;
static int m_chapter_index[MAX_CHAPTER_INDEX + 1];

static int m_split_math = 1;
static int m_zh_flag = 0;
static int m_is_wiki_complete = wiki_is_complete();

static char m_lang[32];			/* -l */
static char m_date[32]; 		/* -d */
static char m_creator[64]; 		/* -c */
static char m_zim_file[256]; 	/* -f */
unsigned int m_max_total = -1; 	/* -t */

#define ZIM_CONVERT_IMG 0x1   /* -i */
#define ZIM_CONVERT_TEXT 0x2  /* -a */
#define ZIM_CONVERT_MATH 0x4  /* -m */
#define ZIM_CONVERT_ZH   0x8  /* -z , 转成简体中文 */

static unsigned int m_convert_flag = ZIM_CONVERT_IMG | ZIM_CONVERT_TEXT | ZIM_CONVERT_MATH;

#define D(x) printf(x); fflush(stdout)

#define CHECK_MATH_IS_NEED() \
	if (m_wiki_math == NULL) { \
		m_wiki_math = new WikiMath(); \
		m_wiki_math->wm_zim_init(m_lang, m_date); \
	}

#define CHECK_IMAGE_IS_NEED() \
	if (m_wiki_image == NULL) { \
		m_wiki_image = new WikiImage(); \
		m_wiki_image->we_init(m_lang, m_date); \
	}

#define CHECK_TEXT_IS_NEED() \
	if (m_wiki_dict == NULL) { \
		m_wiki_dict = new FastwikiDict(); \
		m_wiki_dict->dict_init(m_lang, m_date, "gzip"); \
	}

struct title_key {
	unsigned int crc32;
	unsigned int r_crc32;
};

struct title_value {
	unsigned char len;
	char r[3];
};

static SHash *m_title_hash = NULL;

static int init_title_hash()
{
	m_title_hash = new SHash();
	m_title_hash->sh_init(30*10000, sizeof(struct title_key), sizeof(struct title_value));

	return 0;
}

extern int mylog(const char *fmt, ...);

static int add_title_hash(const char *title)
{
	struct title_key key;
	struct title_value value;

	memset(&key, 0, sizeof(key));
	memset(&value, 0, sizeof(value));

	value.len = strlen(title);
	crc32sum(title, value.len, &key.crc32, &key.r_crc32);

	m_title_hash->sh_replace(&key, &value);

	mylog("title:%s\n", title);

	return 0;
}

static int find_title_hash(const char *title)
{
	int len;
	struct title_key key;
	struct title_value *f;

	memset(&key, 0, sizeof(key));

	len = strlen(title);
	crc32sum(title, len, &key.crc32, &key.r_crc32);

	if (m_title_hash->sh_find(&key, (void **)&f) == _SHASH_FOUND) {
		if (f->len == len)
			return 1;
	}

	return 0;
}

static int zim_html_name_format(char *name)
{
	int len;
	char *p;

	if ((p = strrchr(name, '/'))) {
		*p++ = 0;
		strcpy(name, p);
		len = strlen(name);
		if (len > 5 && strncasecmp(name + len - 5, ".html", 5) == 0) {
			name[len - 5] = 0;
		}
	}

	return 0;
}

static int zim_parse_image_fname(char *fname, const char *from)
{
	int flag = 0, flag2 = 0;
	const char *p = strrchr(from, '/');

	if (p == NULL)
		p = from;
	else
		p++;

	for (; *p; ) {
		if (*p == '.')
			flag2 = 1;
		if (!((*p >= 'a' && *p <= 'f') || (*p >= 'A' && *p <= 'F') || (*p >= '0' && *p <= '9'))) {
			if (flag2 == 0)
				flag = 1;
		}
		*fname++ = *p++;
	}
	*fname = 0;

	return m_split_math == 0 ? 1 : flag;
}

/* p == body */
#define ZIM_IS_BODY(p) ((0[p] == 'b' || 0[p] == 'B') && (1[p] == 'o' || 1[p] == 'O') \
		&& (2[p] == 'd' || 2[p] == 'D') && (3[p] == 'y' || 3[p] == 'Y')) 

/* p == /body */
#define ZIM_IS_BODY_END(p) ((0[p] == '/') && ZIM_IS_BODY(p + 1))

/* p == script */
#define ZIM_IS_SCRIPT(p) ((0[p] == 's' || 0[p] == 'S') && (1[p] == 'c' || 1[p] == 'C') \
		&& (2[p] == 'r' || 2[p] == 'R') && (3[p] == 'i' || 3[p] == 'I') \
		&& (4[p] == 'p' || 4[p] == 'P') && (5[p] == 't' || 5[p] == 'T')) 

/* p == /script */
#define ZIM_IS_SCRIPT_END(p) (0[p] == '/' && ZIM_IS_SCRIPT(p + 1))

/* p == span>&nbsp;</span> */
#define ZIM_IS_SPAN_SPACE(p) ((0[p] == 's' || 0[p] == 'S') && (1[p] == 'p' || 1[p] == 'P') && (2[p] == 'a' || 2[p] == 'A') \
		&& (3[p] == 'n' || 3[p] == 'N') && 4[p] == '>' && 5[p] == '&' && (6[p] == 'n' || 6[p] == 'N') \
		&& (7[p] == 'b' || 7[p] == 'B') && (8[p] == 's' || 8[p] == 'S') && (9[p] == 'p' || 9[p] == 'P') \
		&& 10[p] == ';' && 11[p] == '<' && 12[p] == '/' && (13[p] == 's' || 13[p] == 'S') \
		&& (14[p] == 'p' || 14[p] == 'P') && (15[p] == 'a' || 15[p] == 'A') && (16[p] == 'n' || 16[p] == 'N') \
		&& 17[p] == '>')

/* p == sup id=cite_ref */
#define ZIM_IS_REF_SUP(p) ((0[p] == 's' || 0[p] == 'S') && (1[p] == 'u' || 1[p] == 'U') && (2[p] == 'p' || 2[p] == 'P') \
		&& 3[p] == ' ' && (4[p] == 'i' || 4[p] == 'I') && (5[p] == 'd' || 5[p] == 'D') && 6[p] == '=' \
		&& (7[p] == 'c' || 7[p] == 'C') && (8[p] == 'i' || 8[p] == 'I') && (9[p] == 't' || 9[p] == 'T') \
		&& (10[p] == 'e' || 10[p] == 'E') && 11[p] == '_' && (12[p] == 'r' || 12[p] == 'R') \
		&& (13[p] == 'e' || 13[p] == 'E') && (14[p] == 'f' || 14[p] == 'F')) 

/* p == ol class=references> */
#define ZIM_IS_REF_LIST(p) ((0[p] == 'o' || 0[p] == 'O') && (1[p] == 'l' || 1[p] == 'L') && 2[p] == ' ' \
		&& (3[p] == 'c' || 3[p] == 'C') && (4[p] == 'l' || 4[p] == 'L') && (5[p] == 'a' || 5[p] == 'A') \
		&& (6[p] == 's' || 6[p] == 'S') && (7[p] == 's' || 7[p] == 'S') && 8[p] == '=' \
		&& (9[p] == 'r' || 9[p] == 'R') && (10[p] == 'e' || 10[p] == 'E') && (11[p] == 'f' || 11[p] == 'F') \
		&& (12[p] == 'e' || 12[p] == 'E') && (13[p] == 'r' || 13[p] == 'R') && (14[p] == 'e' || 14[p] == 'E') \
		&& (15[p] == 'n' || 15[p] == 'N') && (16[p] == 'c' || 16[p] == 'C') && (17[p] == 'e' || 17[p] == 'E') \
		&& (18[p] == 's' || 18[p] == 'S') && 19[p] == '>' )

/* -{H|zh-cn: ..} */
#define ZIM_IS_ZHCN(p) (0[p] == '-' && 1[p] == '{' && (2[p] == 'h' || 2[p] == 'H') && 3[p] == '|' \
		&& (4[p] == 'z' || 4[p] == 'Z') && (5[p] == 'h' || 5[p] == 'H') && 6[p] == '-' \
		&& (7[p] == 'c' || 7[p] == 'C') && (8[p] == 'n' || 8[p] == 'N') && 9[p] == ':')  


/* -{H|zh-hans: ... } */
#define ZIM_IS_ZHHANS(p) (0[p] == '-' && 1[p] == '{' && (2[p] == 'h' || 2[p] == 'H') \
		&& 3[p] == '|' && (4[p] == 'z' || 4[p] == 'Z') && (5[p] == 'h' || 5[p] == 'H') \
		&& 6[p] == '-' && (7[p] == 'h' || 7[p] == 'H') && (8[p] == 'a' || 8[p] == 'A') \
		&& (9[p] == 'n' || 9[p] == 'N') && (10[p] == 's' || 10[p] == 'S') && 11[p] == ':') 

/* -{H|zh-tw: ... } */
#define ZIM_IS_ZHTW(p) (0[p] == '-' && 1[p] == '{' && (2[p] == 'h' || 2[p] == 'H') && 3[p] == '|' \
		&& (4[p] == 'z' || 4[p] == 'Z') && (5[p] == 'h' || 5[p] == 'H') && 6[p] == '-' \
		&& (7[p] == 't' || 7[p] == 'T') && (8[p] == 'w' || 8[p] == 'W') && 9[p] == ':')  

static int zim_parse(char *page, int page_len, char *buf)
{
	char *p, *w, *w2, *title;
	int pos = 0, save, idx, tmp_len;
	char fname[1024], math[1024], tmp_buf[2048];

	int brace_total = 0;

	for (int i = 0; i < page_len; i++) {
		if (m_zh_flag == 1 && page[i] == '\n') {
			do {
				i++;
				if (!(ZIM_IS_ZHCN(page + i) || ZIM_IS_ZHHANS(page + i) || ZIM_IS_ZHTW(page + i))) {
					i--;
					break;
				}
				for (; page[i] != '\n' && i < page_len; i++);
			} while (i < page_len);

			if (i >= page_len)
				break;
		}
		
#if 1
		/* ignore many -{...}- */
		if (page[i] == '-' && page[i + 1] == '{') {
			save = i;
			brace_total = 0;
			for (; i < page_len; ) {
				for (i += 2; i < page_len; i++) {
					if (page[i] == '}' && page[i + 1] == '-') {
						i += 2;
						brace_total++;
						break;
					}
				}
				for (; page[i] == ' ' || page[i] == '\t'; i++);
				if (!(page[i] == '-' && page[i + 1] == '{')) {
					i--;
					break;
				}
			}
			if (brace_total > 5)
				continue;
			
			i = save;

			for (i += 2; !(page[i] == '}' && page[i + 1] == '-') && i - save < 2048; i++);

			if (i - save < 2048) {
				p = page + save + 2;
				page[i] = 0;
				if ((w = strchr(p, ';'))) {
					*w = 0;
					if ((w = strchr(p, '|')))
						p = w + 1;
					else if ((w = strchr(p, ':')))
						p = w + 1;
				}
				pos += sprintf(buf + pos, "%s", p);
				i += 1;
				continue;
			}
			i = save;
		}
#endif

		if (page[i] == '<') {
			if (ZIM_IS_BODY(page + i + 1)) {
				for (i += 4; page[i] != '>'; i++);
				pos = 0;
				continue;
			}

			if (ZIM_IS_BODY_END(page + i + 1)) {
				break;
			}

			if (ZIM_IS_SCRIPT(page + i + 1)) {
				for (i += 6; page[i] != '>'; i++);
				continue;
			}

			if (ZIM_IS_SCRIPT_END(page + i + 1)) {
				for (i += 6; page[i] != '>'; i++);
				continue;
			}

#if 1
			/* <h > ... </h> */
			if ((0[page + i + 1] == 'h' || 0[page + i + 1] == 'H')) {
				char ch[16], T[64];
				int total, l = 0;

				save = i;
				if (!(page[i + 2] >= '0' && page[i + 2] <= '9'))
					goto out;

				for (i += 2; page[i] != '>' && page[i] != ' '; i++) {
					ch[l++] = page[i];
				}
				ch[l] = 0;

				if ((total = atoi(ch)) == 1) {
					i = save;
					goto out;
				}

				sprintf(T, "</%c%d>", page[save + 1], total);
				i++;
				if ((p = strstr(page + i, T)) == NULL) {
					i = save;
					goto out;
				}
				*p = 0;

				total -= 2;

				for (int j = total + 1; j < MAX_CHAPTER_INDEX; j++) {
					m_chapter_index[j] = 0;
				}
				m_chapter_index[total]++;

				int index_pos = 0;
				char index[256];

				for (int j = 0; j < MAX_CHAPTER_INDEX && m_chapter_index[j] > 0; j++) {
					index_pos += sprintf(index + index_pos, "%d.", m_chapter_index[j]);
				}

				char *line = page + i;
				int line_len = p - line;

				if (line_len < 1024) {
					zim_parse(line, line_len, tmp_buf);
					line = tmp_buf;
				}

				if (strncmp(buf + pos - 4, "</p>", 4) != 0) {
					memcpy(buf + pos, "<br/>", 5);
					pos += 5;
				}

				pos += sprintf(buf + pos, "\n<b><a id=c%d href='#h%d'>%s</a> %s</b><br/>\n",
						m_chapter_count, m_chapter_count, index, line);

				m_chapter_size += sprintf(m_chapter + m_chapter_size,
						"<a id=h%d href='#c%d'>%s</a>%s<br/>\n",
						m_chapter_count, m_chapter_count, index, line);

				m_chapter_count++;
				i = p + strlen(T) - page - 1;
				continue;
			}
#endif

			/* <a href ..> */
			if ((page[i + 1] == 'a' || page[i + 1] == 'A') && page[i + 2] == ' ') {
				save = i;
				for (i++; page[i] != '>'; i++);
				i++;
				title = page + i;

				if ((p = strstr(title, "</a>"))) {
					title[-1] = 0;
					if ((w = strstr(page + save + 2, "#cite_"))) {
						if ((p = strstr(page + save + 2, "href=")) && p < w) {
							title[-1] = '>';
							*p = 0;
							pos += sprintf(buf + pos, "%shref=", page + save);
							if (p[5] == '\'' || p[5] == '"')
								buf[pos++] = p[5];
							i = w - page - 1;
							continue;
						}
					}

					*p = 0;
					tmp_len = p - title;

					if (p - title < (int)sizeof(tmp_buf)) {
						tmp_len = zim_parse(title, p - title, tmp_buf);
						title = tmp_buf;
					}

					if ((idx = m_wiki_dict->dict_find_title(title, tmp_len)) >= 0) {
						pos += sprintf(buf + pos, "<a href=\"%d#%s\">%s</a>", idx, title, title);
					} else {
						if ((w = strchr(title, ':')) && 
								(strncmp(w - 4, "http", 4) == 0 || strncmp(w - 5, "https", 5) == 0)) {
							w2 = w;
							*w2++ = 0;
							for (; *w2 == '/'; w2++);
							w -= w[-1] == 's' ? 5 : 4;
							pos += sprintf(buf + pos, "<a href='%s://%s'>%s</a>", w, w2, w2);
						} else {
							pos += sprintf(buf + pos, "%s", title);
						}
					}
					i = p + 3 - page;
					continue;
				} else {
					i = save;
				}
			}

			/* == img */
			if ((0[page + i + 1] == 'i' || 0[page + i + 1] == 'I') 
							&& (1[page + i + 1] == 'm' || 1[page + i + 1] == 'M') 
							&& (2[page + i + 1] == 'g' || 2[page + i + 1] == 'G')) { 
				if ((p = strstr(page + i + 4, "src="))) {
					int flag;
					p += 4;
					if (p[0] == '\'' || p[0] == '\"') {
						if ((w = strchr(p + 1, p[0])) == NULL)
							goto out;

						*w = 0;
						memcpy(buf + pos, page + i, p - (page + i));
						pos += p - (page + i);

						flag = zim_parse_image_fname(fname, p + 1);

						if ((int)m_max_total > 0)
							add_title_hash(fname);

						if (flag == 0) {
							wm_math2fname(fname, strlen(fname), math);
							pos += sprintf(buf + pos, "\"%s\" ", math);
						} else {
							format_image_name(fname, sizeof(fname));
							pos += sprintf(buf + pos, "\"I.%s\" ", fname);
						}
						i = w - page;
					} else {
						for (; *p == ' '; p++);
						char *w1 = NULL;
						int t = 0;
						if ((w = strchr(p, ' ')) == NULL)
							goto out;
						if ((w1 = strchr(p, '>'))) {
							if (w1 - w < 0) {
								w = w1;
								t = 1;
							}
						}

						*w = 0;

						memcpy(buf + pos, page + i, p - (page + i));
						pos += p - (page + i);

						flag = zim_parse_image_fname(fname, p);

						if ((int)m_max_total > 0)
							add_title_hash(fname);

						if (flag == 0) {
							wm_math2fname(fname, strlen(fname), math);
							pos += sprintf(buf + pos, "\"%s\"%s", math, t ? ">" : " ");
						} else {
							format_image_name(fname, sizeof(fname));
							pos += sprintf(buf + pos, "\"I.%s\"%s", fname, t ? ">" : " ");
						}
						i = w - page;
					}
					continue;
				}
			}
			if (ZIM_IS_SPAN_SPACE(page + i + 1)) {
				buf[pos++] = ' ';
				i += 18;
				continue;
			}

		} else if (page[i] == '&') {
			/* == lt;  create by case.pl */
			if ((0[page + i + 1] == 'l' || 0[page + i + 1] == 'L') 
							&& (1[page + i + 1] == 't' || 1[page + i + 1] == 'T') 
							&& (2[page + i + 1] == ';' || 2[page + i + 1] == ';')) { 
				buf[pos++] = '<';
				i += 3;
				continue;
			}
			/* == gt;  create by case.pl */
			if ((0[page + i + 1] == 'g' || 0[page + i + 1] == 'G') 
							&& (1[page + i + 1] == 't' || 1[page + i + 1] == 'T') 
							&& (2[page + i + 1] == ';' || 2[page + i + 1] == ';')) { 
				buf[pos++] = '>';
				i += 3;
				continue;
			}
		}
out:
		buf[pos++] = page[i];
	}

	buf[pos] = 0;

	return pos;
}

int fz_init_title()
{
	struct zim_tmp_title st;
	char title[1024], redirect[1024];

	m_zim->zim_title_reset();

	while (m_zim->zim_title_read(&st, title, redirect) == 1) {
		if (st.name_space == 'A') {
			zim_html_name_format(title);
			m_wiki_dict->dict_add_title(title, strlen(title));
		}
	}

	m_wiki_dict->dict_add_title_done();

	return 0;
}

int fz_add_page(struct zim_tmp_title *st, char *title, char *redirect, char *data, int size)
{
	int n, tmp_start, len;
	char *page;

	if (zim_is_redirect(st)) {
		m_wiki_dict->dict_add_page(NULL, 0, title, strlen(title), redirect, strlen(redirect));
	} else {
		m_chapter_size = 0;
		m_chapter_count = 0;
		memset(m_chapter_index, 0, sizeof(m_chapter_index));
		tmp_start = BUF_START;

		if (m_is_wiki_complete) {
			memcpy(m_buf + BUF_START, data, size);
			len = size;
		} else {
			len = zim_parse(data, size, m_buf + BUF_START);
		}

		if (m_chapter_size > 0) {
			m_chapter_size += sprintf(m_chapter + m_chapter_size, "%s", "<hr align=left width=35%>\n");

			memcpy(m_buf + tmp_start - m_chapter_size, m_chapter, m_chapter_size);
			tmp_start -= m_chapter_size;
			len += m_chapter_size;
		}
		page = m_buf + tmp_start;

		if (m_convert_flag & ZIM_CONVERT_ZH) {
			if ((n = m_wiki_zh->wz_convert_2hans(m_buf + tmp_start, len, m_tmp)) > 0) {
				page = m_tmp;
				len = n;
			}
		}

		m_wiki_dict->dict_add_page(page, len, title, strlen(title), redirect, strlen(redirect));
	}

	return 0;
}

int fz_init_var()
{
	memset(m_lang, 0, sizeof(m_lang));
	memset(m_date, 0, sizeof(m_date));
	memset(m_creator, 0, sizeof(m_creator));
	memset(m_zim_file, 0, sizeof(m_zim_file));

	m_buf = (char *)malloc(10*1024*1024);
	m_tmp = (char *)malloc(10*1024*1024);
	m_chapter = (char *)malloc(1*1024*1024);

	m_zim = new Zim();

	m_wiki_zh = new WikiZh();
	m_wiki_zh->wz_init();

	return 0;
}

#ifndef WIN32
int fz_test(const char *file)
{
	mapfile_t mt;

	q_mmap(file, &mt);
	memcpy(m_chapter, mt.start, mt.size);

	m_wiki_dict->dict_add_title("1", 1);
	m_wiki_dict->dict_add_title_done();

	zim_parse(m_chapter, (int)mt.size, m_buf);
	printf("%s\n", m_buf);
	exit(0);
}
#endif

void usage(const char *name)
{
	print_usage_head();

	printf("usage: fastwiki-zim <-l lang> <-d date> <-f zim_file> [-t max_total] [-a] [-i] [-m] [-z]\n");
	printf( "       -l   language string. 'en', 'en_simple', etc.\n"
			"       -d   date\n"
			"       -f   zim file\n"
			"       -t   max total, usage for test/debug \n"
			"       -a   If assign, output all articles page data\n"
			"            If doubt, don't assign -a \n"
			"       -i   If assign, output all images\n"
			"            If doubt, don't assign -i \n"
			"       -m   If assign, output all mathematical formulas images\n"
			"            If doubt, don't assign -m \n"
			"       -z   convert all Traditional Chinese to Simplified Chinese\n"
			"            If doubt, don't assign -z\n"
			"Note: \n"
			"     default output all data, include all images and all articles, \n"
			"     equivalent -a -i -m, but, if assign -a, only output articles data. \n"
			"          if assign -a and -i, only output images and articles data. \n"
			"          etc. \n"
			"example: \n"
			"     fastwiki-zim -l en -d 201402 -f wikipedia_en_all_02_2014.zim \n"
			"     fastwiki-zim -l en_simple -d 201402 -f wikipedia_en_simple_12_2013.zim \n"
			"\n"
			"For debug: \n"
			"     fastwiki-zim -l en -d 201402 -f wikipedia_en_all_02_2014.zim -t 100000 \n"
			"     * just output first 100000 page, include images\n"

			".\n"

		  );

	print_usage_tail();
}

int fz_init_option(int argc, char **argv)
{
	unsigned int convert_flag = 0;
	int opt;

	while ((opt = getopt(argc, argv, "zimal:d:c:f:t:")) != -1) {
		switch (opt) {
			case 'i':
				convert_flag |= ZIM_CONVERT_IMG;
				break;
			case 'm':
				convert_flag |= ZIM_CONVERT_MATH;
				break;
			case 'a':
				convert_flag |= ZIM_CONVERT_TEXT;
				break;
			case 'z':
				convert_flag |= ZIM_CONVERT_ZH;
				break;
			case 'l':
				strncpy(m_lang, optarg, sizeof(m_lang) - 1);
				if (strncasecmp(m_lang, "zh", 2) == 0)
					m_zh_flag = 1;
				break;
			case 'd':
				strncpy(m_date, optarg, sizeof(m_date) - 1);
				break;
			case 'c':
				strncpy(m_creator, optarg, sizeof(m_creator) - 1);
				break;
			case 'f':
				strncpy(m_zim_file, optarg, sizeof(m_zim_file) - 1);
				break;
			case 't':
				m_max_total = atoi(optarg);
				init_title_hash();
				break;

			default: /* '?' */
				usage(argv[0]);
				break;
		}
	}
	if (convert_flag != 0) {
		if (convert_flag == ZIM_CONVERT_ZH)
			m_convert_flag |= ZIM_CONVERT_ZH;
		else
			m_convert_flag = convert_flag;
	}

	if (m_lang[0] == 0 || m_date[0] == 0 || m_zim_file[0] == 0)
		usage(argv[0]);

	if (!dashf(m_zim_file)) {
		printf("error %s: %s\n", m_zim_file, strerror(errno));
		exit(0);
	}

	return 0;
}


int main(int argc, char *argv[])
{
	int size, ret, flag;
	char *data;
	struct zim_tmp_title st;
	char title[1024], redirect[1024], fname[1024];

	unsigned int curr_total = 0;

	fz_init_var();
	fz_init_option(argc, argv);

	if (m_zim->zim_init(m_zim_file) == -1) {
		printf("read zim file error:%s\n", m_zim_file);
		return 0;
	}

	flag = (m_convert_flag & ZIM_CONVERT_TEXT) ? 0 : 1;

	if (flag == 0) {
		CHECK_TEXT_IS_NEED();
		fz_init_title();
	}

	m_zim->zim_data_reset();

	int total_flag = 0;

	for (;;) {
		if (curr_total++ > m_max_total) {
			total_flag = 1;
		}
		if ((ret = m_zim->zim_data_read(&st, title, redirect, &data, &size, flag)) == 0)
			break;
		if (ret == -1 || ret == 2)
			continue;

		zim_html_name_format(title);
		zim_html_name_format(redirect);

		if (total_flag == 0 && st.name_space == 'A' && (m_convert_flag & ZIM_CONVERT_TEXT)) {
			CHECK_TEXT_IS_NEED();
			fz_add_page(&st, title, redirect, data, size);
			continue;
		}

		if (st.name_space == 'I') {
			int tmp = zim_parse_image_fname(fname, title);
			if ((int)m_max_total > 0) {
				if (!find_title_hash(fname))
					continue;
			}

			if (tmp == 0) {
				if (m_convert_flag & ZIM_CONVERT_MATH) {
					CHECK_MATH_IS_NEED();
					m_wiki_math->wm_zim_add_math(fname, data, size);
				}
			} else {
				if (m_convert_flag & ZIM_CONVERT_IMG) {
					CHECK_IMAGE_IS_NEED();
					format_image_name(fname, sizeof(fname));
					m_wiki_image->we_add_one_image(fname, data, size);
				}
			}
		}
	}

	if (m_wiki_image) {
		m_wiki_image->we_output(NULL);
		delete m_wiki_image;
	}

	if (m_wiki_math) {
		m_wiki_math->wm_zim_output();
		delete m_wiki_math;
	}

	if (m_wiki_dict) {
		m_wiki_dict->dict_output();
		delete m_wiki_dict;
	}

	m_zim->zim_done();

	delete m_zim;

	printf("done.\n");

	return 0;
}
