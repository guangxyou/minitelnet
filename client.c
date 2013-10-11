#include "client.h"
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <termios.h>
#include <sys/stat.h>
#include <errno.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>

#define	FIRST	1
#define	SECOND	2
#define	THIRD	3

int sockfd;
struct Pack pack;

int main(int argc, char *argv[])
{
	if (argc != 2)
	{
		client_usage();
		exit(0);
	}

	struct sockaddr_in serveraddr;

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		printf("Socket failed.\n");
		exit(0);
	}

	if (signal(SIGALRM, sig_alarm) == SIG_ERR)
		perror("signal SIGALRM failed.");
	
	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons((short)PORT);
	inet_pton(AF_INET, argv[1], &serveraddr.sin_addr.s_addr);

	if (connect(sockfd, (struct sockaddr *)&serveraddr, sizeof(serveraddr)) == -1)
	{
		printf("Connect failed.\n");
		exit(0);
	}

	proc_echo();
	close(sockfd);

	return 0;
}

void client_usage(void)
{
	printf("Usage: client <IP Adress>\n");
	printf("\tFor example: ./client 127.0.0.1\n");
}

void sig_alarm(int sig)
{
	if (sig == SIGALRM)
	{
		encode(&pack, ID_HEAT, "heart");
		write(sockfd, &pack, sizeof(pack));
		alarm(5);
	}
}

void proc_echo(void)
{
	int ret;

	while (1)
	{
		ret = read(sockfd, &pack, sizeof(pack));
		if (ret == 0)
		{
			printf("Server closed.\n");
			break;
		}
		else if (ret == -1)
		{
			if (errno == EINTR)
				continue;
			else
				break;
		}
		else
		{
			decode(&pack);
			proc_swch();
		}
	}
}

void proc_swch(void)
{
	char c;
	int select;
	char cmd[SIZE];
	char msg[BUFSIZE];
	char name[SIZE], passwd[SIZE];

	switch(pack.head)
	{
		case ID_WEL:
			{
				system("clear");
				system("cat ./surface/welcome");
				select_oprate(&select, 0, 2);	//选择0～2的数字
				switch(select)
				{
					case 0:		//Exit
						printf("Thank you, goodbye.\n");
						exit(EXIT_SUCCESS);
					case 1:		//Log in
						goto LOGIN;
					case 2:		//Register
						goto REGISTER;
					default:
						exit(EXIT_FAILURE);
				}
			}
			break;
REGISTER:
		case ID_REG:
			{
				if ( user_register(name, passwd) )
				{
					//把name和passwd合并成msg
					merge(name, passwd, msg);
					encode(&pack, ID_REG, msg);
					write(sockfd, &pack, sizeof(pack));
				}
				else
				{
					printf("\nRegister failed.\n");
					exit(EXIT_FAILURE);
				}
			}
			break;
LOGIN:
		case ID_LOG:
			{
				//get name & passwd
				printf("\n请输入用户名: ");
				fflush(stdout);
				getchar();
				get_name(name);
				printf("请输入密码  : ");
				fflush(stdout);
				get_passwd(passwd);

				merge(name, passwd, msg);
				encode(&pack, ID_LOG, msg);
				write(sockfd, &pack, sizeof(pack));
			}
			break;
		case ID_FAIL:
			if (!strcmp(pack.data, "Register failed."))
				printf("\n用户名被已注册.\n");
			if (!strcmp(pack.data, "Login failed."))
				printf("\nLog in failed.\n");
			exit(EXIT_FAILURE);
CMD:
		case ID_CMD:
			{
				alarm(5);
				printf("\nInput cmd('q' to quit): ");
				fflush(stdout);
				fgets(cmd, SIZE, stdin);
				if (strlen(cmd) < SIZE-1)
					cmd[strlen(cmd)-1] = '\0';
				else
					while ((c=getchar())!='\n' && c!=EOF);

				if (strcmp(cmd, "q") == 0)
				{
					encode(&pack, ID_CMD, "quit");
					write(sockfd, &pack, sizeof(pack));
					exit(EXIT_SUCCESS);
				}
				encode(&pack, ID_CMD, cmd);
				write(sockfd, &pack, sizeof(pack));
			}
			break;
		case ID_MSG:
			{
				if (!strcmp(pack.data, "MsgOver"))
					goto CMD;
				if (!strcmp(pack.data, "Wrong cmd"))	//输入命令错误
				{
					perror("Wrong command.");
					goto CMD;
				}
				printf("%s",pack.data);
				fflush(stdout);
			}
			break;
		default:
			exit(EXIT_FAILURE);
	}
}

