/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <time.h>
#include <string.h>
#include <stdio.h>

#define __WIKI_LOCAL_CPP

#include "wiki_local.h"

static char _lang[128] = {0};

extern "C" int LOG(const char *fmt, ...);

void set_lang(const char *l)
{
	strcpy(_lang, l);
}

char *get_lang()
{
	return _lang;
}

static const char *_local_msg(const char *what, const char *name)
{
	int i; 

	for (i = 0; all_lang[i].name; i++) {
		if (strcmp(what, all_lang[i].name) == 0) {
			for (int j = 0; all_lang[i].l[j].flag; j++) {
				struct local *l = &all_lang[i].l[j];
				if (strcmp(l->flag, name) == 0)
					return l->s;
			}
			break;
		}
	}

	return "";
}

const char *local_msg(const char *name)
{
	const char *get;
	
	get = _local_msg(_lang, name);

	if (get[0] == 0) {
		return _local_msg("en", name);
	}

	return get;
}

#define MONTH_DAY_FUNC_ARGV char *key, int tm_mon, int tm_mday
#define MONTH_DAY_FUNC(x) GET_MONTH_DAY_##x
#define _GET_MONTH_DAY(x) {#x, MONTH_DAY_FUNC(x)}
#define GET_MONTH_DAY(x) MONTH_DAY_FUNC(x) (MONTH_DAY_FUNC_ARGV)

