#include "user_list.h"

struct USER *creat_void_list(void)
{
	struct USER *head =  malloc(sizeof(struct USER));
	head->next = NULL;

	return head;
}

bool is_empty(struct USER *head)
{
	if (head->next == NULL)
		return true;
	else
		return false;
}

void travers_list(struct USER *head)
{
	if ( is_empty(head) )
	{
		printf("User list is empty.\n");
		return ;
	}
	head = head->next;
	while (head != NULL)
	{
		printf("name: %s passwd: %s\n",head->data.name, head->data.passwd);
		head = head->next;
	}
}

void insert_head(struct DATA data, struct USER *head)
{
	struct USER *new = NULL;
	new = malloc(sizeof(struct USER));

	if ( is_empty(head) )
		new->next = NULL;
	else
		new->next = head->next;
	head->next = new;

	strcpy(new->data.name, data.name);
	strcpy(new->data.passwd, data.passwd);
	new->data.status = OFFLINE;	//下线
}

bool reg_chk(struct USER *head, char *name)
{
	if (is_empty(head))
		return false;
	struct USER *tmp = head->next;
	while (tmp != NULL)
	{
		if (!strcmp(tmp->data.name, name))
			return true;
		else
			tmp = tmp->next;
	}
	return false;
}

bool log_chk(struct USER *head, char *name, char *passwd)
{
	if (is_empty(head))
		return false;
	struct USER *tmp = head->next;
	while (tmp != NULL)
	{
		if ( !strcmp(tmp->data.name, name) &&
				!strcmp(tmp->data.passwd, passwd) )
		{
			if (tmp->data.status == ONLINE)
			{
				printf("%s 已经在线.\n", name);
				exit(EXIT_FAILURE);
			}
			tmp->data.status = ONLINE;
			return true;
		}
		else
			tmp = tmp->next;
	}
	return false;
}

void save_file(struct USER *head)
{
	if (is_empty(head))
		return ;

	int fd;
	fd = open("./data/user", O_WRONLY | O_TRUNC | O_CREAT, 0644);
	if (fd == -1)
	{
		perror("Open user info file failed.");
		exit(EXIT_FAILURE);
	}
	
	struct USER *tmp = head->next;
	while (tmp != NULL)
	{
		write(fd, &tmp->data, sizeof(struct DATA));
		tmp = tmp->next;
	}

	close(fd);
}

void load_file(struct USER *head)
{
	int fd = open("./data/user", O_RDONLY);
	if  (fd == -1)
	{
		perror("open user info file failed.");
		exit(EXIT_FAILURE);
	}
	struct USER tmp;
	int cnt = read(fd, &tmp.data, sizeof(struct DATA));
	if (cnt == 0)
	{
		printf("user info file is empty.\n");
		close(fd);
		return ;
	}
	insert_head(tmp.data, head);
	while (1)
	{
		cnt = read(fd, &tmp.data, sizeof(struct DATA));
		if (cnt == 0)
			break;
		insert_head(tmp.data, head);
	}
	close(fd);
}

void free_list(struct USER *head)
{
	if (is_empty(head))
	{
		free(head);
		return ;
	}

	struct USER *tmp = NULL;
	while (head != NULL)
	{
		tmp = head->next;
		free(head);
		head = tmp;
	}
	tmp = NULL;
	head = NULL;
}

/*
int main()
{
	struct USER *head = creat_void_list();
	//struct DATA data = {"test", "test", 0};
	//insert_head(data, head);
	load_file(head);
	//save_file(head);
	travers_list(head);

	return 0;
}
*/
