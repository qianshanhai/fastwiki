/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#ifndef FW_NJI

#include <string.h>
#include <stdlib.h>

#include "q_util.h"
#include "wiki_lua.h"

#define MAX_LUA_FILE (1024*1024)

static content_func_t m_curr_content_func = NULL;
static title_func_t m_curr_title_func = NULL;
static find_title_func_t m_find_title_func = NULL;

int lua_page_title(lua_State *L)
{
	int len;
	char *title;

	title = m_curr_title_func(&len);
	lua_pushstring(L, title);

	return 1;
}

int lua_page_content(lua_State *L)
{
	int len;
	char *content;
	
	content = m_curr_content_func(&len);
	lua_pushlstring(L, content, len);

	return 1;
}

int lua_find_title(lua_State *L)
{
	int idx;
	size_t len;
	const char *title;

	title = lua_tolstring(L, 1, &len);
	idx = m_find_title_func(title, len);

	lua_pushinteger(L, idx);

	return 1;
}

WikiLua::WikiLua()
{
	m_lua = NULL;
	m_script = NULL;
}

WikiLua::~WikiLua()
{
}

int WikiLua::lua_init(const char *file, content_func_t cf, title_func_t tf, find_title_func_t ff)
{
	m_script = (char *)malloc(MAX_LUA_FILE);

	int n = q_read_file(file, m_script, MAX_LUA_FILE);
	if (n <= 0)
		return -1;

	m_script[n] = 0;

	return lua_init_script(NULL, cf, tf, ff);
}

int WikiLua::lua_init_script(const char *script, content_func_t cf, title_func_t tf, find_title_func_t ff)
{ 
	if (script) {
		m_script = (char *)malloc(MAX_LUA_FILE);
		strcpy(m_script, script);
	}

	m_curr_content_func = cf;
	m_curr_title_func = tf;
	m_find_title_func = ff;

	m_lua = luaL_newstate();

	return lua_init_sys(m_lua);
}

int WikiLua::lua_init_sys(lua_State *L)
{
	luaL_openlibs(L);

	lua_register(L, "page_title", lua_page_title);
	lua_register(L, "page_content", lua_page_content);
	lua_register(L, "find_title", lua_find_title);

	lua_newtable(L);
	lua_pushstring(L, "title_flag");
	lua_pushboolean(L, 0);
	lua_settable(L, -3);
	lua_setglobal(L, "fastwiki");

	return 0;
}

int WikiLua::lua_content(char *ret_buf, int max_size, int flag)
{
	size_t len = 0;
	const char *s;
	lua_State *L = m_lua;

	if (flag >= 0 && flag <= 1) {
		lua_getglobal(L, "fastwiki");
		lua_pushstring(L, "title_flag");
		lua_pushboolean(L, flag);
		lua_settable(L, -3);
	}

	luaL_dostring(L, m_script);
	int ret = lua_pcall(L, 0, 0, 0);

	lua_getglobal(L, "content");
	s = lua_tolstring(L, -1, &len);

	if (len > 0)
		memcpy(ret_buf, s, len);

	ret_buf[len] = 0;
	lua_pop(L,1);
	
	return (int)len;
}

#endif
