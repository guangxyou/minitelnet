#ifndef CLIENT_FUN_H
#define	CLIENT_FUN_H

#include "common.h"

void client_usage(void);
void sig_alarm(int sig);
void proc_echo(void);
void proc_swch(void);
void select_oprate(int *select, int min, int max);
bool user_register(char *name, char *passwd);
void get_name(char *name);
void get_passwd(char *passwd);
int  hide_passwd(void);
bool check_str(char *str);
void merge(char *name, char *passwd, char *msg);




#endif