void select_oprate(int *num, int min, int max)
{
	char c;
	bool flag = true;
	int cnt = 0;
	do {
		if ( flag )
		{
			printf("\n输入选项: ");
			fflush(stdout);
		}
		else
		{
			while ((c=getchar())!='\n' && c!=EOF);
			printf("\n输入错误选项,重新输入: ");
			fflush(stdout);
		}
		cnt = scanf("%d", num);
		if (cnt!=1 || *num<min || *num>max)
			flag = false;
		else
			flag = true;
	}while ( !flag );
}

bool user_register(char *name, char *passwd)
{
	int cnt = 3;	//最多输入3次
	bool flag = true;
	char temp[SIZE];

	do {
		if (cnt-- == 0)
			return false;

		if ( flag )
		{
			printf("\n请输入注册用户名: ");
			fflush(stdout);
		}
		else
		{
			printf("名字非法,重新输入: ");
			fflush(stdout);
		}
		
		getchar();
		get_name(name);

		if ( check_str(name) )
			flag = true;
		else
			flag = false;

	} while ( !flag );


	int times = FIRST;		//输入密码次数
	flag = false;
	printf("请输入用户密码  : ");
	fflush(stdout);
	do {	//检查非法字符并输入两次确认
		if ( times == FIRST )	//第一次输入密码
		{
			get_passwd(passwd);
		}
		else if (times == SECOND)	//第二次输入密码
		{
			get_passwd(temp);
			times = THIRD;
		}
		
		if (times == FIRST)
		{
			if ( check_str(passwd) )
			{
				times = SECOND;
				printf("\n再次输入密码    : ");
				fflush(stdout);
			}
			else
			{
				printf("\n密码非法,重新输入:");
				fflush(stdout);
			}
		}

		if (times == THIRD)
		{
			if (strcmp(passwd, temp) == 0)
			{
				flag = true;	//只有执行此步才能跳出循环
			}
			else
			{
				times = FIRST;	//两次密码不一致，全部重新输入
				printf("\n密码不一致,重新输入:");
				fflush(stdout);
			}
		}
	} while ( !(flag && times==THIRD) );

	return true;
}

void get_name(char *name)
{
	char c;
	fgets(name, SIZE, stdin);
	if (strlen(name) < SIZE-1)
		name[strlen(name)-1] = '\0';
	else
		while ((c=getchar())!='\n' && c!=EOF);
}

int hide_passwd(void)
{
	struct termios tm, tm_old;
	int fd = STDIN_FILENO;
	int c;

	if (tcgetattr(fd, &tm) != 0)
		return -1;
	tm_old = tm;
	cfmakeraw(&tm);
	if (tcsetattr(fd, TCSANOW, &tm) != 0)
		return -1;
	c = fgetc(stdin);
	if (tcsetattr(fd, TCSANOW, &tm_old) != 0)
		return -1;

	return c;
}

void get_passwd(char *passwd)
{
	char temp[SIZE];
	int ch;
	int i = 0;

	while ((ch=hide_passwd())!='\r' && i<SIZE-1)
	{
		temp[i++] = ch;
		putchar('*');
	}
	temp[i] = '\0';
	strcpy(passwd, temp);
}


bool check_str(char *str)
{
	int i = 0;
	while (str[i] != '\0')
	{
		if (str[i]!='#' && str[i]!='&' && str[i]!='@'
				&& str[i]!='*' && str[i]!='*')
			i++;
		else
			return false;
	}

	return true;
}


void merge(char *name, char *passwd, char *msg)
{
	int len = strlen(name);

	strcpy(msg, name);
	msg[len] = '#';
	msg[len+1] = '\0';
	strcat(msg, passwd);
}
