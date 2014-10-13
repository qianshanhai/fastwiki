/*
 * Copyright (C) 2014 Qian Shanhai (qianshanhai@gmail.com)
 */
#ifndef __FW_STARDICT_H
#define __FW_STARDICT_H

struct fw_stardict_st {
	char date[64];
	char lang[64];
	char script_file[128];
	char idx[128];
	char dict[128];
	char compress[128];
	int script_flag;
	int max_total;
};

extern "C" {

int stardict_find_title(const char *title, int len);
char *stardict_curr_title(int *len);
char *stardict_curr_content(int *len);
int convert_dict(struct fw_stardict_st *st);

};

#endif
