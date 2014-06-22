/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>
#include <jni.h>

#define MAX_KEY_LEN 64

#include "wiki_version.h"
#include "wiki_local.h"
#include "wiki_common.h"
#include "wiki_manage.h"

#define my_get_string(x) ({ jboolean __f = false; env->GetStringUTFChars(x, &__f); })

extern "C" {

static WikiManage *m_wiki_manage = NULL;

jstring
Java_com_hace_fastwiki_FastWikiAbout_WikiAbout(JNIEnv* env, jobject thiz)
{
	return env->NewStringUTF(m_wiki_manage->wiki_about());
}

jstring
Java_com_hace_fastwiki_MyFileManager_N(JNIEnv* env, jobject thiz, jstring flag)
{
	const char *tmp = my_get_string(flag);

	return env->NewStringUTF(local_msg(tmp));
}

jstring
Java_com_hace_fastwiki_FastWiki_N(JNIEnv* env, jobject thiz, jstring flag)
{
	const char *tmp = my_get_string(flag);

	return env->NewStringUTF(local_msg(tmp));
}

jint
Java_com_hace_fastwiki_FastWiki_IsValidDir(JNIEnv* env, jobject thiz, jstring dir)
{
	const char *tmp = my_get_string(dir);

	if (m_wiki_manage->wiki_add_dir(tmp) == -1)
		return 0;

	return 1;
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
Java_com_hace_fastwiki_FastWiki_IsRandom(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_is_random();
}

jint
Java_com_hace_fastwiki_FastWiki_SetRandom(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_switch_random();
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

jint
Java_com_hace_fastwiki_FastWiki_WikiInit(JNIEnv* env, jobject thiz)
{
	m_wiki_manage = new WikiManage();

	return m_wiki_manage->wiki_init() == -1 ? 0 : 1;
}

jint
Java_com_hace_fastwiki_FastWiki_WikiExit(JNIEnv* env, jobject thiz)
{
	m_wiki_manage->wiki_exit();

	return 0;
}

jstring
Java_com_hace_fastwiki_FastWiki_WikiCurrLang(JNIEnv* env, jobject thiz)
{
	char buf[64];

	m_wiki_manage->wiki_curr_lang(buf);

	return env->NewStringUTF(buf);
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

	/* TODO maybe return -1 */
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

	/* TODO maybe return -1 */
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
	int i, total;
	wiki_title_t *buf;
	jobjectArray args;
	jstring str;

	const char *start;
	
	start = my_get_string(_start);
	m_wiki_manage->wiki_match(start, &buf, &total);

	args = env->NewObjectArray(total, env->FindClass("java/lang/String"),0);

	for (i = 0; i < total; i++) {
		str = env->NewStringUTF(buf[i].title);
		env->SetObjectArrayElement(args, i, str);
	}

	return args;
}

int
Java_com_hace_fastwiki_FastWiki_WikiGetFontSize(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_get_fontsize();
}

/*
 * n = -1 or 1
 */
int
Java_com_hace_fastwiki_FastWiki_WikiSetFontSize(JNIEnv* env, jobject thiz, jint n)
{
	return m_wiki_manage->wiki_set_fontsize(n);
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
	struct lang_list lang[32];

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

int
Java_com_hace_fastwiki_FastWiki_WikiInitLang(JNIEnv* env, jobject thiz, jstring _lang)
{
	const char *lang = my_get_string(_lang);

	return m_wiki_manage->wiki_init_one_lang(lang);
}

jstring
Java_com_hace_fastwiki_FastWiki_WikiViewHistory(JNIEnv* env, jobject thiz, int pos)
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
Java_com_hace_fastwiki_FastWiki_WikiViewFavorite(JNIEnv* env, jobject thiz, int pos)
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

	args = env->NewObjectArray(3, env->FindClass("java/lang/String"),0);
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
	return m_wiki_manage->wiki_set_hide_menu_flag();
}

jint
Java_com_hace_fastwiki_FastWiki_WikiGetHideMenuFlag(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_get_hide_menu_flag();
}

jint
Java_com_hace_fastwiki_FastWiki_WikiCurrColorMode(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_get_color_mode();
}

jint
Java_com_hace_fastwiki_FastWiki_WikiColorMode(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_set_color_mode();
}

jint
Java_com_hace_fastwiki_FastWiki_WikiGetFullScreenFlag(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_get_full_screen_flag();
}

jint
Java_com_hace_fastwiki_FastWiki_WikiSwitchFullScreen(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_switch_full_screen();
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

jobjectArray
Java_com_hace_fastwiki_FastWiki_WikiListColor(JNIEnv* env, jobject thiz)
{
	char list_bg[32], list_fg[32];

	jobjectArray args = env->NewObjectArray(2, env->FindClass("java/lang/String"), 0);

	m_wiki_manage->wiki_get_list_color(list_bg, list_fg);

	env->SetObjectArrayElement(args, 0, env->NewStringUTF(list_bg));
	env->SetObjectArrayElement(args, 1, env->NewStringUTF(list_fg));

	return args;
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
Java_com_hace_fastwiki_History_WikiHistoryDelete(JNIEnv* env, jobject thiz, int idx)
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

/* favorite */

jint
Java_com_hace_fastwiki_FastWiki_IsFavorite(JNIEnv* env, jobject thiz)
{
	return m_wiki_manage->wiki_is_favorite();
}

jint
Java_com_hace_fastwiki_FastWiki_AddFavorite(JNIEnv* env, jobject thiz)
{
	m_wiki_manage->wiki_add_favorite();

	return 0;
}

jint
Java_com_hace_fastwiki_FastWiki_DeleteFavorite(JNIEnv* env, jobject thiz)
{
	m_wiki_manage->wiki_delete_favorite();

	return 0;
}

jint
Java_com_hace_fastwiki_Favorite_WikiFavoriteDelete(JNIEnv* env, jobject thiz, int idx)
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
Java_com_hace_fastwiki_Favorite_WikiFavoriteGet(JNIEnv* env, jobject thiz, int flag)
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

};

