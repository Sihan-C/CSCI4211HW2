
CC = gcc

servers: server_tcp server_udp

server_tcp : server_tcp.c ../message.h
	$(CC) -o server_tcp server_tcp.c

server_udp : server_udp.c ../message.h
	$(CC) -o server_udp server_udp.c
