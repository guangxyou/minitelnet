#include "server.h"
#include <time.h>
#include <signal.h>
#include <pthread.h>
#include <errno.h>
#include <unistd.h>
#include <netdb.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <semaphore.h>

#define	CONNECT	0x00
#define	LOGIN	0x01
#define	EXCUTE	0x02
#define	EXIT	0x03
#define	handle_error(msg) \
	do { perror(msg); exit(EXIT_FAILURE); } while(0)

static int logfd;
static int sockfd;
static sem_t sem_link;
static int thread_num = 0;
static sem_t thread;
struct USER *head = NULL;

int main(int argc, char *argv[])
{
	if (argc != 1)
	{
		server_usage();
		exit(0);
	}

	int nsock;
	char clientip[64];
	pthread_t threadid;
	struct sockaddr_in serveraddr;
	struct sockaddr_in clientaddr;

	if (signal(SIGINT, ctrl_c) == SIG_ERR)
		perror("signal SIGINT failed.");
	
	//O_APPEND方式打开文件,write之前文件指针都移动到文件尾部,
	//这种写日志的操作具有原子性,无需文件锁
	logfd = open("./data/log", O_WRONLY | O_APPEND | O_CREAT, 0644);
	if (logfd == -1)
	{
		perror("open file log failed.");
		exit(EXIT_FAILURE);
	}
	
	//初始化信号量,0代表在线程中共享,1代表初始值
	if (sem_init(&sem_link, 0, 1) == -1)
		handle_error("sem_init link");
	if (sem_init(&thread, 0, 1) == -1)
		handle_error("sem_init thread");

	//链表初始化
	head = creat_void_list();
	load_file(head);
	travers_list(head);

	sockfd = socket(AF_INET, SOCK_STREAM, 0);
	if (sockfd == -1)
	{
		perror("Socket failed.\n");
		exit(0);
	}

	serveraddr.sin_family = AF_INET;
	serveraddr.sin_port = htons((short) PORT);
	serveraddr.sin_addr.s_addr = INADDR_ANY;

	if (bind(sockfd,(struct sockaddr *)&serveraddr,sizeof(serveraddr)) == -1)
	{
		perror("Bind failed.\n");
		exit(EXIT_FAILURE);
	}

	if (listen(sockfd, 256) == -1)
	{
		perror("Listen failed.\n");
		exit(EXIT_FAILURE);
	}

	do {
		socklen_t len = sizeof(clientaddr);
		nsock = accept(sockfd, (struct sockaddr *)&clientaddr, &len);
		if (nsock == -1)
		{
			if (errno == EINTR)
				continue;
			else
				break;
		}
		inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientip, sizeof(clientip));
		printf("IP %s\t is connected.\n",clientip);

		//多线程
		pthread_attr_t attr;
		pthread_attr_init(&attr);
		pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
		if (pthread_create(&threadid, &attr, proc_echo, (void *)&nsock) == -1)
			printf("Creat thread failed.\n");
		write_log(clientip, CONNECT, NULL, NULL);

		sem_wait(&thread);
		thread_num++;
		sem_post(&thread);
	} while (thread_num);
	save_file(head);
	close(sockfd);
	close(logfd);

	return 0;
}

void server_usage(void)
{
	printf("usage: server\n");
	printf("\tfor example: ./server\n");
}

void ctrl_c(int sig)
{
	if (sig == SIGINT)
	{
		if (thread_num != 0)
		{
			printf("\nThere is stills %d %s running.\n", thread_num,
					(thread_num>1) ? "clients" : "client");
			printf("You cannot EXIT server.\n");
		}
		else
		{
			save_file(head);
			close(sockfd);
			close(logfd);
		}
	}
}

void *proc_echo(void *sock)
{
	int ret;
	int nsock = *(int *)sock;
	struct Pack pack;

	encode(&pack,ID_WEL,"Welcome.");
	write(nsock, &pack, sizeof(pack));

	do {
		ret = read(nsock, &pack, sizeof(pack));
		if (ret == 0)
		{
			printf("Client has been closed.\n");
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
			proc_swch(pack, nsock);
		}
	}while (1);
	sem_wait(&thread);
	thread_num--;
	sem_post(&thread);
	close(nsock);
	return NULL;
}

