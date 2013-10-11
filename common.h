#ifndef	COMMON_H
#define	COMMON_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdbool.h>
#include <fcntl.h>

#define	BUFSIZE	1024
#define	PORT	2013
#define	SIZE	20

#define	ID_WEL	0x00	//欢迎
#define	ID_REG	0x01	//注册
#define	ID_LOG	0x02	//登陆
#define	ID_SUCC	0x03	//登陆成功
#define	ID_FAIL	0x04	//登陆失败
#define	ID_CMD	0x05	//执行命令
#define	ID_MSG	0x06	//命令信息
#define	ID_HEAT	0x07	//心跳包

#pragma pack(1)

struct Pack
{
	int head;
	char data[BUFSIZE];
};

#pragma pack()

void decode(struct Pack *pack);
void encode(struct Pack *pack, int head, const char *data);


#endif
