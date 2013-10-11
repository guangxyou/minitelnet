all:server client 

server:server.o user_list.o common.o 
	gcc -o server server.o user_list.o common.o -g -lpthread

server.o:server.c
	gcc -c server.c -g -Wall

user_list.o:user_list.c
	gcc -c user_list.c -g -Wall

common.o:common.c
	gcc -c common.c -g -Wall

client:client.o common.o
	gcc -o client client.o common.o -g
	
client.o:client.c
	gcc -c client.c -g -Wall

clean:
	rm -rf *.o
