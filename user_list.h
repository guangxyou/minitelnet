#ifndef	USER_LIST_H
#define	USER_LIST_H

#include "common.h"

#define	OFFLINE	0
#define	ONLINE	1
struct DATA
{
	char name[SIZE];
	char passwd[SIZE];
	char status;	//1 ONLINE在线,0 OFFLINE下线
};

struct USER
{
	struct DATA data;
	struct USER *next;
};

struct USER *creat_void_list(void);
bool is_empty(struct USER *head);
void travers_list(struct USER *head);
void insert_head(struct DATA data, struct USER *head);
bool reg_chk(struct USER *head, char *name);
bool log_chk(struct USER *head, char *name, char *passwd);
void save_file(struct USER *head);
void load_file(struct USER *head);
void free_list(struct USER *head);






#endif
