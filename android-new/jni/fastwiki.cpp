/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <dirent.h>
#include <jni.h>

#define MAX_KEY_LEN 64

#include "wiki_local.h"
#include "wiki_common.h"
#include "wiki_manage.h"

#define my_get_string(x) ({ jboolean __f = false; env->GetStringUTFChars(x, &__f); })

#ifdef __cplusplus
extern "C" {
#endif

static WikiManage *m_wiki_manage = NULL;
static WikiConfig *m_wiki_config = NULL;

jstring
Java_com_hace_fastwiki_FastWikiAbout_WikiAbout(JNIEnv* env, jobject thiz)
{
	return env->NewStringUTF(m_wiki_manage->wiki_about());
}

jstring
Java_com_hace_fastwiki_FastWiki_N(JNIEnv* env, jobject thiz, jstring flag)
{
	const char *tmp = my_get_string(flag);

	return env->NewStringUTF(local_msg(tmp));
}

void
Java_com_hace_fastwiki_FastWiki_LOG(JNIEnv* env, jobject thiz, jstring arg)
{
	const char *tmp = my_get_string(arg);

	LOG("%s", tmp);

	env->ReleaseStringUTFChars(arg, tmp);
}

void
Java_com_hace_fastwiki_History_LOG(JNIEnv* env, jobject thiz, jstring arg)
{
	Java_com_hace_fastwiki_FastWiki_LOG(env, thiz, arg);
}

jint
Java_com_hace_fastwiki_Library_SetSelectLang(JNIEnv* env, jobject thiz, jintArray idx)
{
	jint* arr;
	jint length;

	arr = env->GetIntArrayElements(idx, NULL);
	length = env->GetArrayLength(idx);

	return m_wiki_manage->wiki_set_select_lang(arr, length);
}

jint
Java_com_hace_fastwiki_Library_ScanSDcard(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_scan_sdcard();
}

jstring
Java_com_hace_fastwiki_FastWiki_WikiRandom(JNIEnv* env, jobject thiz)
{
	int size;
	char *buf;

	m_wiki_manage->wiki_random_page(&buf, &size);

	return env->NewStringUTF(buf);
}

jstring
Java_com_hace_fastwiki_FastWiki_WikiViewIndex(JNIEnv* env, jobject thiz, int idx)
{
	int size = 0;
	char *buf;

	m_wiki_manage->wiki_view_index(idx, &buf, &size);
	buf[size] = 0;

	return env->NewStringUTF(buf);
}

jstring
Java_com_hace_fastwiki_FastWiki_IndexMsg(JNIEnv* env, jobject thiz)
{
	int size;
	char *buf;

	m_wiki_manage->wiki_index_msg(&buf, &size);

	return env->NewStringUTF(buf);
}

jint
Java_com_hace_fastwiki_FastWiki_WikiReInit(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_reinit() == -1 ? 0 : 1;
}

extern "C" int LOG(const char *fmt, ...);

jint
Java_com_hace_fastwiki_FastWiki_WikiInit(JNIEnv* env, jobject thiz)
{
	int n;

	m_wiki_manage = new WikiManage();

	n = m_wiki_manage->wiki_init() == -1 ? 0 : 1;
	m_wiki_config = m_wiki_manage->wiki_get_config();

	return n;
}

jint
Java_com_hace_fastwiki_FastWiki_WikiExit(JNIEnv* env, jobject thiz)
{
	m_wiki_manage->wiki_exit();

	return 0;
}

jobjectArray
Java_com_hace_fastwiki_FastWiki_WikiCurrLang(JNIEnv* env, jobject thiz)
{
	int total;
	char lang[MAX_SELECT_LANG_TOTAL][24];

	jobjectArray args;
	jstring str;

	m_wiki_manage->wiki_select_lang(lang, &total);

	args = env->NewObjectArray(total, env->FindClass("java/lang/String"),0);

	for (int i = 0; i < total; i++) {
		str = env->NewStringUTF(lang[i]);
		env->SetObjectArrayElement(args, i, str);
	}

	return args;
}

jobjectArray
Java_com_hace_fastwiki_Library_WikiCurrLang(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiCurrLang(env, thiz);
}

jstring
Java_com_hace_fastwiki_FastWiki_WikiCurr(JNIEnv* env, jobject thiz)
{
	int size;
	char *buf;

	if (m_wiki_manage->wiki_curr(&buf, &size) == -1)
		return env->NewStringUTF("");

	return env->NewStringUTF(buf);
}

/*
 * [status, data]
 * 1=CONTENT, data
 * 2=MATCH,   title
 */
jobjectArray
Java_com_hace_fastwiki_FastWiki_WikiBack(JNIEnv* env, jobject thiz)
{
	int n, size;
	char *buf, status[16];
	jobjectArray args;
	jstring str;
	char tmp[2][4] = {"", "1"};

	memset(status, 0, sizeof(status));
	args = env->NewObjectArray(2, env->FindClass("java/lang/String"), 0);

	if ((n = m_wiki_manage->wiki_back(status, &buf, &size)) == -1)
		buf = tmp[0];
	else if (n == 0)
		buf = tmp[1];

	env->SetObjectArrayElement(args, 0, env->NewStringUTF(status));
	env->SetObjectArrayElement(args, 1, env->NewStringUTF(buf));

	return args;
}

jobjectArray
Java_com_hace_fastwiki_FastWiki_WikiForward(JNIEnv* env, jobject thiz)
{
	int n, size;
	char *buf, status[16];
	jobjectArray args;
	jstring str;
	char tmp[2][4] = {"", "1"};

	memset(status, 0, sizeof(status));
	args = env->NewObjectArray(2, env->FindClass("java/lang/String"), 0);

	if ((n = m_wiki_manage->wiki_forward(status, &buf, &size)) == -1)
		buf = tmp[0];
	else if (n == 0)
		buf = tmp[1];

	env->SetObjectArrayElement(args, 0, env->NewStringUTF(status));
	env->SetObjectArrayElement(args, 1, env->NewStringUTF(buf));

	return args;
}

jobjectArray
Java_com_hace_fastwiki_FastWiki_WikiMatch(JNIEnv* env, jobject thiz, jstring _start)
{
	int total;
	wiki_title_t *buf;
	jobjectArray args;
	jstring str;

	const char *start;
	
	start = my_get_string(_start);
	m_wiki_manage->wiki_match(start, &buf, &total);

	args = env->NewObjectArray(total, env->FindClass("java/lang/String"),0);

	for (int i = 0; i < total; i++) {
		str = env->NewStringUTF(buf[i].title);
		env->SetObjectArrayElement(args, i, str);
	}

	return args;
}

int
Java_com_hace_fastwiki_FastWiki_WikiMatchTotal(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_match_total();
}

jobjectArray
Java_com_hace_fastwiki_FastWiki_WikiMatchLang(JNIEnv* env, jobject thiz)
{
	int total, flag;
	wiki_title_t *buf;

	jobjectArray args;
	jstring str;

	m_wiki_manage->wiki_match_lang(&buf, &total, &flag);

	args = env->NewObjectArray(total, env->FindClass("java/lang/String"),0);

	for (int i = 0; i < total; i++) {
		if (flag > 1) {
			str = env->NewStringUTF(buf[i].which->lang);
		} else {
			str = env->NewStringUTF("");
		}

		env->SetObjectArrayElement(args, i, str);
	}

	return args;
}

int
Java_com_hace_fastwiki_FastWiki_WikiGetFontSize(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_fontsize();
}

/*
 * n = -1 or 1
 */
int
Java_com_hace_fastwiki_FastWiki_WikiSetFontSize(JNIEnv* env, jobject thiz, jint n)
{
	return m_wiki_config->wc_set_fontsize(n);
}

jstring
Java_com_hace_fastwiki_FastWiki_WikiBaseUrl(JNIEnv* env, jobject thiz)
{
	char buf[256];

	m_wiki_manage->wiki_base_url(buf);

	return env->NewStringUTF(buf);
}

void
Java_com_hace_fastwiki_FastWiki_WikiSetTitlePos(JNIEnv* env, jobject thiz,
		jint pos, jint height, jint screen_height)
{
	if (pos >= 0) {
		m_wiki_manage->wiki_add_key_pos(pos, height, screen_height);
	}
}

/*
 * TODO not use
 */
int
Java_com_hace_fastwiki_FastWiki_WikiLastPos(JNIEnv* env, jobject thiz)
{
	int pos, height;

	return m_wiki_manage->wiki_find_key_pos(&pos, &height);
}

/*
 * TODO not use
 */
int
Java_com_hace_fastwiki_FastWiki_WikiLastHeight(JNIEnv* env, jobject thiz)
{
	int pos, height;

	m_wiki_manage->wiki_find_key_pos(&pos, &height);

	return height;
}

jobjectArray
Java_com_hace_fastwiki_FastWiki_WikiLangList(JNIEnv* env, jobject thiz)
{
	jobjectArray args;
	jstring str;

	int total, i;
	struct lang_list lang[128];

	memset(lang, 0, sizeof(lang));

	if (m_wiki_manage->wiki_get_lang_list(lang, &total) > 0) {
		args = env->NewObjectArray(total, env->FindClass("java/lang/String"),0);

		for (i = 0; i < total; i++) {
			str = env->NewStringUTF(lang[i].lang);
			env->SetObjectArrayElement(args, i, str);
		}
	} else {
		args = env->NewObjectArray(1, env->FindClass("java/lang/String"),0);
		str = env->NewStringUTF("");
		env->SetObjectArrayElement(args, i, str);
	}

	return args;
}

jobjectArray
Java_com_hace_fastwiki_Library_WikiLangList(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiLangList(env, thiz);
}

int
Java_com_hace_fastwiki_FastWiki_WikiInitLang(JNIEnv* env, jobject thiz, jstring _lang)
{
	const char *lang = my_get_string(_lang);

	return m_wiki_manage->wiki_init_one_lang(lang);
}

jstring
Java_com_hace_fastwiki_FastWiki_WikiViewHistory(JNIEnv* env, jobject thiz, jint pos)
{
	int size, n;
	char *buf;

	n = m_wiki_manage->wiki_view_history(pos, &buf, &size);
	if (n <= 0) {
		return env->NewStringUTF("");
	}

	return env->NewStringUTF(buf);
}

jstring
Java_com_hace_fastwiki_FastWiki_WikiViewFavorite(JNIEnv* env, jobject thiz, jint pos)
{
	int size, n;
	char *buf;

	n = m_wiki_manage->wiki_view_favorite(pos, &buf, &size);
	if (n <= 0) {
		return env->NewStringUTF("");
	}

	return env->NewStringUTF(buf);
}

int
Java_com_hace_fastwiki_FastWiki_WikiLoadOnePageDone(JNIEnv *env, jobject thiz)
{
	return m_wiki_manage->wiki_load_one_page_done();
}

/*
 * [flag, title, data]
 * flag:
 * 0   Not found.
 * 1   found
 * 2   http link
 */
jobjectArray
Java_com_hace_fastwiki_FastWiki_WikiParseUrl(JNIEnv* env, jobject thiz, jstring _url)
{
	jobjectArray args;
	jstring str;
	const char *url = my_get_string(_url);
	char flag[8], title[256], *data = NULL;
	const char *buf[3] = {flag, title, NULL};

	args = env->NewObjectArray(3, env->FindClass("java/lang/String"), 0);

	m_wiki_manage->wiki_parse_url(url, flag, title, &data);

	buf[2] = data;

	if (buf[2] == NULL)
		buf[2] = "";

	for (int i = 0; i < 3; i++) {
		str = env->NewStringUTF(buf[i]);
		env->SetObjectArrayElement(args, i, str);
	}

	return args;
}

jstring
Java_com_hace_fastwiki_FastWiki_WikiCurrTitle(JNIEnv* env, jobject thiz)
{
	return env->NewStringUTF(m_wiki_manage->wiki_curr_title());
}

void
Java_com_hace_fastwiki_FastWiki_WikiSleep(JNIEnv* env, jobject thiz, jint sec)
{
	usleep(sec);
}

jint
Java_com_hace_fastwiki_FastWiki_GetTimeOfDay(JNIEnv* env, jobject thiz)
{
	int n;
	struct timeval tv;

	gettimeofday(&tv, NULL);

	n = (tv.tv_sec % 10000000) * 100 + (tv.tv_usec / 10000) % 100;

	return n;
}

jint
Java_com_hace_fastwiki_FastWiki_WikiSetHideMenuFlag(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_set_hide_menu_flag();
}

jint
Java_com_hace_fastwiki_FastWiki_WikiGetHideMenuFlag(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_hide_menu_flag();
}

jint
Java_com_hace_fastwiki_FastWiki_WikiCurrColorMode(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_color_mode();
}

jint
Java_com_hace_fastwiki_FastWiki_WikiSetColorMode(JNIEnv* env, jobject thiz, jint idx)
{
	return m_wiki_config->wc_set_color_mode(idx);
}

jint
Java_com_hace_fastwiki_FastWiki_WikiGetFullScreenFlag(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_full_screen_flag();
}

jint
Java_com_hace_fastwiki_FastwikiSetting_WikiSwitchFullScreen(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_switch_full_screen();
}

jint
Java_com_hace_fastwiki_History_WikiGetFullScreenFlag(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiGetFullScreenFlag(env, thiz);
}

jint
Java_com_hace_fastwiki_Favorite_WikiGetFullScreenFlag(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiGetFullScreenFlag(env, thiz);
}

jint
Java_com_hace_fastwiki_FastwikiSetting_WikiGetFullScreenFlag(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiGetFullScreenFlag(env, thiz);
}

jint
Java_com_hace_fastwiki_Library_WikiGetFullScreenFlag(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiGetFullScreenFlag(env, thiz);
}

jint
Java_com_hace_fastwiki_FastWikiAbout_WikiGetFullScreenFlag(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiGetFullScreenFlag(env, thiz);
}

jobjectArray
Java_com_hace_fastwiki_FastWiki_WikiListColor(JNIEnv* env, jobject thiz)
{
	char list_bg[32], list_fg[32];

	jobjectArray args = env->NewObjectArray(2, env->FindClass("java/lang/String"), 0);

	m_wiki_config->wc_get_list_color(list_bg, list_fg);

	env->SetObjectArrayElement(args, 0, env->NewStringUTF(list_bg));
	env->SetObjectArrayElement(args, 1, env->NewStringUTF(list_fg));

	return args;
}

jobjectArray
Java_com_hace_fastwiki_Library_WikiListColor(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiListColor(env, thiz);
}

jobjectArray
Java_com_hace_fastwiki_History_WikiListColor(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiListColor(env, thiz);
}

jobjectArray
Java_com_hace_fastwiki_Favorite_WikiListColor(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiListColor(env, thiz);
}

jobjectArray
Java_com_hace_fastwiki_History_WikiHistory(JNIEnv* env, jobject thiz)
{
	char title[128];
	int total = m_wiki_manage->wiki_history_begin();

	jobjectArray args = env->NewObjectArray(total, env->FindClass("java/lang/String"), 0);

	for (int i = 0; i < total; i++) {
		m_wiki_manage->wiki_history_next(i, title);
		env->SetObjectArrayElement(args, i, env->NewStringUTF(title));
	}

	return args;
}

jint
Java_com_hace_fastwiki_History_WikiHistoryDelete(JNIEnv* env, jobject thiz, jint idx)
{
	return m_wiki_manage->wiki_delete_history(idx);
}

jint
Java_com_hace_fastwiki_History_WikiHistoryClean(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_clean_history();
}

jstring
Java_com_hace_fastwiki_History_N(JNIEnv* env, jobject thiz, jstring flag)
{
	return Java_com_hace_fastwiki_FastWiki_N(env, thiz, flag);
}

jstring
Java_com_hace_fastwiki_Favorite_N(JNIEnv* env, jobject thiz, jstring flag)
{
	return Java_com_hace_fastwiki_FastWiki_N(env, thiz, flag);
}

jstring
Java_com_hace_fastwiki_Library_N(JNIEnv* env, jobject thiz, jstring flag)
{
	return Java_com_hace_fastwiki_FastWiki_N(env, thiz, flag);
}

jstring
Java_com_hace_fastwiki_FileBrowse_N(JNIEnv* env, jobject thiz, jstring flag)
{
	return Java_com_hace_fastwiki_FastWiki_N(env, thiz, flag);
}

jstring
Java_com_hace_fastwiki_FastwikiSetting_N(JNIEnv* env, jobject thiz, jstring flag)
{
	return Java_com_hace_fastwiki_FastWiki_N(env, thiz, flag);
}

jstring
Java_com_hace_fastwiki_FastWikiAbout_N(JNIEnv* env, jobject thiz, jstring flag)
{
	return Java_com_hace_fastwiki_FastWiki_N(env, thiz, flag);
}

/* favorite */

jint
Java_com_hace_fastwiki_FastWiki_IsFavorite(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_is_favorite();
}

jint
Java_com_hace_fastwiki_FastWiki_AddFavorite(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_add_favorite();
}

jint
Java_com_hace_fastwiki_FastWiki_DeleteFavorite(JNIEnv* env, jobject thiz)
{
	m_wiki_manage->wiki_delete_favorite();

	return 0;
}

jint
Java_com_hace_fastwiki_Favorite_WikiFavoriteDelete(JNIEnv* env, jobject thiz, jint idx)
{
	m_wiki_manage->wiki_delete_favorite_one(idx);

	return 0;
}

jint
Java_com_hace_fastwiki_Favorite_WikiFavoriteClean(JNIEnv* env, jobject thiz)
{
	m_wiki_manage->wiki_clean_favorite();

	return 0;
}

jobjectArray
Java_com_hace_fastwiki_Favorite_WikiFavoriteGet(JNIEnv* env, jobject thiz, jint flag)
{
	int total;
	char title[128], rate[32];
	jobjectArray ret;

	if (flag)
		m_wiki_manage->wiki_favorite_begin();

	total = m_wiki_manage->wiki_favorite_total();
	ret = env->NewObjectArray(total, env->FindClass("java/lang/String"),0);

	for (int i = 0; i < total; i++) {
		m_wiki_manage->wiki_favorite_next(i, title, rate);
		env->SetObjectArrayElement(ret, i, env->NewStringUTF(flag ? title : rate));
	}

	return ret;
}

jobjectArray
Java_com_hace_fastwiki_Favorite_WikiFavoriteRate(JNIEnv *env, jobject thiz)
{
	return Java_com_hace_fastwiki_Favorite_WikiFavoriteGet(env, thiz, 0);
}

jobjectArray
Java_com_hace_fastwiki_Favorite_WikiFavorite(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_Favorite_WikiFavoriteGet(env, thiz, 1);
}


void
Java_com_hace_fastwiki_Favorite_LOG(JNIEnv* env, jobject thiz, jstring arg)
{
	Java_com_hace_fastwiki_FastWiki_LOG(env, thiz, arg);
}

jstring
Java_com_hace_fastwiki_FastWiki_TransLate(JNIEnv* env, jobject thiz, jstring flag)
{
	char *buf;
	const char *key = my_get_string(flag);

	m_wiki_manage->wiki_translate(key, &buf);

	return env->NewStringUTF(buf);
}

jint
Java_com_hace_fastwiki_FastwikiSetting_GetMutilLangListMode(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_mutil_lang_list_mode();
}

jint
Java_com_hace_fastwiki_FastwikiSetting_SetMutilLangListMode(JNIEnv* env, jobject thiz, jint mode)
{
	return m_wiki_config->wc_set_mutil_lang_list_mode(mode);
}

jint
Java_com_hace_fastwiki_FastwikiSetting_GetTranslateShowLine(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_translate_show_line();
}

jint
Java_com_hace_fastwiki_FastwikiSetting_SetTranslateShowLine(JNIEnv* env, jobject thiz, jint x)
{
	return m_wiki_config->wc_set_translate_show_line(x);
}

jint
Java_com_hace_fastwiki_FastwikiSetting_GetUseLanguage(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_use_language();
}

jint
Java_com_hace_fastwiki_FastwikiSetting_SetUseLanguage(JNIEnv* env, jobject thiz, jint idx)
{
	return m_wiki_config->wc_set_use_language(idx);
}

jint
Java_com_hace_fastwiki_FastwikiSetting_GetHomePageFlag(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_home_page_flag();
}

jint
Java_com_hace_fastwiki_FastwikiSetting_SetHomePageFlag(JNIEnv* env, jobject thiz, jint flag)
{
	return m_wiki_config->wc_set_home_page_flag(flag);
}

jint
Java_com_hace_fastwiki_FastwikiSetting_GetRandomFlag(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_random_flag();
}

jint
Java_com_hace_fastwiki_FastwikiSetting_SetRandomFlag(JNIEnv* env, jobject thiz, jint flag)
{
	return m_wiki_config->wc_set_random_flag(flag);
}

jint
Java_com_hace_fastwiki_FastwikiSetting_GetNeedTranslate(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_need_translate();
}

jint
Java_com_hace_fastwiki_FastwikiSetting_SetNeedTranslate(JNIEnv* env, jobject thiz, jint flag)
{
	return m_wiki_config->wc_set_need_translate(flag);
}

jstring
Java_com_hace_fastwiki_FastwikiSetting_TranslateDefault(JNIEnv* env, jobject thiz)
{
	char lang[32];

	m_wiki_config->wc_get_translate_default(lang);

	return env->NewStringUTF(lang);
}

jobjectArray
Java_com_hace_fastwiki_FastwikiSetting_WikiLangList(JNIEnv* env, jobject thiz)
{
	return Java_com_hace_fastwiki_FastWiki_WikiLangList(env, thiz);
}

jint
Java_com_hace_fastwiki_FastwikiSetting_SetTranslateDefault(JNIEnv* env, jobject thiz, jstring lang)
{
	return m_wiki_config->wc_set_translate_default(my_get_string(lang));
}

jint
Java_com_hace_fastwiki_FastwikiSetting_GetBodyImageFlag(JNIEnv* env, jobject thiz)
{
	return m_wiki_config->wc_get_body_image_flag();
}

jint
Java_com_hace_fastwiki_FastwikiSetting_SetBodyImageFlag(JNIEnv* env, jobject thiz, jint flag)
{
	return m_wiki_config->wc_set_body_image_flag(flag);
}

jint
Java_com_hace_fastwiki_FastwikiSetting_SetBodyImagePath(JNIEnv* env, jobject thiz, jstring path)
{
	return m_wiki_config->wc_set_body_image_path(my_get_string(path));
}

jstring
Java_com_hace_fastwiki_FastwikiSetting_GetBodyImagePath(JNIEnv* env, jobject thiz)
{
	return env->NewStringUTF(m_wiki_config->wc_get_body_image_path());
}

int _cmp_file_name(const void *a, const void *b)
{
	const char *pa = (const char *)a;
	const char *pb = (const char *)b;

	return strcmp(pa + 1, pb + 1);
}

jstring
Java_com_hace_fastwiki_FileBrowse_RealPath(JNIEnv* env, jobject thiz, jstring path)
{
	int len;
	char *p, buf[256];

	memset(buf, 0, sizeof(buf));
	strncpy(buf, my_get_string(path), sizeof(buf) - 1);

	len = strlen(buf);

	if (len > 2 && strncmp(buf + len - 3, "/..", 3) == 0) {
		buf[len - 3] = 0;
		if ((p = strrchr(buf, '/')))
			*p = 0;
	}

	if (buf[0] == 0)
		strcpy(buf, "/");

	return env->NewStringUTF(buf);
}

int is_image_fname(const char *fname)
{
	int len;
	const char *img[] = {
		".jpg",
		".png",
		".jpeg",
		".bmp",
		NULL,
	};

	len = strlen(fname);

	if (len <= 3)
		return 0;

	for (int i = 0; img[i]; i++) {
		int t = strlen(img[i]);
		if (strncasecmp(fname + len - t, img[i], t) == 0)
			return 1;
	}

	return 0;
}

#define _MAX_IMAGE_TOTAL 250

jobjectArray
Java_com_hace_fastwiki_FileBrowse_GetFiles(JNIEnv* env, jobject thiz, jstring path)
{
	int total = 0;
	char buf[_MAX_IMAGE_TOTAL][64], tmp[128];
	jobjectArray ret;

	const char *file = my_get_string(path);

	DIR *dirp;
	struct dirent *d;

	if ((dirp = opendir(file)) == NULL)
		dirp = opendir("/");

	sprintf(buf[total++], "1..");

	while ((d = readdir(dirp))) {
		if (d->d_name[0] == '.')
			continue;

		if (total >= _MAX_IMAGE_TOTAL - 1)
			break;

		snprintf(tmp, sizeof(tmp), "%s/%s", file, d->d_name);
		
		if (dashf(tmp) && access(tmp, R_OK) == 0) {
			if (is_image_fname(d->d_name))
				snprintf(buf[total++], sizeof(buf[0]), "0%s", d->d_name);
		} else if (dashd(tmp) && access(tmp, R_OK | X_OK) == 0) {
			snprintf(buf[total++], sizeof(buf[0]), "1%s", d->d_name);
		}
	}

	closedir(dirp);

	qsort(buf, total, 64, _cmp_file_name);

	ret = env->NewObjectArray(total, env->FindClass("java/lang/String"), 0);

	for (int i = 0; i < total; i++) {
		env->SetObjectArrayElement(ret, i, env->NewStringUTF(buf[i]));
	}

	return ret;
}

#ifdef __cplusplus
};
#endif

