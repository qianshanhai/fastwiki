/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */

#ifndef FW_NJI
#ifndef __WIKI_LUA_H
#define __WIKI_LUA_H

#include "lua.hpp"

extern "C" {
	int page_content(lua_State *L);
	int page_title(lua_State *L);
	int find_title(lua_State *L);
};

typedef char *(*content_func_t)(int *len);
typedef char *(*title_func_t)(int *len);
typedef int (*find_title_func_t)(const char *title, int len);

class WikiLua {
	private:
		char *m_script;
		lua_State *m_lua;

	public:
		WikiLua();
		~WikiLua();

		int lua_init(const char *file, content_func_t cf, title_func_t tf, find_title_func_t ff);
		int lua_init_script(const char *script, content_func_t cf, title_func_t tf, find_title_func_t ff);
		int lua_content(char *ret_buf, int max_size = 0);
		int lua_init_sys(lua_State *L);
};

#endif

#endif
