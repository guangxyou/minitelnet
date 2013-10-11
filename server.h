#ifndef	SERVER_FUN_H
#define	SERVER_FUN_H

#include "common.h"
#include "user_list.h"

void server_usage(void);
void ctrl_c(int sig);
void *proc_echo(void *sock);
void proc_swch(struct Pack pack, int nsock);
void apart(char *msg, char *name, char *passwd);
void exec_cmd(char *cmd, char *msg, int nsock, struct Pack *pack);
void write_log(char *ip, int type, char *name, char *cmd);


#endif
