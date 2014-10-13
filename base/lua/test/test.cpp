#include "lua.hpp"

int idx = 0;

int page_content(lua_State *l)
{
	char buf[1024];
	sprintf(buf, "content: %d", idx);
	lua_pushstring(l, buf);
	return 1;
}

int page_title(lua_State* l)
{
	char hello[] = "hello";
	lua_pushstring(l, hello);

	return 1;
}

int lua_find_title(lua_State *L)
{
	size_t len;
	const char *title;

	title = lua_tolstring(L, 1, &len);

	lua_pushinteger(L, idx);

	return 1;
}

int main(int argc, char *argv[])
{
	int ret;
	lua_State *L = luaL_newstate() ;

	if (argc < 2) {
		printf("usage: %s <lua file>\n", argv[0]);
		return 0;
	}

	luaL_openlibs(L);

	lua_register(L, "page_title", page_title);  
	lua_register(L, "page_content", page_content);  
	lua_register(L, "find_title", lua_find_title);

	for (int i = 0; i < 100000; i++) {
		idx = i;

		if (luaL_loadfile(L, argv[1])) {
			printf("error\n");
		}

		ret = lua_pcall(L,0,0,0) ;

		lua_getglobal(L, "content");
		const char *s = lua_tostring(L, -1);
		lua_pop(L,1) ;

	}

	return 0;
}
