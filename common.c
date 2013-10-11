#include "common.h"
#include <arpa/inet.h>

void decode(struct Pack *pack)
{
	pack->head = ntohl(pack->head);
}

void encode(struct Pack *pack, int head, const char *data)
{
	pack->head = htonl(head);
	strcpy(pack->data, data);
}