static char *GET_MONTH_DAY(sv)
{
	const char *month[] = {
		"januari", 	"februari", 	"mars", 	"april",
		"maj", 	"juni", 	"juli", 	"augusti",
		"september", 	"oktober", 	"november", 	"december",
	};

	sprintf(key, "%d %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(su)
{
	const char *month[] = {
		"Januari", 	"P" "\xc3\xa9" "bruari", 	"Maret", 	"April",
		"M" "\xc3\xa9" "i", 	"Juni", 	"Juli", 	"Agustus",
		"S" "\xc3\xa9" "pt" "\xc3\xa9" "mber", 	"Oktober",
		"Nop" "\xc3\xa9" "mber", 	"D" "\xc3\xa9" "s" "\xc3\xa9" "mber",
	};

	sprintf(key, "%d %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(sw)
{
	const char *month[] = {
		"Januari", 	"Februari", 	"Machi", 	"Aprili",
		"Mei", 	"Juni", 	"Julai", 	"Agosti",
		"Septemba", 	"Oktoba", 	"Novemba", 	"Desemba",
	};

	sprintf(key, "%d %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(fr)
{
	const char *month[] = {
		"janvier", 	"f" "\xc3\xa9" "vrier", 	"mars", 	"avril",
		"mai", 	"juin", 	"juillet", 	"ao" "\xc3\xbb" "t",
		"septembre", 	"octobre", 	"novembre", 	"d" "\xc3\xa9" "cembre",
	};

	sprintf(key, "%d %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(pt)
{
	const char *month[] = {
		"de janeiro", 	"de fevereiro", 	"de mar" "\xc3\xa7" "o", 	"de abril",
		"de maio", 	"de junho", 	"de julho", 	"de agosto",
		"de setembro", 	"de outubro", 	"de novembro", 	"de dezembro",
	};

	sprintf(key, "%d %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(fi)
{
	const char *month[] = {
		"tammikuuta", 	"helmikuuta", 	"maaliskuuta", 	"huhtikuuta",
		"toukokuuta", 	"kes" "\xc3\xa4" "kuuta", 	"hein" "\xc3\xa4" "kuuta", 	"elokuuta",
		"syyskuuta", 	"lokakuuta", 	"marraskuuta", 	"joulukuuta",
	};

	sprintf(key, "%d. %s", tm_mday, month[tm_mon]);

	return key;
}

/* TODO */
static char *GET_MONTH_DAY(fa)
{
	const char *month[] = {
		"\xda\x98\xd8\xa7\xd9\x86\xd9\x88\xdb\x8c\xd9\x87", 	
		"\xd9\x81\xd9\x88\xd8\xb1\xdb\x8c\xd9\x87", 	
		"\xd9\x85\xd8\xa7\xd8\xb1\xd8\xb3", 	
		"\xd8\xa2\xd9\x88\xd8\xb1\xdb\x8c\xd9\x84", 	
		"\xd9\x85\xd9\x87", 	
		"\xda\x98\xd9\x88\xd8\xa6\xd9\x86", 	
		"\xda\x98\xd9\x88\xd8\xa6\xdb\x8c\xd9\x87", 	
		"\xd8\xa7\xd9\x88\xd8\xaa", 	
		"\xd8\xb3\xd9\xbe\xd8\xaa\xd8\xa7\xd9\x85\xd8\xa8\xd8\xb1", 	
		"\xd8\xa7\xda\xa9\xd8\xaa\xd8\xa8\xd8\xb1", 	
		"\xd9\x86\xd9\x88\xd8\xa7\xd9\x85\xd8\xa8\xd8\xb1", 	
		"\xd8\xaf\xd8\xb3\xd8\xa7\xd9\x85\xd8\xa8\xd8\xb1", 	
	};

	const char *day[] = {
		"\xdb\xb1", 	
		"\xdb\xb2", 	
		"\xdb\xb3", 	
		"\xdb\xb4", 	
		"\xdb\xb5", 	
		"\xdb\xb6", 	
		"\xdb\xb7", 	
		"\xdb\xb8", 	
		"\xdb\xb9", 	
		"\xdb\xb1\xdb\xb0", 	
		"\xdb\xb1\xdb\xb1", 	
		"\xdb\xb1\xdb\xb2", 	
		"\xdb\xb1\xdb\xb3", 	
		"\xdb\xb1\xdb\xb4", 	
		"\xdb\xb1\xdb\xb5", 	
		"\xdb\xb1\xdb\xb6", 	
		"\xdb\xb1\xdb\xb7", 	
		"\xdb\xb1\xdb\xb8", 	
		"\xdb\xb1\xdb\xb9", 	
		"\xdb\xb2\xdb\xb0", 	
		"\xdb\xb2\xdb\xb1", 	
		"\xdb\xb2\xdb\xb2", 	
		"\xdb\xb2\xdb\xb3", 	
		"\xdb\xb2\xdb\xb4", 	
		"\xdb\xb2\xdb\xb5", 	
		"\xdb\xb2\xdb\xb6", 	
		"\xdb\xb2\xdb\xb7", 	
		"\xdb\xb2\xdb\xb8", 	
		"\xdb\xb2\xdb\xb9", 	
		"\xdb\xb3\xdb\xb0", 	
		"\xdb\xb3\xdb\xb1", 	
	};

	sprintf(key, "%s %s", day[tm_mday - 1], month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(hu)
{
	const char *month[] = {
		"Janu" "\xc3\xa1" "r", 	"Febru" "\xc3\xa1" "r",
		"M" "\xc3\xa1" "rcius", 	"\xc3\x81" "prilis",
		"M" "\xc3\xa1" "jus", 	"J" "\xc3\xba" "nius", 	"J" "\xc3\xba" "lius", 	"Augusztus",
		"Szeptember", 	"Okt" "\xc3\xb3" "ber", 	"November", 	"December",
	};

	sprintf(key, "%s %d.", month[tm_mon], tm_mday);

	return key;
}

static char *GET_MONTH_DAY(cs)
{
	const char *month[] = {
		"leden", 	"\xc3\xba" "nor", 	"b" "\xc5\x99" "ezen", 	"duben",
		"kv" "\xc4\x9b" "ten", 	"\xc4\x8d" "erven", 	"\xc4\x8d" "ervenec", 	"srpen",
		"z" "\xc3\xa1\xc5\x99\xc3\xad", 	"\xc5\x99\xc3\xad" "jen", 	"listopad", 	"prosinec",
	};

	sprintf(key, "%d. %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(no)
{
	const char *month[] = {
		"januar", 	"februar", 	"mars", 	"april",
		"mai", 	"juni", 	"juli", 	"august",
		"september", 	"oktober", 	"november", 	"desember",
	};

	sprintf(key, "%d. %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(nl)
{
	const char *month[] = {
		"januari", "februari", "maart", "april",
		"mei", "juni", "juli", "augustus",
		"september", "oktober", "november", "december",
	};

	sprintf(key, "%d %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(ms)
{
	const char *month[] = {
		"Januari", "Februari", "Mac", "April",
		"Mei", "Jun", "Julai", "Ogos",
		"September", "Oktober", "November", "Disember",
	};

	sprintf(key, "%d %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(es)
{
	const char *month[] = {
		"enero", "febrero", "marzo", "abril",
		"mayo", "junio", "julio", "agosto",
		"septiembre", "octubre", "noviembre", "diciembre",
	};

	sprintf(key, "%d de %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(it)
{
	const char *month[] = {
		"gennaio", "febbraio", "marzo", "aprile",
		"maggio", "giugno", "luglio", "agosto",
		"settembre", "ottobre", "novembre", "dicembre",
	};

	sprintf(key, "%d%s %s", tm_mday, (tm_mday == 1 ? "\xc2\xba" : ""), month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(de)
{
	const char *month[] = {
		"Januar", "Februar", "M\xc3\xa4rz", "April",
		"Mai", "Juni", "Juli", "August",
		"September", "Oktober", "November", "Dezember",
	};

	sprintf(key, "%d. %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(pl)
{
	const char *month[] = {
		"stycznia", 	"lutego", 	"marca", 	"kwietnia",
		"maja", 	"czerwca", 	"lipca", 	"sierpnia",
		"wrze" "\xc5\x9b" "nia", 	"pa" "\xc5\xba" "dziernika", 	"listopada", 	"grudnia",
	};

	sprintf(key, "%d. %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(ca)
{
	const char *month[] = {
		"de gener", 	"de febrer", 	"de mar" "\xc3\xa7", 	"d'abril",
		"de maig", 	"de juny", 	"de juliol", 	"d'agost",
		"de setembre", 	"d'octubre", 	"de novembre", 	"de desembre",
	};

	sprintf(key, "%s %d", month[tm_mon], tm_mday);

	return key;
}

static char *GET_MONTH_DAY(en)
{
	const char *month[] = {
		"January", "February", "March", "April",
		"May", "June", "July", "August",
		"September", "October", "November", "December",
	};

	sprintf(key, "%s %d", month[tm_mon], tm_mday);

	return key;
}

static char *GET_MONTH_DAY(uk)
{
	const char *month[] = {
		"\xd1\x81\xd1\x96\xd1\x87\xd0\xbd\xd1\x8f",
		"\xd0\xbb\xd1\x8e\xd1\x82\xd0\xbe\xd0\xb3\xd0\xbe",
		"\xd0\xb1\xd0\xb5\xd1\x80\xd0\xb5\xd0\xb7\xd0\xbd\xd1\x8f",
		"\xd0\xba\xd0\xb2\xd1\x96\xd1\x82\xd0\xbd\xd1\x8f",
		"\xd1\x82\xd1\x80\xd0\xb0\xd0\xb2\xd0\xbd\xd1\x8f",
		"\xd1\x87\xd0\xb5\xd1\x80\xd0\xb2\xd0\xbd\xd1\x8f",
		"\xd0\xbb\xd0\xb8\xd0\xbf\xd0\xbd\xd1\x8f",
		"\xd1\x81\xd0\xb5\xd1\x80\xd0\xbf\xd0\xbd\xd1\x8f",
		"\xd0\xb2\xd0\xb5\xd1\x80\xd0\xb5\xd1\x81\xd0\xbd\xd1\x8f",
		"\xd0\xb6\xd0\xbe\xd0\xb2\xd1\x82\xd0\xbd\xd1\x8f",
		"\xd0\xbb\xd0\xb8\xd1\x81\xd1\x82\xd0\xbe\xd0\xbf\xd0\xb0\xd0\xb4\xd0\xb0",
		"\xd0\xb3\xd1\x80\xd1\x83\xd0\xb4\xd0\xbd\xd1\x8f",
	};

	sprintf(key, "%d %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(ru)
{
	const char *month[] = {
		"\xd1\x8f\xd0\xbd\xd0\xb2\xd0\xb0\xd1\x80\xd1\x8f",
		"\xd1\x84\xd0\xb5\xd0\xb2\xd1\x80\xd0\xb0\xd0\xbb\xd1\x8f",
		"\xd0\xbc\xd0\xb0\xd1\x80\xd1\x82\xd0\xb0",
		"\xd0\xb0\xd0\xbf\xd1\x80\xd0\xb5\xd0\xbb\xd1\x8f",
		"\xd0\xbc\xd0\xb0\xd1\x8f",
		"\xd0\xb8\xd1\x8e\xd0\xbd\xd1\x8f",
		"\xd0\xb8\xd1\x8e\xd0\xbb\xd1\x8f",
		"\xd0\xb0\xd0\xb2\xd0\xb3\xd1\x83\xd1\x81\xd1\x82\xd0\xb0",
		"\xd1\x81\xd0\xb5\xd0\xbd\xd1\x82\xd1\x8f\xd0\xb1\xd1\x80\xd1\x8f",
		"\xd0\xbe\xd0\xba\xd1\x82\xd1\x8f\xd0\xb1\xd1\x80\xd1\x8f",
		"\xd0\xbd\xd0\xbe\xd1\x8f\xd0\xb1\xd1\x80\xd1\x8f",
		"\xd0\xb4\xd0\xb5\xd0\xba\xd0\xb0\xd0\xb1\xd1\x80\xd1\x8f",
	};

	sprintf(key, "%d %s", tm_mday, month[tm_mon]);

	return key;
}

static char *GET_MONTH_DAY(ko)
{
	char month[] = "\xec\x9b\x94";
	char day[] = "\xec\x9d\xbc";

	sprintf(key, "%d%s %d%s", tm_mon + 1, month, tm_mday, day);

	return key;
}

static char *GET_MONTH_DAY(vi)
{
	char month[] = "th" "\xc3\xa1" "ng";

	sprintf(key, "%d %s %d", tm_mday, month, tm_mon + 1);

	return key;
}

static char *GET_MONTH_DAY(zh)
{
	char month[] = "\xe6\x9c\x88";
	char day[] = "\xe6\x97\xa5";

	sprintf(key, "%d%s%d%s", tm_mon + 1, month, tm_mday, day);

	return key;
}

static char *GET_MONTH_DAY(ja)
{
	return MONTH_DAY_FUNC(zh)(key, tm_mon, tm_mday);
}

struct __month_day {
	const char *l;
	char *(*func)(char *key, int tm_mon, int tm_mday);
} all_month_day[] = {
	_GET_MONTH_DAY(en),
	_GET_MONTH_DAY(de), /* µÂÓï */
	_GET_MONTH_DAY(fr), /* ·¨Óï */
	_GET_MONTH_DAY(nl), /* ºÉÀ¼Óï */
	_GET_MONTH_DAY(it), /* Òâ´óÀûÓï */

	_GET_MONTH_DAY(pl), /* ²¨À¼Óï */
	_GET_MONTH_DAY(es), /* Î÷°àÑÀÓï	*/
	_GET_MONTH_DAY(ru), /* ¶íÓï  test done */
	_GET_MONTH_DAY(ja),
	_GET_MONTH_DAY(pt), /* ÆÏÌÑÑÀÓï */

	_GET_MONTH_DAY(zh),
	_GET_MONTH_DAY(sv), /* ÈðµäÓï */
	_GET_MONTH_DAY(vi), /* Ô½ÄÏÓï */
	_GET_MONTH_DAY(uk), /* ÎÚ¿ËÀ¼Óï */
	_GET_MONTH_DAY(ca), /* ¼ÓÌ©ÂÞÄáÑÇÓï */

	_GET_MONTH_DAY(no), /* Å²ÍþÓï */
	_GET_MONTH_DAY(fi), /* ·ÒÀ¼Óï */
	_GET_MONTH_DAY(cs), /* ½Ý¿ËÓï */
	_GET_MONTH_DAY(hu), /* ÐÙÑÀÀûÓï	*/
	_GET_MONTH_DAY(fa), /* ²¨Ë¹Óï*/

	_GET_MONTH_DAY(ko), /* º«Óï */

	_GET_MONTH_DAY(ms), /* ÂíÀ´Î÷À´ */

	_GET_MONTH_DAY(su),
	_GET_MONTH_DAY(sw),

	{NULL, NULL}
};

char *get_month_day(char *key)
{
	time_t now;
	struct tm *tm;

	time(&now);
	tm = localtime(&now);

	key[0] = 0;

	for (int i = 0; all_month_day[i].l; i++) {
		if (strcasecmp(all_month_day[i].l, _lang) == 0)
			return all_month_day[i].func(key, tm->tm_mon, tm->tm_mday);
	}

	return key;
}
