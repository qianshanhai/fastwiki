#include "fastwiki-dict.h"

int main(int argc, char *argv[])
{
	FastwikiDict *dict = new FastwikiDict();

	dict->dict_init("xy", "20131125", "txt");

	dict->dict_add_title("hehe");
	dict->dict_add_title("haha");
	dict->dict_add_title("haha2");

	dict->dict_add_title_done();

	dict->dict_add_page("hehe123", 7, "hehe", 4);
	dict->dict_add_page("haha123", 7, "haha", 4);
	dict->dict_add_page("", 0, "haha2", 5, "haha", 4);

	dict->dict_output();

	return 0;
}