void proc_swch(struct Pack pack, int nsock)
{
	char msg[BUFSIZE];
	struct DATA data;
	struct USER *tmp = NULL;

	//获取客户端IP
	char clientip[64];
	int reval;
	//unsigned short client_port;
	socklen_t clientaddr_lens;
	struct sockaddr_in clientaddr;

	clientaddr_lens = sizeof(clientaddr);
	reval = getpeername(nsock, (struct sockaddr *)&clientaddr, &clientaddr_lens);
	if (reval == 0)
	{
		inet_ntop(AF_INET, &clientaddr.sin_addr.s_addr, clientip, sizeof(clientip));
		//client_port = ntohs(clientaddr.sin_port);
	}
	else
	{
		perror("getpeername");
		strcpy(clientip, "NULL");
	}


	switch(pack.head)
	{
		case ID_REG:		//Register
			{
				apart(pack.data, data.name, data.passwd);
				sem_wait(&sem_link);
				if (reg_chk(head, data.name))	//名字存在返回true
				{
					sem_post(&sem_link);
					encode(&pack, ID_FAIL, "Register failed.");
					write(nsock, &pack, sizeof(pack));
				}
				else
				{
					insert_head(data, head);
					sem_post(&sem_link);
					encode(&pack, ID_CMD, "Register success.");
					write(nsock, &pack, sizeof(pack));
				}
			}
			break;
		case ID_LOG:
			{
				apart(pack.data, data.name, data.passwd);
				sem_wait(&sem_link);
				if (log_chk(head, data.name, data.passwd))
				{
					sem_post(&sem_link);
					encode(&pack, ID_CMD, "Log in success.");
					write(nsock, &pack, sizeof(pack));
				}
				else
				{
					sem_post(&sem_link);
					encode(&pack, ID_FAIL, "Login failed.");
					write(nsock, &pack, sizeof(pack));
				}
			}
			break;
		case ID_CMD:
			{
				if (!strcmp(pack.data, "quit"))
				{
					sem_wait(&sem_link);
					if (!is_empty(head))
					{
						tmp = head->next;
						while (tmp != NULL)
						{
							if (!strcmp(data.name, tmp->data.name))
							{
								tmp->data.status = OFFLINE;
								break;
							}
							else
								tmp = tmp->next;
						}
					}
					sem_post(&sem_link);
					return ;
				}
				write_log(clientip, EXCUTE, data.name, pack.data);
				exec_cmd(pack.data, msg, nsock, &pack);
			}
			break;
		case ID_HEAT:
			printf("still alive.\n");
			break;
		default:
			printf("Something unkown happened.\n");
			printf("info: %x.\n", pack.head);
			exit(EXIT_FAILURE);
	}
}
void apart(char *msg, char *name, char *passwd)
{
	int i = 0;
	while (msg[i] != '#')
	{
		name[i] = msg[i];
		i++;
	}
	name[i] = '\0';
	i++;
	int j = 0;
	while (msg[i] != '\0')
	{
		passwd[j] = msg[i];
		i++;
		j++;
	}
	passwd[j] = '\0';
}

void exec_cmd(char *cmd, char *msg, int nsock, struct Pack *pack)
{
	int file[2];
	if (pipe(file) == -1)
	{
		perror("Pipe failed.\n");
		exit(EXIT_FAILURE);
	}

	pid_t pid;
	int cnt;
	int err;
	char *newargv[4];
	newargv[0] = "bash";
	newargv[1] = "-c";
	newargv[2] = cmd;
	newargv[3] = NULL; 
	char *newenv[] = { NULL };

	pid = fork();
	if (pid == -1)
	{
		perror("Fork failed.\n");
		exit(EXIT_FAILURE);
	}
	else if (pid == 0)
	{
		close(file[0]);
		dup2(file[1], fileno(stdout));
		err = execve("/bin/bash",newargv, newenv);
		//if (err == -1)
		//{
			printf("cmd %s\n",cmd);
			printf("error = %d\n",errno);
			encode(pack, ID_MSG, "Wrong cmd");
			write(nsock, pack, sizeof(struct Pack));
		//}
		close(file[1]);
	}
	else
	{
		wait(NULL);
		close(file[1]);
		//不能写成sizeof(msg),msg此时只是一个4bytes的指针
		while ((cnt = read(file[0], msg, BUFSIZE-1)) > 0)	
		{
			msg[cnt] = '\0';
			encode(pack, ID_MSG, msg);
			write(nsock, pack, sizeof(struct Pack));
		}
		close(file[0]);
		encode(pack, ID_MSG, "MsgOver");
		write(nsock, pack, sizeof(struct Pack));
	}
	return ;
}

void time2str(char *stime)
{
	time_t rawtime;
	struct tm *timeinfo;

	time(&rawtime);
	timeinfo = localtime(&rawtime);
	//月份从0开始,比实际要小1
	sprintf(stime, "%d-%s%d-%s%d %s%d:%s%d:%s%d",
			timeinfo->tm_year+1900, 
			(timeinfo->tm_mon+1 > 9)  ? "" : "0",
			timeinfo->tm_mon+1, 
			(timeinfo->tm_mday > 9) ? "" : "0",
			timeinfo->tm_mday,
			(timeinfo->tm_hour > 9) ? "" : "0",
			timeinfo->tm_hour,
			(timeinfo->tm_min > 9)  ? "" : "0",
			timeinfo->tm_min,
			(timeinfo->tm_sec > 9)  ? "" : "0",
			timeinfo->tm_sec);
}

void write_log(char *ip, int type, char *name, char *cmd)
{
	char stime[20];
	char event[BUFSIZE];
	time2str(stime);
	switch(type)
	{
		case CONNECT:
			sprintf(event,"%s %s %s %s\n",
					stime, "IP", ip, "connected");
			break;
		case LOGIN:
			sprintf(event,"%s %s %s %s%s%s\n",
					stime, "IP", ip, "user <", name, "> login");
			break;
		case EXCUTE:
			sprintf(event,"%s %s %s %s%s%s%s%s\n",
					stime, "IP", ip, "user <", name, "> excute cmd \"", cmd, "\"");
			break;
		case EXIT:
			sprintf(event,"%s %s %s %s%s%s\n",
					stime, "IP", ip, "user <", name, "> exit");
			break;
		default:
			break;
	}
	int cnt = write(logfd, event, strlen(event));
	if (cnt == -1)
	{
		perror("write log file failed.");
		exit(EXIT_FAILURE);
	}
}
